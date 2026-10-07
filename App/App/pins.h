#ifndef APP_PINS_H
#define APP_PINS_H

#include "main.h"

/* CubeMX(.ioc) 에 없는 핀은 각 드라이버가 직접 초기화한다. */

/* TP4056 상태 출력 (open-drain, Low 가 활성) */
#define PIN_CHRG_PORT   GPIOB
#define PIN_CHRG        GPIO_PIN_5
#define PIN_STDBY_PORT  GPIOB
#define PIN_STDBY       GPIO_PIN_4

#endif /* APP_PINS_H */
