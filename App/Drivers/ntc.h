#ifndef NTC_H
#define NTC_H

/*
 * NTC 온도센서 드라이버 (PA1 분압 -> Beta 식).
 * config.h 의 APP_HAVE_NTC 가 1일 때만 호출한다. 분압 구성/B 값은 config.h 의 NTC_* 참고.
 */

#include <stdint.h>
#include <stdbool.h>

/* ADC raw -> 온도 (0.1 C 단위). 단선/단락 등 범위 밖이면 false. */
bool ntc_temp_x10_from_raw(uint16_t raw, uint16_t vdda_mv, int16_t *temp_x10);

#endif /* NTC_H */
