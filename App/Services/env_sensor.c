#include "env_sensor.h"
#include "config.h"

#if ENV_BACKEND_BME680
#include "bme680_port.h"
#include "i2c.h"
#else
#include "dht11.h"
#endif

#define ENV_MAX_FAILS 3   /* 연속 실패가 이 횟수에 도달하면 값을 무효 처리 */

static env_data_t data;
static uint32_t   last_ms;
static uint8_t    fail_cnt;
static bool       pending;   /* BME680 측정 진행 중 */

bool env_sensor_init(void)
{
#if ENV_BACKEND_BME680
  return bme680_port_init(&hi2c1, BME680_I2C_ADDR_7BIT);
#else
  dht11_init();
  return true;
#endif
}

static void on_failure(void)
{
  if (++fail_cnt >= ENV_MAX_FAILS) {
    fail_cnt = ENV_MAX_FAILS;
    data.valid_mask = 0;
  }
}

void env_sensor_task(uint32_t now_ms)
{
#if ENV_BACKEND_BME680
  if (pending) {
    env_data_t out;
    if (bme680_port_poll(&out)) {
      data = out;
      fail_cnt = 0;
      pending = false;
    } else if ((now_ms - last_ms) > 1000) {   /* 측정 타임아웃 */
      pending = false;
      on_failure();
    }
    return;
  }
  if ((now_ms - last_ms) >= PERIOD_ENV_MS) {
    last_ms = now_ms;
    if (bme680_port_trigger()) {
      pending = true;
    } else {
      on_failure();
    }
  }
#else
  (void)pending;
  if ((now_ms - last_ms) < PERIOD_ENV_MS) {
    return;
  }
  last_ms = now_ms;

  int16_t t;
  uint16_t h;
  if (dht11_read(&t, &h) == DHT11_OK) {
    data.temp_x10 = t;
    data.hum_x10 = h;
    data.valid_mask = ENV_HAS_TEMP | ENV_HAS_HUM;
    fail_cnt = 0;
  } else {
    on_failure();
  }
#endif
}

const env_data_t *env_sensor_get(void)
{
  return &data;
}
