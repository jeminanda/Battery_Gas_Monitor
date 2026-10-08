#include "safety.h"
#include "config.h"

static void raise(safety_result_t *r, safety_state_t s, uint16_t flag)
{
  r->flags |= flag;
  if (s > r->state) {
    r->state = s;
  }
}

safety_result_t safety_evaluate(const safety_input_t *in)
{
  safety_result_t r = { SAFETY_OK, 0 };

  if (in->cell_mv <= SAFE_CELL_LOW_ALARM_MV) {
    raise(&r, SAFETY_ALARM, SAFETY_F_LOW_VOLT);
  } else if (in->cell_mv <= SAFE_CELL_LOW_WARN_MV) {
    raise(&r, SAFETY_WARN, SAFETY_F_LOW_VOLT);
  }
  if (in->cell_mv >= SAFE_CELL_HIGH_ALARM_MV) {
    raise(&r, SAFETY_ALARM, SAFETY_F_HIGH_VOLT);
  } else if (in->cell_mv >= SAFE_CELL_HIGH_WARN_MV) {
    raise(&r, SAFETY_WARN, SAFETY_F_HIGH_VOLT);
  }

  if (!in->batt_temp_valid) {
    r.flags |= SAFETY_F_TEMP_NA;
  } else if (in->batt_temp_x10 >= SAFE_TEMP_ALARM_X10) {
    raise(&r, SAFETY_ALARM, SAFETY_F_OVER_TEMP);
  } else if (in->batt_temp_x10 >= SAFE_TEMP_WARN_X10) {
    raise(&r, SAFETY_WARN, SAFETY_F_OVER_TEMP);
  }

  if (!in->gas_valid) {
    r.flags |= SAFETY_F_GAS_NA;
  } else if (in->gas_warmup) {
    r.flags |= SAFETY_F_GAS_WARMUP;
  } else if (in->gas_level >= 2) {
    raise(&r, SAFETY_ALARM, SAFETY_F_GAS);
  } else if (in->gas_level == 1) {
    raise(&r, SAFETY_WARN, SAFETY_F_GAS);
  }

  return r;
}
