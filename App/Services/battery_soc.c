/*
 * battery_soc.c -- 전압 기반 잔량(OCV 테이블)
 * 역할: 셀 전압(mV)을 1셀 리튬이온 무부하 곡선 테이블로 보간해 잔량(%, 0.1%)으로 바꾼다.
 * 용도: app.c 는 디버그 로그의 soc% 로, soc_estimator.c 는 전류 적분 SOC 의
 *       초기값과 휴지 시 보정값으로 사용한다.
 */

#include "battery_soc.h"

/* 셀 전압(mV) -> 잔량(%) 기준점. 1셀 리튬이온의 무부하(OCV) 곡선을 근사한 값 */
typedef struct { uint16_t mv; uint8_t pct; } soc_point_t;

/* 전압 오름차순. 기준점 사이는 직선으로 보간한다 */
static const soc_point_t table[] = {
  {3000, 0}, {3500, 10}, {3600, 25}, {3700, 50},
  {3800, 65}, {3900, 80}, {4000, 90}, {4200, 100},
};
#define TABLE_LEN (sizeof(table) / sizeof(table[0]))

/* 셀 전압 -> 잔량 % (0~100). 범위 밖은 양 끝 값으로 고정 */
uint8_t battery_soc_pct_from_mv(uint16_t cell_mv)
{
  if (cell_mv <= table[0].mv) {
    return table[0].pct;
  }
  for (unsigned i = 1; i < TABLE_LEN; i++) {
    if (cell_mv <= table[i].mv) {
      uint32_t dmv = table[i].mv - table[i - 1].mv;
      uint32_t dpct = table[i].pct - table[i - 1].pct;
      return (uint8_t)(table[i - 1].pct + ((cell_mv - table[i - 1].mv) * dpct) / dmv);
    }
  }
  return table[TABLE_LEN - 1].pct;
}

/* 위와 같은 테이블을 0.1% 단위(0~1000)로. 전류 적분 SOC 의 초기값/보정값에 쓴다 */
uint16_t battery_soc_permil_from_mv(uint16_t cell_mv)
{
  if (cell_mv <= table[0].mv) {
    return (uint16_t)(table[0].pct * 10);
  }
  for (unsigned i = 1; i < TABLE_LEN; i++) {
    if (cell_mv <= table[i].mv) {
      uint32_t dmv = table[i].mv - table[i - 1].mv;
      uint32_t dpm = (uint32_t)(table[i].pct - table[i - 1].pct) * 10;
      return (uint16_t)(table[i - 1].pct * 10 + ((cell_mv - table[i - 1].mv) * dpm) / dmv);
    }
  }
  return (uint16_t)(table[TABLE_LEN - 1].pct * 10);
}
