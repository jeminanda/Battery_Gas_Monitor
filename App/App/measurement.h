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
} measurement_t;

#endif /* MEASUREMENT_H */
