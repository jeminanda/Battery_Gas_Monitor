#include "cell_voltage.h"
#include "config.h"

/*
 * ADC raw -> 핀 전압(mV) -> 분압비를 되돌려 셀 전압(mV).
 *   핀 전압 = raw * VDDA / 4095
 *   셀 전압 = 핀 전압 * (R_top + R_bottom) / R_bottom
 */
uint16_t cell_voltage_mv_from_raw(uint16_t raw, uint16_t vdda_mv)
{
  uint32_t pin_mv = ((uint32_t)raw * vdda_mv) / ADC_FULL_SCALE;
  uint32_t cell_mv = (pin_mv * (CELL_DIV_R_TOP_OHM + CELL_DIV_R_BOTTOM_OHM))
                     / CELL_DIV_R_BOTTOM_OHM;
  return (cell_mv > UINT16_MAX) ? UINT16_MAX : (uint16_t)cell_mv;
}

/* 같은 환산을 raw n 개의 합에 적용해 uV 로 반환 (평균을 정밀하게 얻기 위함) */
int32_t cell_voltage_uv_from_raw_sum(uint32_t raw_sum, uint32_t n, uint16_t vdda_mv)
{
  /* 64bit 오버플로 방지를 위해 핀 전압(uV)을 먼저 구한 뒤 분압비를 곱한다 */
  uint64_t pin_uv = (uint64_t)raw_sum * vdda_mv * 1000ULL / ((uint64_t)n * ADC_FULL_SCALE);
  return (int32_t)(pin_uv * (CELL_DIV_R_TOP_OHM + CELL_DIV_R_BOTTOM_OHM) / CELL_DIV_R_BOTTOM_OHM);
}
