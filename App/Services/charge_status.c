#include "charge_status.h"
#include "config.h"
#include "tp4056.h"
#include "log.h"

#define SAMPLE_MS 20

static uint8_t  stable = CHARGE_STATE_NONE;
static uint8_t  candidate = CHARGE_STATE_NONE;
static uint32_t candidate_since;
static uint32_t last_ms;

static uint8_t read_raw(void)
{
  bool chrg = tp4056_chrg_active();
  bool stdby = tp4056_stdby_active();
  if (chrg && stdby) {
    return CHARGE_STATE_FAULT;
  }
  if (chrg) {
    return CHARGE_STATE_CHARGING;
  }
  if (stdby) {
    return CHARGE_STATE_DONE;
  }
  return CHARGE_STATE_NONE;
}

void charge_status_init(void)
{
  tp4056_init();
  stable = candidate = read_raw();
}

void charge_status_task(uint32_t now_ms)
{
  if ((now_ms - last_ms) < SAMPLE_MS) {
    return;
  }
  last_ms = now_ms;

  uint8_t raw = read_raw();
  if (raw != candidate) {
    candidate = raw;
    candidate_since = now_ms;
  } else if (candidate != stable && (now_ms - candidate_since) >= CHG_DEBOUNCE_MS) {
    stable = candidate;
    LOG("charge_state=%u", stable);
  }
}

uint8_t charge_status_get(void)
{
  return stable;
}
