#ifndef INA219_H
#define INA219_H

#include <stdint.h>
#include <stdbool.h>

/* I2C1 을 APP_I2C_SPEED_HZ 로 설정한다. */
void ina219_bus_init(void);

/* 응답 확인 후 연속 변환(션트/버스 12비트)으로 설정. 성공하면 true. */
bool ina219_probe(void);

/* 션트 전압 (uV) */
bool ina219_read_shunt_uv(int32_t *uv);

/* 버스 전압 (mV) */
bool ina219_read_bus_mv(uint16_t *mv);

/* 전류 (uA). INA219_CURRENT_SIGN 이 적용되어 충전 +, 방전 - */
bool ina219_read_current_ua(int32_t *ua);

#endif /* INA219_H */
