#ifndef DCIR_LOAD_H
#define DCIR_LOAD_H

#include <stdint.h>
#include <stdbool.h>

/*
 * DCIR 부하(MOSFET2) 펄스. TIM3(PA6, PWM2 원펄스)는 CubeMX 에서 설정한 값
 * (Prescaler, Period, Pulse)을 그대로 사용한다. 펄스 폭 = Period / 타이머 tick.
 */

/* 타이머 정지 상태 확인 및 시간 환산 값 계산. MX_TIM3_Init 이후 호출 */
void dcir_load_init(void);

/* CubeMX 설정으로 결정되는 펄스 폭 (ms) */
uint32_t dcir_load_pulse_ms(void);

/* us -> TIM3 tick */
uint32_t dcir_load_us_to_ticks(uint32_t us);

/* 부하 펄스 시작 (펄스 폭이 지나면 하드웨어가 자동 OFF). 카운터는 0 부터 시작 */
void dcir_load_pulse_start(void);

/* 펄스 진행 중인지 */
bool dcir_load_busy(void);

/* 펄스 시작 후 경과 tick */
uint32_t dcir_load_elapsed_ticks(void);

/* 즉시 OFF (중단/안전용) */
void dcir_load_off(void);

#endif /* DCIR_LOAD_H */
