#include "adc_sampler.h"
#include "config.h"
#include "main.h"
#include "adc.h"

static uint16_t dma_buf[ADC_IDX_COUNT];
static uint32_t acc[ADC_IDX_COUNT];
static uint16_t avg[ADC_IDX_COUNT];
static uint16_t vdda_mv = VDDA_NOMINAL_MV;
static uint8_t  acc_count;
static bool     ready;
static bool     running;
static uint32_t last_start_ms;
static volatile bool conv_done;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == &hadc1) {
    conv_done = true;
  }
}

static void start_conversion(void)
{
  HAL_ADC_Stop_DMA(&hadc1);
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)dma_buf, ADC_IDX_COUNT) == HAL_OK) {
    running = true;
  }
}

static void publish_average(void)
{
  for (int i = 0; i < ADC_IDX_COUNT; i++) {
    avg[i] = (uint16_t)(acc[i] / ADC_AVG_SAMPLES);
    acc[i] = 0;
  }
  acc_count = 0;

  if (avg[ADC_IDX_VREF] != 0) {
    vdda_mv = (uint16_t)(((uint32_t)VREFINT_CAL_VDDA_MV * *VREFINT_CAL_ADDR)
                         / avg[ADC_IDX_VREF]);
  }
  ready = true;
}

void adc_sampler_init(void)
{
  conv_done = false;
  start_conversion();
}

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

  /* DMAContinuousRequests=DISABLE 이므로 시퀀스마다 변환을 다시 시작해야 한다. */
  if (!running && (now_ms - last_start_ms) >= PERIOD_ADC_SAMPLE_MS) {
    last_start_ms = now_ms;
    start_conversion();
  }
}

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
