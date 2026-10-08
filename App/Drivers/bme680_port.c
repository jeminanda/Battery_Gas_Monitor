/*
 * bme680_port.c -- BME680 포팅 계층
 * 역할: Bosch BME68x 공식 드라이버(bme68x/)의 read/write/delay 콜백을 STM32 HAL I2C1 에 연결하고,
 *       forced mode 측정을 시작(trigger)/완료 확인(poll)하는 비동기 인터페이스를 제공한다.
 * 용도: env_sensor.c 가 주기마다 trigger -> poll 로 온도/습도/기압/가스 저항을 읽는다.
 *       센서가 없으면 trigger 때마다 재탐색하므로 나중에 꽂아도 자동으로 인식된다.
 */

#include "bme680_port.h"
#include "bme68x.h"
#include "config.h"
#include "log.h"
#include <math.h>

#define I2C_TIMEOUT_MS 20

static I2C_HandleTypeDef *hi2c;
static uint8_t            addr7;
static struct bme68x_dev  dev;
static struct bme68x_conf conf;
static struct bme68x_heatr_conf heatr;
static bool               ready;
static bool               init_reported;
static uint32_t           trigger_ms;
static uint32_t           meas_ms;

static BME68X_INTF_RET_TYPE port_read(uint8_t reg, uint8_t *data, uint32_t len, void *intf)
{
  (void)intf;
  return (HAL_I2C_Mem_Read(hi2c, (uint16_t)(addr7 << 1), reg, I2C_MEMADD_SIZE_8BIT,
                           data, (uint16_t)len, I2C_TIMEOUT_MS) == HAL_OK)
             ? BME68X_INTF_RET_SUCCESS : 1;
}

static BME68X_INTF_RET_TYPE port_write(uint8_t reg, const uint8_t *data, uint32_t len, void *intf)
{
  (void)intf;
  return (HAL_I2C_Mem_Write(hi2c, (uint16_t)(addr7 << 1), reg, I2C_MEMADD_SIZE_8BIT,
                            (uint8_t *)data, (uint16_t)len, I2C_TIMEOUT_MS) == HAL_OK)
             ? BME68X_INTF_RET_SUCCESS : 1;
}

/* 드라이버가 요구하는 us 지연 (측정 자체는 비동기로 대기하므로 짧은 호출만 들어온다) */
static void port_delay_us(uint32_t period, void *intf)
{
  (void)intf;
  if (period >= 1000U) {
    HAL_Delay((period + 999U) / 1000U);
    return;
  }
  uint32_t ticks = period * (SystemCoreClock / 1000000U);
  uint32_t start = DWT->CYCCNT;
  while ((DWT->CYCCNT - start) < ticks) { }
}

static bool try_init(void)
{
  dev.intf = BME68X_I2C_INTF;
  dev.read = port_read;
  dev.write = port_write;
  dev.delay_us = port_delay_us;
  dev.intf_ptr = NULL;
  dev.amb_temp = 25;

  int8_t rc = bme68x_init(&dev);
  if (rc != BME68X_OK) {
    if (!init_reported) {
      LOG("BME680 not detected (rc=%d, addr=0x%02X): check I2C wiring/address", rc, addr7);
      init_reported = true;
    }
    return false;
  }

  conf.os_hum = BME68X_OS_2X;
  conf.os_pres = BME68X_OS_4X;
  conf.os_temp = BME68X_OS_8X;
  conf.filter = BME68X_FILTER_OFF;
  conf.odr = BME68X_ODR_NONE;
  heatr.enable = BME68X_ENABLE;
  heatr.heatr_temp = BME680_HEATER_TEMP_C;
  heatr.heatr_dur = BME680_HEATER_DUR_MS;

  if (bme68x_set_conf(&conf, &dev) != BME68X_OK ||
      bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr, &dev) != BME68X_OK) {
    LOG("BME680 config failed");
    return false;
  }

  meas_ms = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &dev) / 1000U + heatr.heatr_dur + 1U;
  LOG("BME680 detected (chip_id=0x%02X, addr=0x%02X)", dev.chip_id, addr7);
  init_reported = false;   /* 이후 분리되면 다시 한 번 알린다 */
  return true;
}

bool bme680_port_init(I2C_HandleTypeDef *h, uint8_t addr_7bit)
{
  hi2c = h;
  addr7 = addr_7bit;
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  ready = try_init();
  return ready;
}

bool bme680_port_trigger(void)
{
  if (!ready) {
    ready = try_init();          /* 나중에 꽂아도 자동으로 인식 */
    if (!ready) {
      return false;
    }
  }
  if (bme68x_set_op_mode(BME68X_FORCED_MODE, &dev) != BME68X_OK) {
    ready = false;               /* 통신 오류: 다음 주기에 재초기화 시도 */
    return false;
  }
  trigger_ms = HAL_GetTick();
  return true;
}

bool bme680_port_poll(env_data_t *out)
{
  if ((HAL_GetTick() - trigger_ms) < meas_ms) {
    return false;
  }

  struct bme68x_data d;
  uint8_t n = 0;
  if (bme68x_get_data(BME68X_FORCED_MODE, &d, &n, &dev) != BME68X_OK || n == 0 ||
      !(d.status & BME68X_NEW_DATA_MSK)) {
    return false;
  }

  out->temp_x10 = (int16_t)lroundf(d.temperature * 10.0f);
  out->hum_x10 = (uint16_t)lroundf(d.humidity * 10.0f);
  out->press_hpa_x10 = (uint16_t)lroundf(d.pressure / 10.0f);   /* Pa -> 0.1 hPa */
  out->valid_mask = ENV_HAS_TEMP | ENV_HAS_HUM | ENV_HAS_PRESS;
  out->gas_ohm = 0;
  if ((d.status & BME68X_GASM_VALID_MSK) && (d.status & BME68X_HEAT_STAB_MSK)) {
    out->gas_ohm = (uint32_t)d.gas_resistance;
    out->valid_mask |= ENV_HAS_GAS;
  }
  return true;
}
