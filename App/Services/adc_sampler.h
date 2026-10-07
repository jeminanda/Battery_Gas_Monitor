#ifndef ADC_SAMPLER_H
#define ADC_SAMPLER_H

#include <stdint.h>
#include <stdbool.h>

/* CubeMX 에서 설정한 ADC1 rank 순서와 일치해야 한다. */
typedef enum {
  ADC_IDX_MQ = 0,   /* PA0  */
  ADC_IDX_NTC,      /* PA1  (NTC 미도착: 값은 읽히지만 사용하지 않음) */
  ADC_IDX_CELL,     /* PA4  */
  ADC_IDX_VREF,     /* VREFINT */
  ADC_IDX_COUNT
} adc_idx_t;

void adc_sampler_init(void);

/* 메인 루프에서 호출: DMA 완료 처리, 평균, 다음 변환 시작 */
void adc_sampler_task(uint32_t now_ms);

/*
 * 즉시 1회 변환(블로킹, 약 0.1ms). 평균 없이 raw 를 그대로 반환한다.
 * DCIR 처럼 빠른 연속 샘플링이 필요할 때 사용하며, 진행 중이던 일반 변환은 중단한다.
 */
bool adc_sampler_read_once(uint16_t out[ADC_IDX_COUNT], uint32_t timeout_ms);

/* 첫 평균값이 준비되었는지 */
bool adc_sampler_ready(void);

/* 평균된 raw 값 */
uint16_t adc_sampler_get_raw(adc_idx_t idx);

/* VREFINT 로 계산한 실제 VDDA (mV) */
uint16_t adc_sampler_get_vdda_mv(void);

#endif /* ADC_SAMPLER_H */
