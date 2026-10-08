/*
 * charge_status.c -- 충전 상태 판별 (TP4056)
 * 역할: CHRG/STDBY 두 핀 조합을 미연결/충전 중/완충/비정상(0~3)으로 바꾸고,
 *       CHG_DEBOUNCE_MS 동안 같은 값이어야 상태를 바꿔 CHRG 점멸 같은 흔들림을 걸러낸다.
 * 용도: app.c(텔레메트리), soc_estimator.c(완충 시 100% 리셋), dcir.c(충전 중이면 시작 금지)가 사용한다.
 */

#include "charge_status.h"
#include "config.h"
#include "tp4056.h"
#include "log.h"

#define SAMPLE_MS 20

static uint8_t  stable = CHARGE_STATE_NONE;
static uint8_t  candidate = CHARGE_STATE_NONE;
static uint32_t candidate_since;
static uint32_t last_ms;

static uint8_t read_raw(void)
{
  bool chrg = tp4056_chrg_active();
  bool stdby = tp4056_stdby_active();
  if (chrg && stdby) {
    return CHARGE_STATE_FAULT;
  }
  if (chrg) {
    return CHARGE_STATE_CHARGING;
  }
  if (stdby) {
    return CHARGE_STATE_DONE;
  }
  return CHARGE_STATE_NONE;
}

void charge_status_init(void)
{
  tp4056_init();
  stable = candidate = read_raw();
}

void charge_status_task(uint32_t now_ms)
{
  if ((now_ms - last_ms) < SAMPLE_MS) {
    return;
  }
  last_ms = now_ms;

  uint8_t raw = read_raw();
  if (raw != candidate) {
    candidate = raw;
    candidate_since = now_ms;
  } else if (candidate != stable && (now_ms - candidate_since) >= CHG_DEBOUNCE_MS) {
    stable = candidate;
    LOG("charge_state=%u", stable);
  }
}

uint8_t charge_status_get(void)
{
  return stable;
}
