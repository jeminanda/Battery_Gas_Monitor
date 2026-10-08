/*
 * tp4056.c -- TP4056 충전기 상태 핀 입력
 * 역할: 충전기의 상태 출력(CHRG, STDBY)을 읽는 GPIO 입력(PB5, PB4)을 설정하고 읽는다.
 *       두 핀 모두 open-drain 이고 Low 가 활성이다.
 * 용도: charge_status.c 가 이 값을 디바운스해 충전 상태(미연결/충전 중/완충)로 만든다.
 */

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
