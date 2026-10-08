#ifndef GAS_MONITOR_H
#define GAS_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "env_sensor.h"

/*
 * BME680 가스 저항의 "기준값 대비 변화율"로 이상(VOC 증가 등)을 판정한다.
 * 절대 저항값은 개체/온도/습도/사용시간마다 크게 달라 임계값으로 쓸 수 없으므로,
 * 깨끗한 공기에서의 기준 저항을 천천히 추적하고 그보다 얼마나 떨어졌는지를 본다.
 */

typedef enum {
  GAS_LEVEL_OK = 0,
  GAS_LEVEL_WARN,
  GAS_LEVEL_ALARM
} gas_level_t;

typedef struct {
  bool        valid;        /* 최근 가스 측정값이 있고 판정이 가능한 상태 */
  bool        warmup;       /* 기준값 안정화 중 (판정 보류) */
  gas_level_t level;
  uint16_t    drop_pm;      /* 기준 대비 감소량 (1/1000), 0~1000 */
  uint32_t    baseline_ohm;
} gas_state_t;

void gas_monitor_init(void);

/* 메인 루프에서 호출. env 에 새 측정(sample_id 변경)이 있을 때만 판정을 갱신한다. */
void gas_monitor_update(uint32_t now_ms, const env_data_t *env);

const gas_state_t *gas_monitor_get(void);

#endif /* GAS_MONITOR_H */
