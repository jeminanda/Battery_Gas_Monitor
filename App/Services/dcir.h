#ifndef DCIR_H
#define DCIR_H

#include <stdint.h>
#include <stdbool.h>

/* 측정 1세트(반복 DCIR_REPEAT_N 회)의 결과. v/i 는 반복 평균, dcir 은 반복 평균 및 표준편차 */
typedef struct {
  uint32_t tick_ms;
  int32_t  v0_uv, v1_uv;          /* 부하 전/후 셀 전압 */
  int32_t  i0_ua, i1_ua;          /* 부하 전/후 전류 (충전 +, 방전 -) */
  int32_t  dcir_uohm;
  int32_t  dcir_std_uohm;
  uint8_t  repeat_n;              /* 유효한 반복 횟수 */
  uint16_t vcell_mv;              /* 측정 당시 셀 전압 */
  bool     temp_valid;
  int32_t  temp_mc;               /* 측정 당시 온도 (0.001 C) */
} dcir_result_t;

/* 인터록 판정에 필요한 현재 상태 (메인 루프가 채워서 전달) */
typedef struct {
  bool     ready;                 /* 측정값(measurement)이 유효한지 */
  uint16_t cell_mv;
  uint8_t  charge_state;
  bool     power_valid;           /* INA219 응답 */
  int32_t  current_ma;            /* 현재 전류 (충전 +, 방전 -). 안정(휴지) 상태 확인용 */
  bool     alarm;                 /* 안전 경보 상태 (경보 중에는 부하를 걸지 않음) */
  bool     temp_valid;
  int16_t  temp_x10;
} dcir_input_t;

void dcir_init(void);

/* 측정 시작 요청. 이미 진행 중이면 false. 인터록 실패 시 시작되지 않고 이유가 LOG 로 출력된다. */
bool dcir_request(void);

/* 진행 중인 측정을 중단하고 부하 OFF */
void dcir_abort(const char *reason);

bool dcir_is_active(void);

/* 메인 루프에서 호출 (측정 한 번 동안만 약 0.2초 블로킹) */
void dcir_task(uint32_t now_ms, const dcir_input_t *in);

#endif /* DCIR_H */
