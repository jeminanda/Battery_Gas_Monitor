#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  bool     valid;          /* INA219 가 응답 중이고 최근 읽기가 성공 */
  uint16_t vbus_mv;
  int32_t  current_ma;     /* 충전 +, 방전 - */
} power_data_t;

/* I2C 속도 설정 및 INA219 탐색 시작 */
void power_monitor_init(void);

/* 메인 루프에서 호출. 미검출이면 주기적으로 재탐색하므로 나중에 꽂아도 자동 인식 */
void power_monitor_task(uint32_t now_ms);

const power_data_t *power_monitor_get(void);

#endif /* POWER_MONITOR_H */
