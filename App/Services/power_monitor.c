/*
 * power_monitor.c -- 전류/버스 전압 주기 측정 (INA219)
 * 역할: INA219 를 PERIOD_POWER_MS 마다 읽어 전류(충전 +, 방전 -)와 버스 전압을 보관한다.
 *       응답이 없으면 주기적으로 다시 찾으므로 나중에 꽂아도 자동으로 인식한다.
 * 용도: app.c(텔레메트리), soc_estimator.c(전류 적분), dcir.c(시작 인터록)가 사용한다.
 */

#include "power_monitor.h"
#include "config.h"
#include "ina219.h"
#include "log.h"

#define MAX_READ_FAILS 3

static power_data_t data;
static uint32_t     last_ms;
static uint8_t      fail_cnt;

static void set_present(bool present)
{
  if (present != data.valid) {
    LOG("INA219 %s", present ? "detected" : "lost");
  }
  data.valid = present;
  fail_cnt = 0;
}

void power_monitor_init(void)
{
  ina219_bus_init();
  set_present(ina219_probe());
}

void power_monitor_task(uint32_t now_ms)
{
  if (!data.valid) {
    if ((now_ms - last_ms) >= PERIOD_INA_REPROBE_MS) {
      last_ms = now_ms;
      set_present(ina219_probe());
    }
    return;
  }

  if ((now_ms - last_ms) < PERIOD_POWER_MS) {
    return;
  }
  last_ms = now_ms;

  int32_t ua;
  uint16_t mv;
  if (ina219_read_current_ua(&ua) && ina219_read_bus_mv(&mv)) {
    data.current_ma = (ua >= 0) ? (ua + 500) / 1000 : (ua - 500) / 1000;
    data.vbus_mv = mv;
    fail_cnt = 0;
  } else if (++fail_cnt >= MAX_READ_FAILS) {
    set_present(false);
  }
}

const power_data_t *power_monitor_get(void)
{
  return &data;
}
