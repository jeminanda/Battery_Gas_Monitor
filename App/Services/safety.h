#ifndef SAFETY_H
#define SAFETY_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  SAFETY_OK = 0,
  SAFETY_WARN,
  SAFETY_ALARM
} safety_state_t;

/* 상태에 영향을 주는 플래그 */
#define SAFETY_F_LOW_VOLT    (1u << 0)
#define SAFETY_F_HIGH_VOLT   (1u << 1)
#define SAFETY_F_OVER_TEMP   (1u << 2)
#define SAFETY_F_GAS         (1u << 3)
/* 정보성 플래그 (상태를 올리지 않음) */
#define SAFETY_F_TEMP_NA     (1u << 8)   /* 배터리 온도센서 없음/무효 -> 과열 판정 불가 */
#define SAFETY_F_GAS_WARMUP  (1u << 9)   /* 가스 기준값 안정화 중 -> 가스 판정 보류 */
#define SAFETY_F_GAS_NA      (1u << 10)  /* 가스 센서 값 없음 -> 가스 감시 불가 */

typedef struct {
  uint16_t cell_mv;
  bool     batt_temp_valid;
  int16_t  batt_temp_x10;
  bool     gas_valid;     /* 가스 판정이 가능한 상태 (센서 값 있음) */
  bool     gas_warmup;    /* 가스 기준값 안정화 중 -> 판정 보류 */
  uint8_t  gas_level;     /* 0 정상 / 1 경고 / 2 경보 (gas_monitor 의 gas_level_t) */
} safety_input_t;

typedef struct {
  safety_state_t state;
  uint16_t       flags;
} safety_result_t;

safety_result_t safety_evaluate(const safety_input_t *in);

#endif /* SAFETY_H */
