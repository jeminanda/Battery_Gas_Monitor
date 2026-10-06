#include "dht11.h"
#include "main.h"
#include "tim.h"

/* TIM2: 1MHz(1us/tick) 32bit free-run. PSC=100-1, APB1 타이머 클럭 100MHz 가정 */
#define DHT_TIM htim2

static void pin_output_idle(void)
{
  GPIO_InitTypeDef gi = {0};
  gi.Pin = DHT11_Pin;
  gi.Mode = GPIO_MODE_OUTPUT_OD;
  gi.Pull = GPIO_PULLUP;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_GPIO_Port, &gi);
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);
}

static inline uint32_t now_us(void)
{
  return __HAL_TIM_GET_COUNTER(&DHT_TIM);
}

static inline void delay_us(uint32_t us)
{
  uint32_t start = now_us();
  while ((now_us() - start) < us) { }
}

/* 핀이 level 이 될 때까지 대기. 경과 us 반환, 타임아웃이면 -1 */
static int32_t wait_level(GPIO_PinState level, uint32_t timeout_us)
{
  uint32_t start = now_us();
  while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) != level) {
    if ((now_us() - start) >= timeout_us) {
      return -1;
    }
  }
  return (int32_t)(now_us() - start);
}

void dht11_init(void)
{
  HAL_TIM_Base_Start(&DHT_TIM);
  pin_output_idle();
}

dht11_status_t dht11_read(int16_t *temp_x10, uint16_t *hum_x10)
{
  uint8_t data[5] = {0};
  dht11_status_t st = DHT11_OK;

  /* 시작 신호: 18ms 이상 Low */
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);

  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);
  delay_us(30);

  /* 센서 응답: Low 80us -> High 80us -> Low (데이터 시작) */
  if (wait_level(GPIO_PIN_RESET, 100) < 0 ||
      wait_level(GPIO_PIN_SET, 100) < 0 ||
      wait_level(GPIO_PIN_RESET, 100) < 0) {
    st = DHT11_ERR_NO_RESPONSE;
  } else {
    for (int i = 0; i < 40 && st == DHT11_OK; i++) {
      if (wait_level(GPIO_PIN_SET, 80) < 0) {
        st = DHT11_ERR_TIMEOUT;
        break;
      }
      int32_t high_us = wait_level(GPIO_PIN_RESET, 100);
      if (high_us < 0) {
        st = DHT11_ERR_TIMEOUT;
        break;
      }
      data[i / 8] <<= 1;
      if (high_us > 40) {   /* '0' 약 26~28us, '1' 약 70us */
        data[i / 8] |= 1;
      }
    }
  }

  __set_PRIMASK(primask);
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);

  if (st != DHT11_OK) {
    return st;
  }
  if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) {
    return DHT11_ERR_CHECKSUM;
  }

  *hum_x10 = (uint16_t)(data[0] * 10 + data[1]);
  *temp_x10 = (int16_t)(data[2] * 10 + (data[3] & 0x7F));
  if (data[3] & 0x80) {
    *temp_x10 = (int16_t)-*temp_x10;
  }
  return DHT11_OK;
}
