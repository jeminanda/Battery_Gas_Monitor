#include "tp4056.h"
#include "pins.h"

void tp4056_init(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef gi = {0};
  gi.Pin = PIN_CHRG | PIN_STDBY;
  gi.Mode = GPIO_MODE_INPUT;
  gi.Pull = GPIO_PULLUP;   /* 모듈이 없으면 High = "미연결" */
  HAL_GPIO_Init(GPIOB, &gi);
}

bool tp4056_chrg_active(void)
{
  return HAL_GPIO_ReadPin(PIN_CHRG_PORT, PIN_CHRG) == GPIO_PIN_RESET;
}

bool tp4056_stdby_active(void)
{
  return HAL_GPIO_ReadPin(PIN_STDBY_PORT, PIN_STDBY) == GPIO_PIN_RESET;
}
