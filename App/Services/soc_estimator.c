/*
 * soc_estimator.c -- 전류 적분 SOC 추정
 * 역할: 전류를 사다리꼴로 적분해 잔량(0.1%)을 추적한다. 시작 시 OCV 테이블로 초기화하고,
 *       완충(STDBY) 시 100% 로 리셋하며, 휴지가 길게 이어지면 OCV 로 보정한다.
 * 용도: app.c 가 주기마다 갱신하고 결과를 텔레메트리의 soc_permil 로 보낸다.
 *       INA219 나 배터리가 없으면 무효(NA)이다.
 */

#include "soc_estimator.h"
#include "config.h"
#include "battery_soc.h"
#include "charge_status.h"
#include "log.h"

/* 용량: uA*s */
#define CAPACITY_UAS ((int64_t)BATT_CAPACITY_MAH * 1000 * 3600)

static bool     valid;
static int64_t  charge_uas;        /* 현재 충전량 */
static uint32_t last_ms;
static int32_t  last_ma;
static bool     rest_active;
static uint32_t rest_since;

static int64_t uas_from_permil(uint16_t permil)
{
  return CAPACITY_UAS * permil / 1000;
}

void soc_estimator_init(void)
{
  valid = false;
  rest_active = false;
}

void soc_estimator_update(uint32_t now_ms, bool power_valid, int32_t current_ma,
                          uint16_t cell_mv, uint8_t charge_state)
{
  if (!power_valid || cell_mv < SOC_BATT_PRESENT_MV) {
    valid = false;
    rest_active = false;
    return;
  }

  if (!valid) {
    charge_uas = uas_from_permil(battery_soc_permil_from_mv(cell_mv));
    last_ms = now_ms;
    last_ma = current_ma;
    valid = true;
    LOG("SOC start (OCV) %u/1000", (unsigned)soc_estimator_permil());
    return;
  }

  uint32_t dt = now_ms - last_ms;
  last_ms = now_ms;
  if (dt <= SOC_MAX_DT_MS) {
    /* 사다리꼴: mA * ms = uA*s */
    charge_uas += ((int64_t)(last_ma + current_ma) * (int64_t)dt) / 2;
  }
  last_ma = current_ma;

  if (charge_state == CHARGE_STATE_DONE) {
    charge_uas = CAPACITY_UAS;       /* 완충 리셋 */
  }

  /* 휴지 시 OCV 보정 */
  int32_t abs_ma = (current_ma < 0) ? -current_ma : current_ma;
  if (abs_ma <= SOC_REST_CURRENT_MA && charge_state != CHARGE_STATE_DONE) {
    if (!rest_active) {
      rest_active = true;
      rest_since = now_ms;
    } else if ((now_ms - rest_since) >= SOC_REST_CORRECT_MS) {
      charge_uas = uas_from_permil(battery_soc_permil_from_mv(cell_mv));
      rest_since = now_ms;
      LOG("SOC OCV correction %u/1000", (unsigned)soc_estimator_permil());
    }
  } else {
    rest_active = false;
  }

  if (charge_uas < 0) {
    charge_uas = 0;
  } else if (charge_uas > CAPACITY_UAS) {
    charge_uas = CAPACITY_UAS;
  }
}

bool soc_estimator_valid(void)
{
  return valid;
}

uint16_t soc_estimator_permil(void)
{
  int64_t p = charge_uas * 1000 / CAPACITY_UAS;
  if (p < 0) {
    return 0;
  }
  return (p > 1000) ? 1000 : (uint16_t)p;
}
