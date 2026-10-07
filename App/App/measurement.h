#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <stdint.h>
#include <stdbool.h>
#include "env_sensor.h"
#include "safety.h"

/* 한 주기의 측정 결과 스냅샷. telemetry/log 가 읽는다. */
typedef struct {
  uint32_t tick_ms;
  uint16_t vdda_mv;
  uint16_t cell_mv;
  uint8_t  soc_pct;
  bool     batt_temp_valid;    /* NTC 없으면 항상 false */
  int16_t  batt_temp_x10;
  bool     mq_ready;
  uint16_t mq_mv;
  env_data_t env;
  safety_result_t safety;

  /* 전류/충전 (INA219, TP4056) */
  bool     power_valid;        /* INA219 가 없으면 false */
  uint16_t vbus_mv;
  int16_t  current_ma;         /* 충전 +, 방전 - */
  uint8_t  charge_state;       /* 0 미연결 / 1 충전 중 / 2 완충 / 3 비정상 */
  bool     charge_enabled;     /* 충전 차단 MOSFET 상태 (PB10) */
  bool     soc_valid;
  uint16_t soc_permil;         /* 전류 적분 SOC, 0~1000 */
} measurement_t;

#endif /* MEASUREMENT_H */
