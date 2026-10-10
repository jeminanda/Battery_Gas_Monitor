#ifndef CHARGE_STATUS_H
#define CHARGE_STATUS_H

#include <stdint.h>
#include <stdbool.h>

/* 충전 상태: INA219 전류(충전 +)와 셀 전압으로 추정한다 (TP4056 상태 핀은 사용하지 않음) */
#define CHARGE_STATE_NONE      0   /* 충전 전류 없음 (어댑터 미연결 또는 충전 안 함) */
#define CHARGE_STATE_CHARGING  1   /* 충전 전류가 흐르는 중 */
#define CHARGE_STATE_DONE      2   /* 충전이 끝나고 셀 전압이 높게 유지됨 (완충 추정) */

void charge_status_init(void);

/*
 * 충전 상태 갱신. CHG_DEBOUNCE_MS 동안 같은 판정이 이어져야 상태가 바뀐다.
 *  power_valid: INA219 응답 여부 (없으면 미연결(0)로 본다. 전류 유무는 current_ma 의 NA 로 구분),
 *  current_ma: 충전 +, 방전 -,  cell_mv: 셀 전압
 */
void charge_status_update(uint32_t now_ms, bool power_valid, int32_t current_ma, uint16_t cell_mv);

uint8_t charge_status_get(void);

#endif /* CHARGE_STATUS_H */
