#include "mq_gas.h"
#include "config.h"

uint16_t mq_gas_mv_from_raw(uint16_t raw, uint16_t vdda_mv)
{
  return (uint16_t)(((uint32_t)raw * vdda_mv) / ADC_FULL_SCALE);
}

bool mq_gas_is_ready(uint32_t now_ms)
{
  return now_ms >= MQ_WARMUP_MS;
}
