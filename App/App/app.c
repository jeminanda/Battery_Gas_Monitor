/*
 * app.c -- 애플리케이션 최상위 (메인 루프 조립)
 * 역할: 각 센서/통신 모듈을 초기화하고 주기적으로 실행하며, 측정 결과를 measurement_t
 *       한 덩어리로 모아 블루투스 텔레메트리, 디버그 로그, 상태 LED 로 내보낸다.
 * 용도: main.c 의 USER CODE 에서 app_init() 을 한 번, app_loop() 을 while(1) 에서 반복 호출한다.
 *       블루투스 명령("DCIR", "DCIR STOP")과 B1 버튼(DCIR 시작) 입력도 여기서 처리한다.
 */

#include "app.h"
#include "config.h"
#include "main.h"
#include "measurement.h"
#include "adc_sampler.h"
#include "env_sensor.h"
#include "battery_soc.h"
#include "safety.h"
#include "telemetry.h"
#include "bt_link.h"
#include "log.h"
#include "cell_voltage.h"
#include "gas_monitor.h"
#include "power_monitor.h"
#include "charge_status.h"
#include "soc_estimator.h"
#include "dcir.h"
#include "thresholds.h"
#include "command.h"
#include <string.h>
#include <stdlib.h>
#if APP_HAVE_NTC
#include "ntc.h"
#endif

static measurement_t meas;        /* 가장 최근 측정 스냅샷 (telemetry/log/dcir 가 읽음) */
static bool          meas_valid;  /* measure_update 가 한 번이라도 실행됐는지 */

/* 주기 판정: period 이상 지났으면 *last 를 now 로 갱신하고 true */
static bool due(uint32_t *last, uint32_t now, uint32_t period)
{
  if ((now - *last) >= period) {
    *last = now;
    return true;
  }
  return false;
}

/* 블루투스로 한 줄(개행 제외)을 받으면 호출된다. 명령 해석은 command.c (임계값 변경만 지원) */
static void on_bt_line(const char *line)
{
  LOG("BT rx: %s", line);
  command_handle_line(line);
}

/* B1(사용자 버튼, 누르면 Low): DCIR 측정 시작. 서버 없이 보드만으로 시험할 때 사용 */
static void poll_button(uint32_t now)
{
  static uint32_t last;
  static bool prev;
  if (!due(&last, now, 20)) {
    return;
  }
  bool pressed = (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET);
  if (pressed && !prev) {
    LOG("B1 pressed: DCIR request");
    if (!dcir_request()) {
      LOG("DCIR already running");
    }
  }
  prev = pressed;
}

/*
 * 센서값을 모아 meas 를 갱신한다 (PERIOD_EVAL_MS 주기).
 * 순서: 셀 전압 -> 배터리 온도(NTC) -> 환경/가스 -> 안전 판정 -> 전류/충전/SOC
 */
static void measure_update(uint32_t now)
{
  meas.tick_ms = now;
  meas.vdda_mv = adc_sampler_get_vdda_mv();
  /* 셀 전압(분압 -> mV)과 OCV 테이블 기반 SOC. 전류 적분 SOC 는 아래 soc_estimator 가 따로 계산 */
  meas.cell_mv = cell_voltage_mv_from_raw(adc_sampler_get_raw(ADC_IDX_CELL), meas.vdda_mv);
  meas.soc_pct = battery_soc_pct_from_mv(meas.cell_mv);

  /* 배터리 온도: NTC 가 단선/단락이면 valid=false (안전 판정에서 TEMP_NA 로 표시) */
#if APP_HAVE_NTC
  uint16_t ntc_raw = adc_sampler_get_raw(ADC_IDX_NTC);
  meas.batt_temp_valid = ntc_temp_x10_from_raw(ntc_raw, meas.vdda_mv, &meas.batt_temp_x10);
  static bool ntc_reported, ntc_last;
  if (!ntc_reported || meas.batt_temp_valid != ntc_last) {
    ntc_reported = true;
    ntc_last = meas.batt_temp_valid;
    if (meas.batt_temp_valid) {
      LOG("NTC OK: %d.%d C (raw=%u)", meas.batt_temp_x10 / 10, abs(meas.batt_temp_x10 % 10), ntc_raw);
    } else {
      LOG("NTC not detected (raw=%u): open or short", ntc_raw);
    }
  }
#else
  meas.batt_temp_valid = false;
#endif

  /* BME680 환경값과 가스 이상 판정(기준 저항 대비 감소율) */
  meas.env = *env_sensor_get();
  gas_monitor_update(now, &meas.env);
  meas.gas = *gas_monitor_get();

  /* 전압/온도/가스를 종합해 OK / WARN / ALARM 결정 */
  safety_input_t in = {
    .cell_mv = meas.cell_mv,
    .batt_temp_valid = meas.batt_temp_valid,
    .batt_temp_x10 = meas.batt_temp_x10,
    .gas_valid = meas.gas.valid,
    .gas_warmup = meas.gas.warmup,
    .gas_level = (uint8_t)meas.gas.level,
  };
  meas.safety = safety_evaluate(&in);

  /* 전류(INA219), 전류 기반 충전 상태, 전류 적분 SOC */
  const power_data_t *pw = power_monitor_get();
  meas.power_valid = pw->valid;
  meas.vbus_mv = pw->vbus_mv;
  meas.current_ma = (int16_t)pw->current_ma;
  charge_status_update(now, pw->valid, pw->current_ma, meas.cell_mv);
  meas.charge_state = charge_status_get();
  soc_estimator_update(now, pw->valid, pw->current_ma, meas.cell_mv, meas.charge_state);
  meas.soc_valid = soc_estimator_valid();
  meas.soc_permil = soc_estimator_permil();

  meas_valid = true;
}

/* OK: 1초 점멸, WARN: 0.25초 점멸, ALARM: 상시 점등 */
static void update_led(uint32_t now)
{
  static uint32_t last;
  uint32_t half_period = (meas.safety.state == SAFETY_WARN) ? 250 : 500;

  if (meas_valid && meas.safety.state == SAFETY_ALARM) {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  } else if (due(&last, now, half_period)) {
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
  }
}

/* main() 의 모든 주변장치 초기화(MX_*_Init) 이후 한 번 호출 */
void app_init(void)
{
  log_init();
  thresholds_init();
  adc_sampler_init();
  env_sensor_init();
  gas_monitor_init();
  power_monitor_init();
  charge_status_init();
  soc_estimator_init();
  dcir_init();
  bt_link_init();
  bt_link_set_line_cb(on_bt_line);
  LOG("Battery monitor boot (ntc=%d)", APP_HAVE_NTC);
}

/* main() 의 while(1) 에서 계속 호출. 논블로킹으로 각 모듈의 task 를 돌린다 */
void app_loop(void)
{
  static uint32_t t_eval, t_tx, t_log;   /* 측정 / 텔레메트리 / 로그 주기 타이머 */
  uint32_t now = HAL_GetTick();

  /* 입력 쪽 task: ADC 평균, 환경센서, INA219, 버튼, 블루투스 수신 */
  adc_sampler_task(now);
  env_sensor_task(now);
  power_monitor_task(now);
  poll_button(now);
  bt_link_task();

  /* ADC 평균이 한 번 나온 뒤부터 측정/전송 시작 */
  if (adc_sampler_ready()) {
    if (due(&t_eval, now, PERIOD_EVAL_MS)) {
      measure_update(now);
    }
    /* DCIR 상태머신에 현재 상태를 전달 (인터록 판정용). 측정 중에는 최대 약 0.2초 블로킹 */
    dcir_input_t di = {
      .ready = meas_valid,
      .cell_mv = meas.cell_mv,
      .charge_state = meas.charge_state,
      .power_valid = meas.power_valid,
      .current_ma = meas.current_ma,
      .alarm = (meas.safety.state == SAFETY_ALARM),
      .temp_valid = meas.batt_temp_valid || (meas.env.valid_mask & ENV_HAS_TEMP),
      .temp_x10 = meas.batt_temp_valid ? meas.batt_temp_x10 : meas.env.temp_x10,
    };
    dcir_task(now, &di);

    if (meas_valid && due(&t_tx, now, PERIOD_TELEMETRY_MS)) {
      telemetry_send(&meas);   /* 블루투스 $BAT 패킷 */
    }
    if (meas_valid && due(&t_log, now, PERIOD_LOG_MS)) {   /* 디버그 UART(USART2) 로그 */
      LOG("cell=%umV soc=%u%% env_t=%d state=%d flags=0x%X ina=%d i=%dmA vbus=%umV chg=%u socq=%u",
          meas.cell_mv, meas.soc_pct, meas.env.temp_x10,
          meas.safety.state, meas.safety.flags,
          meas.power_valid, meas.current_ma, meas.vbus_mv,
          meas.charge_state, meas.soc_permil);
      LOG("ntc=%d(%d) env=0x%X h=%u p=%u gas=%lu base=%lu drop=%u.%u%% lvl=%d%s",
          meas.batt_temp_valid, meas.batt_temp_x10, meas.env.valid_mask,
          meas.env.hum_x10, meas.env.press_hpa_x10, (unsigned long)meas.env.gas_ohm,
          (unsigned long)meas.gas.baseline_ohm,
          (unsigned)(meas.gas.drop_pm / 10), (unsigned)(meas.gas.drop_pm % 10),
          meas.gas.level, meas.gas.warmup ? " (warmup)" : "");
    }
  }
  update_led(now);
}
