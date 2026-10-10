#ifndef THRESHOLDS_H
#define THRESHOLDS_H

#include <stdint.h>
#include <stdbool.h>

/* 서버에서 바꿀 수 있는 안전 임계값 (기본값은 config.h 의 SAFE_*, GAS_DROP_*) */
typedef struct {
  int32_t cell_low_alarm_mv;
  int32_t cell_low_warn_mv;
  int32_t cell_high_warn_mv;
  int32_t cell_high_alarm_mv;
  int32_t temp_warn_x10;        /* 0.1 C */
  int32_t temp_alarm_x10;       /* 0.1 C */
  int32_t gas_drop_warn_pm;     /* 기준 대비 감소율, 1/1000 */
  int32_t gas_drop_alarm_pm;
} thresholds_t;

/* config.h 의 기본값으로 초기화 (재부팅하면 기본값으로 돌아간다) */
void thresholds_init(void);

const thresholds_t *thresholds_get(void);

/*
 * key 에 해당하는 임계값을 value 로 변경. 범위/단계 순서 검증을 통과해야 적용된다.
 * 실패하면 false 와 이유(err)를 돌려준다. 한 번에 하나씩 바꾸므로 단계 순서를 어기지 않는 순서로 보낼 것.
 */
bool thresholds_set(const char *key, int32_t value, const char **err);

#endif /* THRESHOLDS_H */
