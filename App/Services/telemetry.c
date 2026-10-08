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

/* "$...": 체크섬(XOR)과 "*CC\n" 을 붙이고 전체 길이를 반환 (공간 부족 시 0) */
static size_t finish_packet(char *buf, size_t size, int n)
{
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

/* $BAT 패킷 문자열 생성. 필드 순서/의미는 telemetry.h 주석 참고 */
size_t telemetry_format(char *buf, size_t size, const measurement_t *m, uint32_t packet_index)
{
  char cell_temp[12], air_temp[12], air_humi[12], air_pres[12], gas[16];
  put_opt(cell_temp, sizeof cell_temp, m->batt_temp_valid, m->batt_temp_x10);
  put_opt(air_temp, sizeof air_temp, m->env.valid_mask & ENV_HAS_TEMP, m->env.temp_x10);
  put_opt(air_humi, sizeof air_humi, m->env.valid_mask & ENV_HAS_HUM, m->env.hum_x10);
  put_opt(air_pres, sizeof air_pres, m->env.valid_mask & ENV_HAS_PRESS, m->env.press_hpa_x10);

  /* gas: BME680 가스 저항(ohm) */
  put_opt(gas, sizeof gas, m->env.valid_mask & ENV_HAS_GAS, (long)m->env.gas_ohm);

  char vbus[12], cur[12], soc[12];
  put_opt(vbus, sizeof vbus, m->power_valid, m->vbus_mv);
  put_opt(cur, sizeof cur, m->power_valid, m->current_ma);
  put_opt(soc, sizeof soc, m->soc_valid, m->soc_permil);

  int n = snprintf(buf, size, "$BAT,%d,%lu,%lu,%u,%s,%s,%s,%s,%s,%s,%s,%u,%s,%u",
                   APP_DEVICE_ID, (unsigned long)packet_index, (unsigned long)m->tick_ms,
                   m->cell_mv, cell_temp, air_temp, air_humi, air_pres, gas,
                   vbus, cur, (unsigned)m->charge_state, soc, (unsigned)m->charge_enabled);
  return finish_packet(buf, size, n);
}

/* $BAT 패킷을 만들어 블루투스 송신 큐에 넣는다 (packet_index 는 보낼 때마다 증가) */
void telemetry_send(const measurement_t *m)
{
  static uint32_t packet_index;
  char buf[PACKET_MAX];
  size_t len = telemetry_format(buf, sizeof buf, m, packet_index++);
  if (len > 0) {
    bt_link_send(buf, len);
  }
}

/* $DCIR 패킷(내부 저항 측정 결과) 문자열 생성 */
size_t telemetry_format_dcir(char *buf, size_t size, const dcir_result_t *r)
{
  char temp[16];
  put_opt(temp, sizeof temp, r->temp_valid, r->temp_mc);

  int n = snprintf(buf, size, "$DCIR,%d,%lu,%ld,%ld,%ld,%ld,%ld,%ld,%u,%u,%s",
                   APP_DEVICE_ID, (unsigned long)r->tick_ms,
                   (long)r->v0_uv, (long)r->v1_uv, (long)r->i0_ua, (long)r->i1_ua,
                   (long)r->dcir_uohm, (long)r->dcir_std_uohm,
                   (unsigned)r->repeat_n, (unsigned)r->vcell_mv, temp);
  return finish_packet(buf, size, n);
}

/* DCIR 결과를 블루투스로 송신 (측정 1세트가 끝날 때 한 번) */
void telemetry_send_dcir(const dcir_result_t *r)
{
  char buf[PACKET_MAX];
  size_t len = telemetry_format_dcir(buf, sizeof buf, r);
  if (len > 0) {
    bt_link_send(buf, len);
  }
}
