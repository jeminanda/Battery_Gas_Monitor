#ifndef CHARGE_CTRL_H
#define CHARGE_CTRL_H

#include <stdint.h>
#include <stdbool.h>
#include "safety.h"

/* PB10 을 출력(High = 충전 허용)으로 설정 */
void charge_ctrl_init(void);

/* DCIR 측정 중 등 외부 요청으로 충전을 잠근다. */
void charge_ctrl_set_lock(bool lock);

/*
 * 안전 상태를 반영해 MOSFET1 을 갱신한다.
 * 과충전/과열/가스 경보일 때만 차단한다 (저전압 경보는 충전이 필요하므로 차단하지 않음).
 */
void charge_ctrl_update(uint32_t now_ms, const safety_result_t *s);

/* 현재 충전 허용 상태 */
bool charge_ctrl_is_enabled(void);

#endif /* CHARGE_CTRL_H */
