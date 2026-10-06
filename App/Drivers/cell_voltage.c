#include "cell_voltage.h"
#include "config.h"

uint16_t cell_voltage_mv_from_raw(uint16_t raw, uint16_t vdda_mv)
{
  uint32_t pin_mv = ((uint32_t)raw * vdda_mv) / ADC_FULL_SCALE;
  uint32_t cell_mv = (pin_mv * (CELL_DIV_R_TOP_OHM + CELL_DIV_R_BOTTOM_OHM))
                     / CELL_DIV_R_BOTTOM_OHM;
  return (cell_mv > UINT16_MAX) ? UINT16_MAX : (uint16_t)cell_mv;
}
