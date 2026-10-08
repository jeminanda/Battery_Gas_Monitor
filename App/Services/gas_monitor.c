#include "gas_monitor.h"
#include "config.h"
#include "log.h"

static gas_state_t st;
static uint32_t    last_sample_id;
static uint32_t    last_sample_ms;
static uint32_t    warmup_start_ms;
static bool        have_baseline;
static gas_level_t pending_level;
static uint8_t     pending_cnt;

/* 현재 단계(level)와 감소량으로 "원하는 단계"를 정한다. 내려올 때는 히스테리시스를 둔다. */
static gas_level_t decide(gas_level_t level, uint32_t drop_pm)
{
  uint32_t alarm_th = GAS_DROP_ALARM_PM - ((level == GAS_LEVEL_ALARM) ? GAS_DROP_HYST_PM : 0U);
  uint32_t warn_th = GAS_DROP_WARN_PM - ((level >= GAS_LEVEL_WARN) ? GAS_DROP_HYST_PM : 0U);

  if (drop_pm >= alarm_th) {
    return GAS_LEVEL_ALARM;
  }
  if (drop_pm >= warn_th) {
    return GAS_LEVEL_WARN;
  }
  return GAS_LEVEL_OK;
}

/* 기준값을 gas 쪽으로 1/2^shift 만큼 이동 */
static void move_baseline(uint32_t gas, unsigned shift)
{
  int32_t diff = (int32_t)gas - (int32_t)st.baseline_ohm;
  st.baseline_ohm = (uint32_t)((int32_t)st.baseline_ohm + diff / (1 << shift));
}

void gas_monitor_init(void)
{
  st = (gas_state_t){ .valid = false, .warmup = true, .level = GAS_LEVEL_OK };
  have_baseline = false;
  pending_cnt = 0;
}

void gas_monitor_update(uint32_t now_ms, const env_data_t *env)
{
  bool has_gas = (env->valid_mask & ENV_HAS_GAS) && env->gas_ohm > 0;

  /* 새 측정이 없으면 일정 시간 뒤에 무효 처리 */
  if (!has_gas || env->sample_id == last_sample_id) {
    if (st.valid && (now_ms - last_sample_ms) > GAS_STALE_MS) {
      st.valid = false;
      LOG("gas monitor: no fresh gas data");
    }
    return;
  }
  last_sample_id = env->sample_id;
  last_sample_ms = now_ms;

  uint32_t gas = env->gas_ohm;
  if (!have_baseline) {
    have_baseline = true;
    st.baseline_ohm = gas;
    warmup_start_ms = now_ms;
    LOG("gas monitor: first sample %lu ohm, warming up %lus",
        (unsigned long)gas, (unsigned long)(GAS_WARMUP_MS / 1000));
  }
  if (!st.valid) {
    st.valid = true;
  }

  st.warmup = (now_ms - warmup_start_ms) < GAS_WARMUP_MS;

  if (st.warmup) {
    /* 안정화 구간: 기준값이 위/아래 모두 빠르게 따라가고 판정은 하지 않는다 */
    move_baseline(gas, GAS_BASE_UP_SHIFT);
    st.drop_pm = 0;
    st.level = GAS_LEVEL_OK;
    pending_cnt = 0;
    return;
  }

  uint32_t drop_pm = 0;
  if (gas < st.baseline_ohm) {
    drop_pm = (uint32_t)(((uint64_t)(st.baseline_ohm - gas) * 1000ULL) / st.baseline_ohm);
  }
  st.drop_pm = (uint16_t)drop_pm;

  gas_level_t desired = decide(st.level, drop_pm);
  if (desired == st.level) {
    pending_cnt = 0;
  } else if (desired == pending_level) {
    if (++pending_cnt >= GAS_CONFIRM_SAMPLES) {
      st.level = desired;
      pending_cnt = 0;
      LOG("gas level=%d (drop=%u.%u%% gas=%lu base=%lu)", st.level,
          (unsigned)(drop_pm / 10), (unsigned)(drop_pm % 10),
          (unsigned long)gas, (unsigned long)st.baseline_ohm);
    }
  } else {
    pending_level = desired;
    pending_cnt = 1;
  }

  /* 기준값 추적: 깨끗해지는 방향(상승)은 빠르게, 하락은 아주 천천히.
   * 이상 상태에서는 기준이 따라 내려가 경보가 사라지지 않도록 하락 추적을 멈춘다. */
  if (gas > st.baseline_ohm) {
    move_baseline(gas, GAS_BASE_UP_SHIFT);
  } else if (st.level == GAS_LEVEL_OK && desired == GAS_LEVEL_OK) {
    move_baseline(gas, GAS_BASE_DOWN_SHIFT);
  }
}

const gas_state_t *gas_monitor_get(void)
{
  return &st;
}
