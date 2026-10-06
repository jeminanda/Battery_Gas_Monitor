#include "battery_soc.h"

typedef struct { uint16_t mv; uint8_t pct; } soc_point_t;

static const soc_point_t table[] = {
  {3000, 0}, {3500, 10}, {3600, 25}, {3700, 50},
  {3800, 65}, {3900, 80}, {4000, 90}, {4200, 100},
};
#define TABLE_LEN (sizeof(table) / sizeof(table[0]))

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
