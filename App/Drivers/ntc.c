/*
 * ntc.c -- NTC 서미스터 온도 계산
 * 역할: 분압 회로(PA1)의 ADC raw 값을 Beta 식으로 온도(0.1 C)로 환산하고, 단선/단락을 판정한다.
 * 용도: app.c 의 measure_update 가 배터리 온도를 구해 안전 판정(과열)과 텔레메트리에 사용한다.
 */

#include "ntc.h"
#include "config.h"
#include <math.h>

/* 비율형 측정이라 VDDA 에 의존하지 않는다 (vdda_mv 는 사용하지 않음) */
bool ntc_temp_x10_from_raw(uint16_t raw, uint16_t vdda_mv, int16_t *temp_x10)
{
  (void)vdda_mv;

  /* 단선(NTC 분리) / 단락 판정: 분압 중간점이 전원 또는 GND 에 붙은 값 */
  if (raw < NTC_RAW_MIN || raw > NTC_RAW_MAX) {
    return false;
  }

  float ratio = (float)raw / (float)ADC_FULL_SCALE;
#if NTC_ON_LOW_SIDE
  float r_ntc = (float)NTC_R_FIXED_OHM * ratio / (1.0f - ratio);
#else
  float r_ntc = (float)NTC_R_FIXED_OHM * (1.0f - ratio) / ratio;
#endif

  /* Beta 식: 1/T = 1/T0 + ln(R/R0)/B */
  float inv_t = 1.0f / (NTC_T0_C + 273.15f) + logf(r_ntc / (float)NTC_R0_OHM) / (float)NTC_BETA;
  float temp_c = 1.0f / inv_t - 273.15f;

  if (temp_c < NTC_TEMP_MIN_C || temp_c > NTC_TEMP_MAX_C) {
    return false;
  }
  *temp_x10 = (int16_t)lroundf(temp_c * 10.0f);
  return true;
}
