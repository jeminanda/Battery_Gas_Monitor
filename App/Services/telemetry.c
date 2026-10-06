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

size_t telemetry_format(char *buf, size_t size, const measurement_t *m, uint32_t seq)
{
  char bt[12], et[12], eh[12], ep[12], gas[16], mq[12];
  put_opt(bt, sizeof bt, m->batt_temp_valid, m->batt_temp_x10);
  put_opt(et, sizeof et, m->env.valid_mask & ENV_HAS_TEMP, m->env.temp_x10);
  put_opt(eh, sizeof eh, m->env.valid_mask & ENV_HAS_HUM, m->env.hum_x10);
  put_opt(ep, sizeof ep, m->env.valid_mask & ENV_HAS_PRESS, m->env.press_hpa_x10);
  put_opt(gas, sizeof gas, m->env.valid_mask & ENV_HAS_GAS, (long)m->env.gas_ohm);
  put_opt(mq, sizeof mq, m->mq_ready, m->mq_mv);

  int n = snprintf(buf, size, "$BAT,%d,%lu,%lu,%u,%u,%u,%s,%s,%s,%s,%s,%s,%u,%u",
                   APP_DEVICE_ID, (unsigned long)seq, (unsigned long)m->tick_ms,
                   m->vdda_mv, m->cell_mv, m->soc_pct,
                   bt, et, eh, ep, gas, mq,
                   (unsigned)m->safety.state, (unsigned)m->safety.flags);
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
  static uint32_t seq;
  char buf[PACKET_MAX];
  size_t len = telemetry_format(buf, sizeof buf, m, seq++);
  if (len > 0) {
    bt_link_send(buf, len);
  }
}
