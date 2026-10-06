#ifndef NTC_H
#define NTC_H

/*
 * NTC 온도센서 드라이버 -- 헤더만 존재 (부품 미도착, 구현 없음).
 * config.h 의 APP_HAVE_NTC 가 1일 때만 호출해야 하며, ntc.c 를 추가하고
 * CMakeLists.txt 의 소스 목록에 등록한 뒤 사용한다.
 */

#include <stdint.h>
#include <stdbool.h>

/* ADC raw -> 온도 (0.1 C 단위). 단선/단락 등 범위 밖이면 false. */
bool ntc_temp_x10_from_raw(uint16_t raw, uint16_t vdda_mv, int16_t *temp_x10);

#endif /* NTC_H */
