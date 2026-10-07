#ifndef TP4056_H
#define TP4056_H

#include <stdbool.h>

/* PB5(CHRG), PB4(STDBY) 를 내부 풀업 입력으로 설정 */
void tp4056_init(void);

/* open-drain 출력의 현재 핀 상태. Low 가 활성(true) */
bool tp4056_chrg_active(void);
bool tp4056_stdby_active(void);

#endif /* TP4056_H */
