#ifndef BME680_PORT_H
#define BME680_PORT_H

/*
 * BME680 포팅 계층 -- 헤더만 존재 (부품 미도착, 구현 없음).
 * config.h 의 ENV_BACKEND_BME680 이 1일 때 env_sensor.c 가 사용한다.
 * 구현 시 Bosch BME68x 공식 드라이버의 read/write/delay 콜백을 I2C1(HAL)로 연결한다.
 * 측정은 forced mode 로 시작하고 poll 로 완료를 확인하는 비동기 방식(블로킹 금지).
 */

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "env_sensor.h"

bool bme680_port_init(I2C_HandleTypeDef *hi2c, uint8_t addr_7bit);

/* 측정 시작 (forced mode) */
bool bme680_port_trigger(void);

/* 측정 완료 시 true 와 함께 out 채움, 진행 중이면 false */
bool bme680_port_poll(env_data_t *out);

#endif /* BME680_PORT_H */
