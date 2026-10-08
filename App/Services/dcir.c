#include "dcir.h"
#include "config.h"
#include "adc_sampler.h"
#include "cell_voltage.h"
#include "charge_ctrl.h"
#include "charge_status.h"
#include "dcir_load.h"
#include "ina219.h"
#include "telemetry.h"
#include "log.h"

/*
 * 내부 저항(DCIR) 측정. 부하(MOSFET2)를 잠깐 걸어 전압이 떨어지는 양으로 계산한다.
 *   DCIR = (V0 - V1) / |I1 - I0|     (V0,I0: 부하 전 / V1,I1: 부하 중)
 * 진행 순서: 인터록 확인 -> 충전기 분리(MOSFET1 OFF) -> 휴지 -> [V0,I0 평균 -> 부하 펄스 -> V1,I1 평균]
 *            x DCIR_REPEAT_N 회(사이에 회복 휴지) -> 평균/표준편차 계산 -> $DCIR 전송 -> 충전 복구.
 * 휴지는 논블로킹 상태머신이고, 한 번의 측정(약 0.2초)만 블로킹으로 수행한다.
 */
#define ADC_TIMEOUT_MS      5
#define BASELINE_GUARD_MS   500
#define PULSE_GUARD_EXTRA_MS 150

typedef enum {
  ST_IDLE = 0,
  ST_REST,        /* 충전기 분리 후 휴지 / 반복 사이 회복 */
} dcir_state_t;

typedef struct {
  int32_t v0_uv, v1_uv, i0_ua, i1_ua;
} sample_t;

static dcir_state_t state;
static bool         pending;
static uint32_t     rest_start;
static uint32_t     rest_len;
static uint8_t      attempts;
static uint8_t      valid_n;
static int32_t      dcir_each[DCIR_REPEAT_N];
static int64_t      sum_v0, sum_v1, sum_i0, sum_i1;
static bool         has_last;
static uint32_t     last_end_ms;
static uint32_t     last_try_ms;
static uint32_t     window_start_ticks;
static uint32_t     window_end_ticks;
static uint32_t     pulse_guard_ms;

/* ADC(PA4)와 INA219 를 번갈아 n 회 읽어 평균 (uV, uA) */
static bool sample_alternating(uint32_t n, int32_t *v_uv, int32_t *i_ua)
{
  uint16_t raw[ADC_IDX_COUNT];
  uint32_t raw_sum = 0;
  int64_t  i_sum = 0;
  uint32_t t0 = HAL_GetTick();

  for (uint32_t k = 0; k < n; k++) {
    int32_t ua;
    if (!adc_sampler_read_once(raw, ADC_TIMEOUT_MS) || !ina219_read_current_ua(&ua)) {
      return false;
    }
    raw_sum += raw[ADC_IDX_CELL];
    i_sum += ua;
    if ((HAL_GetTick() - t0) > BASELINE_GUARD_MS) {
      return false;
    }
  }
  *v_uv = cell_voltage_uv_from_raw_sum(raw_sum, n, adc_sampler_get_vdda_mv());
  *i_ua = (int32_t)(i_sum / (int64_t)n);
  return true;
}

/* 부하 펄스 중 80~98ms 구간을 번갈아 평균 */
static bool sample_window(int32_t *v_uv, int32_t *i_ua)
{
  uint16_t raw[ADC_IDX_COUNT];
  uint32_t raw_sum = 0;
  int64_t  i_sum = 0;
  uint32_t n = 0;
  uint32_t t0 = HAL_GetTick();

  while (dcir_load_busy() && dcir_load_elapsed_ticks() < window_start_ticks) {
    if ((HAL_GetTick() - t0) > pulse_guard_ms) {
      return false;
    }
  }
  while (dcir_load_busy() && dcir_load_elapsed_ticks() < window_end_ticks) {
    int32_t ua;
    if (!adc_sampler_read_once(raw, ADC_TIMEOUT_MS) || !ina219_read_current_ua(&ua)) {
      return false;
    }
    raw_sum += raw[ADC_IDX_CELL];
    i_sum += ua;
    n++;
    if ((HAL_GetTick() - t0) > pulse_guard_ms) {
      return false;
    }
  }
  if (n < DCIR_MIN_WINDOW_SAMPLES) {
    return false;
  }
  *v_uv = cell_voltage_uv_from_raw_sum(raw_sum, n, adc_sampler_get_vdda_mv());
  *i_ua = (int32_t)(i_sum / (int64_t)n);
  return true;
}

/* V0,I0 -> 부하 펄스 -> V1,I1. 성공 시 s 를 채운다 */
static bool measure_once(sample_t *s)
{
  if (!sample_alternating(DCIR_BASELINE_SAMPLES, &s->v0_uv, &s->i0_ua)) {
    return false;
  }

  dcir_load_pulse_start();
  bool ok = sample_window(&s->v1_uv, &s->i1_ua);

  uint32_t t0 = HAL_GetTick();
  while (dcir_load_busy() && (HAL_GetTick() - t0) <= pulse_guard_ms) {
    /* 하드웨어가 펄스를 끝낼 때까지 대기 */
  }
  dcir_load_off();
  return ok;
}

/* 정수 제곱근 (표준편차 계산용, 비트 단위 방식) */
static uint32_t isqrt64(uint64_t x)
{
  uint64_t r = 0;
  uint64_t bit = 1ULL << 62;
  while (bit > x) {
    bit >>= 2;
  }
  while (bit) {
    if (x >= r + bit) {
      x -= r + bit;
      r = (r >> 1) + bit;
    } else {
      r >>= 1;
    }
    bit >>= 2;
  }
  return (uint32_t)r;
}

static void reset_accumulators(void)
{
  attempts = 0;
  valid_n = 0;
  sum_v0 = sum_v1 = sum_i0 = sum_i1 = 0;
}

static void finish(uint32_t now_ms, const dcir_input_t *in)
{
  state = ST_IDLE;
  charge_ctrl_set_lock(false);
  has_last = true;
  last_end_ms = now_ms;

  if (valid_n < DCIR_MIN_VALID) {
    LOG("DCIR failed: valid %u/%u", valid_n, attempts);
    return;
  }

  int64_t sum = 0;
  for (uint8_t i = 0; i < valid_n; i++) {
    sum += dcir_each[i];
  }
  int32_t mean = (int32_t)(sum / valid_n);

  uint64_t var_sum = 0;
  for (uint8_t i = 0; i < valid_n; i++) {
    int64_t d = (int64_t)dcir_each[i] - mean;
    var_sum += (uint64_t)(d * d);
  }

  dcir_result_t r;
  r.tick_ms = now_ms;
  r.v0_uv = (int32_t)(sum_v0 / valid_n);
  r.v1_uv = (int32_t)(sum_v1 / valid_n);
  r.i0_ua = (int32_t)(sum_i0 / valid_n);
  r.i1_ua = (int32_t)(sum_i1 / valid_n);
  r.dcir_uohm = mean;
  r.dcir_std_uohm = (valid_n > 1) ? (int32_t)isqrt64(var_sum / (valid_n - 1)) : 0;
  r.repeat_n = valid_n;
  r.vcell_mv = in->cell_mv;
  r.temp_valid = in->temp_valid;
  r.temp_mc = (int32_t)in->temp_x10 * 100;

  LOG("DCIR %ld uohm (std %ld, n=%u)", (long)r.dcir_uohm, (long)r.dcir_std_uohm, r.repeat_n);
  telemetry_send_dcir(&r);
}

void dcir_init(void)
{
  dcir_load_init();
  window_start_ticks = dcir_load_us_to_ticks(DCIR_WINDOW_START_MS * 1000U);
  window_end_ticks = dcir_load_us_to_ticks(DCIR_WINDOW_END_MS * 1000U);
  pulse_guard_ms = dcir_load_pulse_ms() + PULSE_GUARD_EXTRA_MS;
  if (DCIR_WINDOW_END_MS >= dcir_load_pulse_ms()) {
    LOG("DCIR warning: window end %ums >= TIM3 pulse %lums",
        (unsigned)DCIR_WINDOW_END_MS, (unsigned long)dcir_load_pulse_ms());
  }
  state = ST_IDLE;
  pending = false;
  has_last = false;
}

bool dcir_request(void)
{
  if (state != ST_IDLE || pending) {
    return false;
  }
  pending = true;
  return true;
}

bool dcir_is_active(void)
{
  return state != ST_IDLE;
}

void dcir_abort(const char *reason)
{
  dcir_load_off();
  charge_ctrl_set_lock(false);
  if (state != ST_IDLE) {
    LOG("DCIR aborted: %s", reason);
  }
  state = ST_IDLE;
  pending = false;
}

/* 시작 조건. 실패 이유 문자열을 반환 (통과하면 NULL) */
static const char *interlock(uint32_t now_ms, const dcir_input_t *in)
{
  if (!in->ready) {
    return "no measurement yet";
  }
  if (!in->power_valid) {
    return "INA219 not detected";
  }
  if (in->charge_state == CHARGE_STATE_CHARGING) {
    return "charging";
  }
  if (in->cell_mv < DCIR_MIN_CELL_MV) {
    return "cell voltage too low";
  }
  if (has_last && (now_ms - last_end_ms) < DCIR_MIN_INTERVAL_MS) {
    return "too soon after previous";
  }
  return NULL;
}

void dcir_task(uint32_t now_ms, const dcir_input_t *in)
{
#if DCIR_AUTO_INTERVAL_MS > 0
  if (state == ST_IDLE && !pending && (now_ms - last_try_ms) >= DCIR_AUTO_INTERVAL_MS) {
    pending = true;
  }
#endif

  if (state == ST_IDLE) {
    if (!pending) {
      return;
    }
    pending = false;
    last_try_ms = now_ms;
    const char *why = interlock(now_ms, in);
    if (why) {
      LOG("DCIR canceled: %s", why);
      return;
    }
    LOG("DCIR start: charger cut, resting %lus", (unsigned long)(DCIR_REST_MS / 1000));
    reset_accumulators();
    charge_ctrl_set_lock(true);          /* MOSFET1 OFF */
    rest_start = now_ms;
    rest_len = DCIR_REST_MS;
    state = ST_REST;
    return;
  }

  /* 측정 중 조건 감시 */
  if (!in->power_valid) {
    dcir_abort("INA219 lost");
    return;
  }
  if (in->cell_mv < DCIR_MIN_CELL_MV) {
    dcir_abort("cell voltage too low");
    return;
  }
  if ((now_ms - rest_start) < rest_len) {
    return;
  }

  /* 휴지 종료: 충전기가 실제로 분리됐는지 확인 후 측정 */
  if (in->charge_state == CHARGE_STATE_CHARGING) {
    dcir_abort("charger still active after cut");
    return;
  }

  sample_t s;
  attempts++;
  if (measure_once(&s)) {
    int32_t dv = s.v0_uv - s.v1_uv;
    int32_t di = s.i1_ua - s.i0_ua;
    if (di < 0) {
      di = -di;                          /* 전류 부호 규약과 무관하게 변화량만 사용 */
    }
    if (dv > 0 && di >= DCIR_MIN_DELTA_UA) {
      dcir_each[valid_n++] = (int32_t)((int64_t)dv * 1000000 / di);   /* uV/uA = ohm -> uohm */
      sum_v0 += s.v0_uv;
      sum_v1 += s.v1_uv;
      sum_i0 += s.i0_ua;
      sum_i1 += s.i1_ua;
    } else {
      LOG("DCIR sample rejected (dv=%ldmV di=%ldmA)", (long)(dv / 1000), (long)(di / 1000));
    }
  } else {
    LOG("DCIR sample failed (read error/timing)");
  }

  if (attempts >= DCIR_REPEAT_N) {
    finish(HAL_GetTick(), in);
  } else {
    rest_start = HAL_GetTick();
    rest_len = DCIR_REPEAT_REST_MS;
  }
}
