/*
 * charge_ctrl.c -- 충전 차단 제어 (MOSFET1, PB10)
 * 역할: 충전 허용(High)/차단(Low)을 결정한다. 과충전/과열/가스 경보일 때 차단하고,
 *       DCIR 측정 중에는 외부 요청으로 잠근다. 경보가 풀려도 일정 시간 차단을 유지한다.
 * 용도: app.c 가 안전 상태를 넘겨 갱신하고, dcir.c 가 측정 동안 잠금을 건다.
 *       저전압 경보는 충전이 필요하므로 차단하지 않는다.
 */

#include "charge_ctrl.h"
#include "config.h"
#include "pins.h"
#include "log.h"

#define CUT_FLAGS (SAFETY_F_HIGH_VOLT | SAFETY_F_OVER_TEMP | SAFETY_F_GAS)

static bool     locked;
static bool     cut_valid;
static uint32_t cut_until;
static bool     enabled = true;

static void apply(bool want)
{
  if (want == enabled) {
    return;
  }
  enabled = want;
  HAL_GPIO_WritePin(PIN_CHG_EN_PORT, PIN_CHG_EN, want ? GPIO_PIN_SET : GPIO_PIN_RESET);
  LOG("charge %s", want ? "enabled" : "cut");
}

static bool cut_active(uint32_t now_ms)
{
  return cut_valid && (int32_t)(cut_until - now_ms) > 0;
}

void charge_ctrl_init(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(PIN_CHG_EN_PORT, PIN_CHG_EN, GPIO_PIN_SET);   /* 평소 High */
  GPIO_InitTypeDef gi = {0};
  gi.Pin = PIN_CHG_EN;
  gi.Mode = GPIO_MODE_OUTPUT_PP;
  gi.Pull = GPIO_NOPULL;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PIN_CHG_EN_PORT, &gi);
  enabled = true;
}

void charge_ctrl_set_lock(bool lock)
{
  locked = lock;
  apply(!locked && !cut_active(HAL_GetTick()));
}

void charge_ctrl_update(uint32_t now_ms, const safety_result_t *s)
{
  if (s->state == SAFETY_ALARM && (s->flags & CUT_FLAGS)) {
    cut_valid = true;
    cut_until = now_ms + CHARGE_CUT_HOLD_MS;
  }
  apply(!locked && !cut_active(now_ms));
}

bool charge_ctrl_is_enabled(void)
{
  return enabled;
}
