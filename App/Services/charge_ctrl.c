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
