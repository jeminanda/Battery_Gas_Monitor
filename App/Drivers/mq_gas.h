#ifndef MQ_GAS_H
#define MQ_GAS_H

#include <stdint.h>
#include <stdbool.h>

/* ADC raw(PA0) -> 센서 출력 전압 (mV). 농도 환산은 실측 보정 후 추가. */
uint16_t mq_gas_mv_from_raw(uint16_t raw, uint16_t vdda_mv);

/* 히터 워밍업 완료 여부 (부팅 후 MQ_WARMUP_MS 경과) */
bool mq_gas_is_ready(uint32_t now_ms);

#endif /* MQ_GAS_H */
