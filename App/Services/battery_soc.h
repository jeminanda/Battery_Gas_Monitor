#ifndef BATTERY_SOC_H
#define BATTERY_SOC_H

#include <stdint.h>

/* 셀 전압(mV) -> 잔량(%). Li-ion 1셀 개방전압 근사 테이블. */
uint8_t battery_soc_pct_from_mv(uint16_t cell_mv);

#endif /* BATTERY_SOC_H */
