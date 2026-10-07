#ifndef CHARGE_STATUS_H
#define CHARGE_STATUS_H

#include <stdint.h>

/* TP4056 CHRG/STDBY 조합 */
#define CHARGE_STATE_NONE      0   /* 미연결 (둘 다 High) */
#define CHARGE_STATE_CHARGING  1   /* CHRG Low */
#define CHARGE_STATE_DONE      2   /* STDBY Low (완충) */
#define CHARGE_STATE_FAULT     3   /* 둘 다 Low (비정상) */

void charge_status_init(void);

/* 메인 루프에서 호출. CHG_DEBOUNCE_MS 동안 같은 값이어야 상태가 바뀐다. */
void charge_status_task(uint32_t now_ms);

uint8_t charge_status_get(void);

#endif /* CHARGE_STATUS_H */
