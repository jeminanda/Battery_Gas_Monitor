/*
 * charge_status.c -- 충전 상태 판별 (INA219 전류 기반)
 * 역할: TP4056 의 상태 핀(CHRG/STDBY)을 쓰지 않고, 충전 전류(약 1A)와 셀 전압으로
 *       미연결 / 충전 중 / 완충(0~2)을 추정한다. INA219 가 없으면 미연결로 본다.
 *       CHG_DEBOUNCE_MS 동안 같은 판정이어야 상태를 바꿔 전류 흔들림을 걸러낸다.
 * 용도: app.c(텔레메트리), soc_estimator.c(완충 시 100% 리셋), dcir.c(충전 중이면 시작 금지)가 사용한다.
 *
 * 판정 규칙
 *   충전 중 : 전류 >= CHG_ON_MA. 한 번 충전 중이 되면 CHG_OFF_MA 까지 유지(테이퍼 구간 히스테리시스)
 *   완충    : 충전 중이었다가 전류가 CHG_OFF_MA 아래로 내려가고 셀 전압이 CHG_DONE_MV 이상이면 진입.
 *             이후 셀 전압이 CHG_RECHARGE_MV 이상이고 방전 중(-CHG_DISCHARGE_MA 이하)이 아니면 유지
 *   미연결  : 그 외
 * 한계: 충전기 연결 여부를 직접 보지 못하므로, 완충 후 어댑터를 뽑아도 셀 전압이 높으면 완충으로 남는다.
 */

#include "charge_status.h"
#include "config.h"
#include "log.h"

static uint8_t  stable = CHARGE_STATE_NONE;     /* 디바운스를 통과한 현재 상태 */
static uint8_t  candidate = CHARGE_STATE_NONE;  /* 막 판정된 후보 상태 */
static uint32_t candidate_since;

/* 현재 값 하나로 "지금 상태는 무엇인가"를 판정 (이전 확정 상태를 히스테리시스에 사용) */
static uint8_t classify(bool power_valid, int32_t current_ma, uint16_t cell_mv)
{
  if (!power_valid) {
    return CHARGE_STATE_NONE;
  }

  if (current_ma >= CHG_ON_MA) {
    return CHARGE_STATE_CHARGING;
  }
  if (stable == CHARGE_STATE_CHARGING) {
    if (current_ma >= CHG_OFF_MA) {
      return CHARGE_STATE_CHARGING;                 /* 테이퍼 구간: 충전 중 유지 */
    }
    if (cell_mv >= CHG_DONE_MV && current_ma > -CHG_DISCHARGE_MA) {
      return CHARGE_STATE_DONE;                     /* 충전 종료 + 높은 전압 = 완충 */
    }
    return CHARGE_STATE_NONE;                       /* 완충 전에 어댑터가 빠진 경우 등 */
  }
  if (stable == CHARGE_STATE_DONE && cell_mv >= CHG_RECHARGE_MV &&
      current_ma > -CHG_DISCHARGE_MA) {
    return CHARGE_STATE_DONE;                       /* 완충 상태 유지 */
  }
  return CHARGE_STATE_NONE;
}

void charge_status_init(void)
{
  stable = candidate = CHARGE_STATE_NONE;
}

void charge_status_update(uint32_t now_ms, bool power_valid, int32_t current_ma, uint16_t cell_mv)
{
  uint8_t raw = classify(power_valid, current_ma, cell_mv);

  if (raw == stable) {
    candidate = stable;                 /* 후보가 현재 상태로 돌아오면 변경 취소 */
    return;
  }
  if (raw != candidate) {
    candidate = raw;                    /* 새 후보 시작 */
    candidate_since = now_ms;
  } else if ((now_ms - candidate_since) >= CHG_DEBOUNCE_MS) {
    stable = candidate;                 /* 같은 판정이 충분히 이어짐 -> 확정 */
    LOG("charge_state=%u (i=%ldmA, cell=%umV)", stable, (long)current_ma, cell_mv);
  }
}

uint8_t charge_status_get(void)
{
  return stable;
}
