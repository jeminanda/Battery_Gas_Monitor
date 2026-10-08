#include "env_sensor.h"
#include "config.h"
#include "log.h"
#include "bme680_port.h"
#include "i2c.h"

#define ENV_MAX_FAILS 3   /* 연속 실패가 이 횟수에 도달하면 값을 무효 처리 */

static env_data_t data;
static uint32_t   last_ms;
static uint8_t    fail_cnt;
static bool       pending;   /* BME680 측정 진행 중 */

/* BME680 초기화. 실패해도 task 가 주기적으로 재탐색하므로 나중에 꽂아도 인식된다 */
bool env_sensor_init(void)
{
  return bme680_port_init(&hi2c1, BME680_I2C_ADDR_7BIT);
}

/* 값이 나오기 시작/끊긴 시점만 로그로 알린다 */
static void report_state(void)
{
  static bool last_ok;
  bool ok = (data.valid_mask != 0);
  if (ok != last_ok) {
    last_ok = ok;
    if (ok) {
      LOG("env sensor OK (mask=0x%X)", data.valid_mask);
    } else {
      LOG("env sensor no data (BME680)");
    }
  }
}

static void on_failure(void)
{
  if (++fail_cnt >= ENV_MAX_FAILS) {
    fail_cnt = ENV_MAX_FAILS;
    data.valid_mask = 0;
  }
  report_state();
}

/*
 * 비동기 측정: 주기마다 측정을 "시작(trigger)"하고, 다음 호출들에서 완료를 "확인(poll)"한다.
 * 완료되면 data 를 갱신하고 sample_id 를 올린다 (gas_monitor 가 새 샘플인지 판별하는 용도).
 */
void env_sensor_task(uint32_t now_ms)
{
  if (pending) {
    env_data_t out;
    if (bme680_port_poll(&out)) {
      uint32_t id = data.sample_id + 1U;
      data = out;
      data.sample_id = id;
      fail_cnt = 0;
      pending = false;
      report_state();
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
}

const env_data_t *env_sensor_get(void)
{
  return &data;
}
