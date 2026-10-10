/*
 * safety.c -- 안전 판정
 * 역할: 셀 전압, 배터리 온도, 가스 단계를 임계값과 비교해 OK / WARN / ALARM 과 원인 플래그를 만든다.
 * 용도: app.c 가 결과로 상태 LED 를 구동한다.
 */

#include "safety.h"
#include "config.h"
#include "thresholds.h"

/* 더 높은 단계로만 올리고(OK < WARN < ALARM) 원인 플래그를 누적한다 */
static void raise(safety_result_t *r, safety_state_t s, uint16_t flag)
{
  r->flags |= flag;
  if (s > r->state) {
    r->state = s;
  }
}

/* 입력(전압/온도/가스)을 임계값과 비교해 상태와 원인 플래그를 만든다 */
safety_result_t safety_evaluate(const safety_input_t *in)
{
  safety_result_t r = { SAFETY_OK, 0 };
  const thresholds_t *th = thresholds_get();   /* 서버 명령으로 바뀔 수 있는 임계값 */

  /* 셀 전압: 저전압 / 과전압 (각각 경고, 경보 임계값) */
  if (in->cell_mv <= th->cell_low_alarm_mv) {
    raise(&r, SAFETY_ALARM, SAFETY_F_LOW_VOLT);
  } else if (in->cell_mv <= th->cell_low_warn_mv) {
    raise(&r, SAFETY_WARN, SAFETY_F_LOW_VOLT);
  }
  if (in->cell_mv >= th->cell_high_alarm_mv) {
    raise(&r, SAFETY_ALARM, SAFETY_F_HIGH_VOLT);
  } else if (in->cell_mv >= th->cell_high_warn_mv) {
    raise(&r, SAFETY_WARN, SAFETY_F_HIGH_VOLT);
  }

  /* 배터리 온도: 센서가 없거나 무효이면 판정하지 않고 정보 플래그만 세운다 */
  if (!in->batt_temp_valid) {
    r.flags |= SAFETY_F_TEMP_NA;
  } else if (in->batt_temp_x10 >= th->temp_alarm_x10) {
    raise(&r, SAFETY_ALARM, SAFETY_F_OVER_TEMP);
  } else if (in->batt_temp_x10 >= th->temp_warn_x10) {
    raise(&r, SAFETY_WARN, SAFETY_F_OVER_TEMP);
  }

  /* 가스: 센서 값이 없거나 기준값 안정화 중이면 판정하지 않는다 (판정은 gas_monitor 가 수행) */
  if (!in->gas_valid) {
    r.flags |= SAFETY_F_GAS_NA;
  } else if (in->gas_warmup) {
    r.flags |= SAFETY_F_GAS_WARMUP;
  } else if (in->gas_level >= 2) {
    raise(&r, SAFETY_ALARM, SAFETY_F_GAS);
  } else if (in->gas_level == 1) {
    raise(&r, SAFETY_WARN, SAFETY_F_GAS);
  }

  return r;
}
