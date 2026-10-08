/*
 * adc_sampler.c -- ADC1 스캔 샘플러 (DMA)
 * 역할: NTC(PA1), 셀 전압(PA4), VREFINT 를 DMA 로 읽어 ADC_AVG_SAMPLES 번 평균한다.
 *       VREFINT 로 실제 VDDA 를 계산해 mV 환산 시 공급 전압 변동을 보정한다.
 * 용도: app.c 가 평균 raw 값과 VDDA 를 가져가 전압/온도로 환산한다. dcir.c 는 즉시 1회 변환을 쓴다.
 * 흐름: 일정 주기로 변환 시작 -> DMA 완료 콜백이 conv_done 설정 -> task 가 누적 -> 평균 공개.
 */

#include "adc_sampler.h"
#include "config.h"
#include "main.h"
#include "adc.h"

static uint16_t dma_buf[ADC_IDX_COUNT];   /* DMA 가 채우는 최신 1회 변환 결과 */
static uint32_t acc[ADC_IDX_COUNT];       /* 평균용 누적 합 */
static uint16_t avg[ADC_IDX_COUNT];       /* 공개되는 평균 raw 값 */
static uint16_t vdda_mv = VDDA_NOMINAL_MV;
static uint8_t  acc_count;                /* 지금까지 누적한 횟수 */
static bool     ready;                    /* 첫 평균이 나왔는지 */
static bool     running;                  /* 변환이 진행 중인지 */
static uint32_t last_start_ms;
static volatile bool conv_done;           /* ISR 에서 설정, task 에서 해제 */

/* DMA 전송 완료(ISR): 메인 루프가 처리하도록 플래그만 세운다 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == &hadc1) {
    conv_done = true;
  }
}

/* 스캔 변환 1회 시작. DMAContinuousRequests=DISABLE 이므로 매번 다시 시작해야 한다 */
static void start_conversion(void)
{
  HAL_ADC_Stop_DMA(&hadc1);
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)dma_buf, ADC_IDX_COUNT) == HAL_OK) {
    running = true;
  }
}

/* 누적 합을 평균으로 확정하고 VREFINT 로 VDDA 를 갱신한다 */
static void publish_average(void)
{
  for (int i = 0; i < ADC_IDX_COUNT; i++) {
    avg[i] = (uint16_t)(acc[i] / ADC_AVG_SAMPLES);
    acc[i] = 0;
  }
  acc_count = 0;

  /* VDDA = (공장 보정 시 VDDA) * (보정 시 VREFINT raw) / (현재 VREFINT raw) */
  if (avg[ADC_IDX_VREF] != 0) {
    vdda_mv = (uint16_t)(((uint32_t)VREFINT_CAL_VDDA_MV * *APP_VREFINT_CAL_ADDR)
                         / avg[ADC_IDX_VREF]);
  }
  ready = true;
}

/* 첫 변환 시작. 이후는 adc_sampler_task 가 주기적으로 이어간다 */
void adc_sampler_init(void)
{
  conv_done = false;
  start_conversion();
}

/* 메인 루프에서 호출: 끝난 변환을 누적하고, 주기가 되면 다음 변환을 시작 */
void adc_sampler_task(uint32_t now_ms)
{
  if (conv_done) {
    conv_done = false;
    running = false;
    for (int i = 0; i < ADC_IDX_COUNT; i++) {
      acc[i] += dma_buf[i];
    }
    if (++acc_count >= ADC_AVG_SAMPLES) {
      publish_average();
    }
  }

  /* 변환 중이 아니고 PERIOD_ADC_SAMPLE_MS 가 지났으면 다음 변환 시작 */
  if (!running && (now_ms - last_start_ms) >= PERIOD_ADC_SAMPLE_MS) {
    last_start_ms = now_ms;
    start_conversion();
  }
}

/* DCIR 용 즉시 1회 변환(블로킹). 평균 없이 raw 를 out 에 그대로 복사한다 */
bool adc_sampler_read_once(uint16_t out[ADC_IDX_COUNT], uint32_t timeout_ms)
{
  HAL_ADC_Stop_DMA(&hadc1);
  conv_done = false;
  running = false;
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)dma_buf, ADC_IDX_COUNT) != HAL_OK) {
    return false;
  }
  uint32_t t0 = HAL_GetTick();
  while (!conv_done) {
    if ((HAL_GetTick() - t0) > timeout_ms) {
      HAL_ADC_Stop_DMA(&hadc1);
      return false;
    }
  }
  conv_done = false;
  for (int i = 0; i < ADC_IDX_COUNT; i++) {
    out[i] = dma_buf[i];
  }
  return true;
}

bool adc_sampler_ready(void)
{
  return ready;
}

uint16_t adc_sampler_get_raw(adc_idx_t idx)
{
  return (idx < ADC_IDX_COUNT) ? avg[idx] : 0;
}

uint16_t adc_sampler_get_vdda_mv(void)
{
  return vdda_mv;
}
