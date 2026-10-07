#include "dcir_load.h"
#include "main.h"
#include "tim.h"

static uint32_t timer_clk_hz;

void dcir_load_init(void)
{
  /* APB1 분주가 1 이 아니면 타이머 클럭은 PCLK1 의 2배 */
  timer_clk_hz = HAL_RCC_GetPCLK1Freq();
  if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
    timer_clk_hz *= 2;
  }

  /* 정지 상태로 두고 CCR1 shadow 를 로드 */
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
  htim3.Instance->EGR = TIM_EGR_UG;
  htim3.Instance->SR = 0;
}

uint32_t dcir_load_us_to_ticks(uint32_t us)
{
  return (uint32_t)((uint64_t)us * timer_clk_hz / ((uint64_t)(htim3.Init.Prescaler + 1U) * 1000000ULL));
}

uint32_t dcir_load_pulse_ms(void)
{
  return dcir_load_us_to_ticks(1000U) ? htim3.Init.Period / dcir_load_us_to_ticks(1000U) : 0U;
}

void dcir_load_pulse_start(void)
{
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
  __HAL_TIM_SET_COUNTER(&htim3, 0);
  __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);   /* 출력 활성화 + 카운터 시작, 원펄스라 끝나면 자동 정지 */
}

bool dcir_load_busy(void)
{
  return (htim3.Instance->CR1 & TIM_CR1_CEN) != 0;
}

uint32_t dcir_load_elapsed_ticks(void)
{
  return __HAL_TIM_GET_COUNTER(&htim3);
}

void dcir_load_off(void)
{
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
  __HAL_TIM_SET_COUNTER(&htim3, 0);
}
