#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

typedef enum {
  DHT11_OK = 0,
  DHT11_ERR_NO_RESPONSE,
  DHT11_ERR_TIMEOUT,
  DHT11_ERR_CHECKSUM
} dht11_status_t;

/* TIM2(1MHz) 시작 (us 딜레이용) 및 핀 idle(High) 설정. MX_TIM2_Init 이후 호출 */
void dht11_init(void);

/* 읽는 동안 약 4ms 인터럽트를 비활성화한다. 호출 간격은 1s 이상 유지할 것. */
dht11_status_t dht11_read(int16_t *temp_x10, uint16_t *hum_x10);

#endif /* DHT11_H */
