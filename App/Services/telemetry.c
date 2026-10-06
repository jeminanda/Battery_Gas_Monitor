#include "telemetry.h"
#include "config.h"
#include "bt_link.h"
#include <stdio.h>

#define PACKET_MAX 160

/* 유효하면 숫자, 아니면 "NA" 를 기록 */
static int put_opt(char *dst, size_t size, bool valid, long value)
{
  return valid ? snprintf(dst, size, "%ld", value) : snprintf(dst, size, "NA");
}

size_t telemetry_format(char *buf, size_t size, const measurement_t *m, uint32_t packet_index)
{
  char cell_temp[12], air_temp[12], air_humi[12], air_pres[12], gas[16];
  put_opt(cell_temp, sizeof cell_temp, m->batt_temp_valid, m->batt_temp_x10);
  put_opt(air_temp, sizeof air_temp, m->env.valid_mask & ENV_HAS_TEMP, m->env.temp_x10);
  put_opt(air_humi, sizeof air_humi, m->env.valid_mask & ENV_HAS_HUM, m->env.hum_x10);
  put_opt(air_pres, sizeof air_pres, m->env.valid_mask & ENV_HAS_PRESS, m->env.press_hpa_x10);

  /* gas: BME680 가스 저항(ohm) 우선, 없으면 MQ 전압(mV) */
  if (m->env.valid_mask & ENV_HAS_GAS) {
    put_opt(gas, sizeof gas, true, (long)m->env.gas_ohm);
  } else {
    put_opt(gas, sizeof gas, m->mq_ready, m->mq_mv);
  }

  int n = snprintf(buf, size, "$BAT,%d,%lu,%lu,%u,%s,%s,%s,%s,%s",
                   APP_DEVICE_ID, (unsigned long)packet_index, (unsigned long)m->tick_ms,
                   m->cell_mv, cell_temp, air_temp, air_humi, air_pres, gas);
  if (n < 0 || (size_t)n + 6 > size) {   /* "*CC\n" + NUL 여유 */
    return 0;
  }

  uint8_t cs = 0;
  for (int i = 1; i < n; i++) {
    cs ^= (uint8_t)buf[i];
  }
  n += snprintf(buf + n, size - (size_t)n, "*%02X\n", cs);
  return (size_t)n;
}

void telemetry_send(const measurement_t *m)
{
  static uint32_t packet_index;
  char buf[PACKET_MAX];
  size_t len = telemetry_format(buf, sizeof buf, m, packet_index++);
  if (len > 0) {
    bt_link_send(buf, len);
  }
}
