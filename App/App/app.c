#include "app.h"
#include "config.h"
#include "main.h"
#include "measurement.h"
#include "adc_sampler.h"
#include "env_sensor.h"
#include "battery_soc.h"
#include "safety.h"
#include "telemetry.h"
#include "bt_link.h"
#include "log.h"
#include "cell_voltage.h"
#include "gas_monitor.h"
#include "power_monitor.h"
#include "charge_status.h"
#include "charge_ctrl.h"
#include "soc_estimator.h"
#include "dcir.h"
#include <string.h>
#include <stdlib.h>
#if APP_HAVE_NTC
#include "ntc.h"
#endif

static measurement_t meas;
static bool          meas_valid;

static bool due(uint32_t *last, uint32_t now, uint32_t period)
{
  if ((now - *last) >= period) {
    *last = now;
    return true;
  }
  return false;
}

static void on_bt_line(const char *line)
{
  /* TODO: 서버/라즈베리파이 명령 처리 (임계값 변경, 주기 변경 등) */
  LOG("BT rx: %s", line);

  if (strcmp(line, "DCIR") == 0) {
    if (!dcir_request()) {
      LOG("DCIR already running");
    }
  } else if (strcmp(line, "DCIR STOP") == 0) {
    dcir_abort("stop command");
  }
}

/* B1(사용자 버튼, 누르면 Low): DCIR 측정 시작. 서버 없이 보드만으로 시험할 때 사용 */
static void poll_button(uint32_t now)
{
  static uint32_t last;
  static bool prev;
  if (!due(&last, now, 20)) {
    return;
  }
  bool pressed = (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET);
  if (pressed && !prev) {
    LOG("B1 pressed: DCIR request");
    if (!dcir_request()) {
      LOG("DCIR already running");
    }
  }
  prev = pressed;
}

static void measure_update(uint32_t now)
{
  meas.tick_ms = now;
  meas.vdda_mv = adc_sampler_get_vdda_mv();
  meas.cell_mv = cell_voltage_mv_from_raw(adc_sampler_get_raw(ADC_IDX_CELL), meas.vdda_mv);
  meas.soc_pct = battery_soc_pct_from_mv(meas.cell_mv);

#if APP_HAVE_NTC
  uint16_t ntc_raw = adc_sampler_get_raw(ADC_IDX_NTC);
  meas.batt_temp_valid = ntc_temp_x10_from_raw(ntc_raw, meas.vdda_mv, &meas.batt_temp_x10);
  static bool ntc_reported, ntc_last;
  if (!ntc_reported || meas.batt_temp_valid != ntc_last) {
    ntc_reported = true;
    ntc_last = meas.batt_temp_valid;
    if (meas.batt_temp_valid) {
      LOG("NTC OK: %d.%d C (raw=%u)", meas.batt_temp_x10 / 10, abs(meas.batt_temp_x10 % 10), ntc_raw);
    } else {
      LOG("NTC not detected (raw=%u): open or short", ntc_raw);
    }
  }
#else
  meas.batt_temp_valid = false;
#endif

  meas.env = *env_sensor_get();
  gas_monitor_update(now, &meas.env);
  meas.gas = *gas_monitor_get();

  safety_input_t in = {
    .cell_mv = meas.cell_mv,
    .batt_temp_valid = meas.batt_temp_valid,
    .batt_temp_x10 = meas.batt_temp_x10,
    .gas_valid = meas.gas.valid,
    .gas_warmup = meas.gas.warmup,
    .gas_level = (uint8_t)meas.gas.level,
  };
  meas.safety = safety_evaluate(&in);

  /* 전류 / 충전 / SOC */
  const power_data_t *pw = power_monitor_get();
  meas.power_valid = pw->valid;
  meas.vbus_mv = pw->vbus_mv;
  meas.current_ma = (int16_t)pw->current_ma;
  meas.charge_state = charge_status_get();
  charge_ctrl_update(now, &meas.safety);
  meas.charge_enabled = charge_ctrl_is_enabled();
  soc_estimator_update(now, pw->valid, pw->current_ma, meas.cell_mv, meas.charge_state);
  meas.soc_valid = soc_estimator_valid();
  meas.soc_permil = soc_estimator_permil();

  meas_valid = true;
}

/* OK: 1초 점멸, WARN: 0.25초 점멸, ALARM: 상시 점등 */
static void update_led(uint32_t now)
{
  static uint32_t last;
  uint32_t half_period = (meas.safety.state == SAFETY_WARN) ? 250 : 500;

  if (meas_valid && meas.safety.state == SAFETY_ALARM) {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  } else if (due(&last, now, half_period)) {
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
  }
}

void app_init(void)
{
  log_init();
  adc_sampler_init();
  env_sensor_init();
  gas_monitor_init();
  power_monitor_init();
  charge_status_init();
  charge_ctrl_init();
  soc_estimator_init();
  dcir_init();
  bt_link_init();
  bt_link_set_line_cb(on_bt_line);
  LOG("Battery monitor boot (ntc=%d)", APP_HAVE_NTC);
}

void app_loop(void)
{
  static uint32_t t_eval, t_tx, t_log;
  uint32_t now = HAL_GetTick();

  adc_sampler_task(now);
  env_sensor_task(now);
  power_monitor_task(now);
  charge_status_task(now);
  poll_button(now);
  bt_link_task();

  if (adc_sampler_ready()) {
    if (due(&t_eval, now, PERIOD_EVAL_MS)) {
      measure_update(now);
    }
    dcir_input_t di = {
      .ready = meas_valid,
      .cell_mv = meas.cell_mv,
      .charge_state = meas.charge_state,
      .power_valid = meas.power_valid,
      .temp_valid = meas.batt_temp_valid || (meas.env.valid_mask & ENV_HAS_TEMP),
      .temp_x10 = meas.batt_temp_valid ? meas.batt_temp_x10 : meas.env.temp_x10,
    };
    dcir_task(now, &di);

    if (meas_valid && due(&t_tx, now, PERIOD_TELEMETRY_MS)) {
      telemetry_send(&meas);
    }
    if (meas_valid && due(&t_log, now, PERIOD_LOG_MS)) {
      LOG("cell=%umV soc=%u%% env_t=%d state=%d flags=0x%X i=%dmA chg=%u en=%u socq=%u",
          meas.cell_mv, meas.soc_pct, meas.env.temp_x10,
          meas.safety.state, meas.safety.flags,
          meas.current_ma, meas.charge_state, meas.charge_enabled, meas.soc_permil);
      LOG("ntc=%d(%d) env=0x%X h=%u p=%u gas=%lu base=%lu drop=%u.%u%% lvl=%d%s",
          meas.batt_temp_valid, meas.batt_temp_x10, meas.env.valid_mask,
          meas.env.hum_x10, meas.env.press_hpa_x10, (unsigned long)meas.env.gas_ohm,
          (unsigned long)meas.gas.baseline_ohm,
          (unsigned)(meas.gas.drop_pm / 10), (unsigned)(meas.gas.drop_pm % 10),
          meas.gas.level, meas.gas.warmup ? " (warmup)" : "");
    }
  }
  update_led(now);
}
