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
#define SAFETY_F_GAS_WARMUP  (1u << 9)   /* MQ 워밍업 중 -> 가스 판정 보류 */

typedef struct {
  uint16_t cell_mv;
  bool     batt_temp_valid;
  int16_t  batt_temp_x10;
  bool     mq_ready;
  uint16_t mq_mv;
} safety_input_t;

typedef struct {
  safety_state_t state;
  uint16_t       flags;
} safety_result_t;

safety_result_t safety_evaluate(const safety_input_t *in);

#endif /* SAFETY_H */
