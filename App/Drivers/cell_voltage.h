#ifndef CELL_VOLTAGE_H
#define CELL_VOLTAGE_H

#include <stdint.h>

/* ADC raw(PA4) -> 셀 전압 (mV). 실제 VDDA 와 분압비를 반영한다. */
uint16_t cell_voltage_mv_from_raw(uint16_t raw, uint16_t vdda_mv);

/* raw n 개의 합 -> 셀 전압 (uV). DCIR 처럼 mV 보다 정밀한 평균이 필요할 때 사용 */
int32_t cell_voltage_uv_from_raw_sum(uint32_t raw_sum, uint32_t n, uint16_t vdda_mv);

#endif /* CELL_VOLTAGE_H */
