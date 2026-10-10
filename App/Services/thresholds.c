/*
 * thresholds.c -- 런타임 안전 임계값
 * 역할: 저전압/고전압/과열/가스 감소율 임계값 8개를 RAM 에 보관하고, 변경 요청을 범위와 단계 순서로
 *       검증해서 안전한 값만 적용한다. 기본값은 config.h 이고 재부팅하면 기본값으로 돌아간다.
 * 용도: command.c 가 서버 명령으로 값을 바꾸고, safety.c 와 gas_monitor.c 가 판정 때 읽는다.
 */

#include "thresholds.h"
#include "config.h"
#include "log.h"
#include <string.h>

static thresholds_t th;

/* 키 이름 -> 값 위치와 허용 범위 */
typedef struct {
  const char *key;
  int32_t    *value;
  int32_t     min;
  int32_t     max;
} entry_t;

static const entry_t table[] = {
  { "cell_low_alarm_mv",  &th.cell_low_alarm_mv,  2500, 3600 },
  { "cell_low_warn_mv",   &th.cell_low_warn_mv,   2500, 3600 },
  { "cell_high_warn_mv",  &th.cell_high_warn_mv,  4000, 4400 },
  { "cell_high_alarm_mv", &th.cell_high_alarm_mv, 4000, 4400 },
  { "temp_warn_x10",      &th.temp_warn_x10,       300,  800 },
  { "temp_alarm_x10",     &th.temp_alarm_x10,      300,  800 },
  /* 하한 100: 단계를 내릴 때 GAS_DROP_HYST_PM 만큼 빼므로 그보다 커야 한다 */
  { "gas_drop_warn_pm",   &th.gas_drop_warn_pm,    100,  900 },
  { "gas_drop_alarm_pm",  &th.gas_drop_alarm_pm,   100,  900 },
};
#define TABLE_LEN (sizeof(table) / sizeof(table[0]))

void thresholds_init(void)
{
  th.cell_low_alarm_mv = SAFE_CELL_LOW_ALARM_MV;
  th.cell_low_warn_mv = SAFE_CELL_LOW_WARN_MV;
  th.cell_high_warn_mv = SAFE_CELL_HIGH_WARN_MV;
  th.cell_high_alarm_mv = SAFE_CELL_HIGH_ALARM_MV;
  th.temp_warn_x10 = SAFE_TEMP_WARN_X10;
  th.temp_alarm_x10 = SAFE_TEMP_ALARM_X10;
  th.gas_drop_warn_pm = GAS_DROP_WARN_PM;
  th.gas_drop_alarm_pm = GAS_DROP_ALARM_PM;
}

const thresholds_t *thresholds_get(void)
{
  return &th;
}

/* 단계 순서: low_alarm < low_warn < high_warn < high_alarm, warn < alarm */
static bool order_ok(const thresholds_t *t)
{
  return t->cell_low_alarm_mv < t->cell_low_warn_mv &&
         t->cell_low_warn_mv < t->cell_high_warn_mv &&
         t->cell_high_warn_mv < t->cell_high_alarm_mv &&
         t->temp_warn_x10 < t->temp_alarm_x10 &&
         t->gas_drop_warn_pm < t->gas_drop_alarm_pm;
}

bool thresholds_set(const char *key, int32_t value, const char **err)
{
  for (unsigned i = 0; i < TABLE_LEN; i++) {
    if (strcmp(key, table[i].key) != 0) {
      continue;
    }
    if (value < table[i].min || value > table[i].max) {
      *err = "out of range";
      return false;
    }

    int32_t old = *table[i].value;
    *table[i].value = value;
    if (!order_ok(&th)) {
      *table[i].value = old;      /* 순서가 어긋나면 되돌린다 */
      *err = "breaks warn/alarm order";
      return false;
    }
    LOG("threshold %s: %ld -> %ld", key, (long)old, (long)value);
    return true;
  }
  *err = "unknown key";
  return false;
}
