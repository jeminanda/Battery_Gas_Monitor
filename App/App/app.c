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
#include "mq_gas.h"
#include "power_monitor.h"
#include "charge_status.h"
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
}

static void measure_update(uint32_t now)
{
  meas.tick_ms = now;
  meas.vdda_mv = adc_sampler_get_vdda_mv();
  meas.cell_mv = cell_voltage_mv_from_raw(adc_sampler_get_raw(ADC_IDX_CELL), meas.vdda_mv);
  meas.soc_pct = battery_soc_pct_from_mv(meas.cell_mv);

  meas.mq_ready = mq_gas_is_ready(now);
  meas.mq_mv = mq_gas_mv_from_raw(adc_sampler_get_raw(ADC_IDX_MQ), meas.vdda_mv);

#if APP_HAVE_NTC
  meas.batt_temp_valid = ntc_temp_x10_from_raw(adc_sampler_get_raw(ADC_IDX_NTC),
                                               meas.vdda_mv, &meas.batt_temp_x10);
#else
  meas.batt_temp_valid = false;
#endif

  meas.env = *env_sensor_get();

  safety_input_t in = {
    .cell_mv = meas.cell_mv,
    .batt_temp_valid = meas.batt_temp_valid,
    .batt_temp_x10 = meas.batt_temp_x10,
    .mq_ready = meas.mq_ready,
    .mq_mv = meas.mq_mv,
  };
  meas.safety = safety_evaluate(&in);

  const power_data_t *pw = power_monitor_get();
  meas.power_valid = pw->valid;
  meas.vbus_mv = pw->vbus_mv;
  meas.current_ma = (int16_t)pw->current_ma;
  meas.charge_state = charge_status_get();

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
  power_monitor_init();
  charge_status_init();
  bt_link_init();
  bt_link_set_line_cb(on_bt_line);
  LOG("Battery monitor boot (ntc=%d, bme680=%d)", APP_HAVE_NTC, ENV_BACKEND_BME680);
}

void app_loop(void)
{
  static uint32_t t_eval, t_tx, t_log;
  uint32_t now = HAL_GetTick();

  adc_sampler_task(now);
  env_sensor_task(now);
  power_monitor_task(now);
  charge_status_task(now);
  bt_link_task();

  if (adc_sampler_ready()) {
    if (due(&t_eval, now, PERIOD_EVAL_MS)) {
      measure_update(now);
    }
    if (meas_valid && due(&t_tx, now, PERIOD_TELEMETRY_MS)) {
      telemetry_send(&meas);
    }
    if (meas_valid && due(&t_log, now, PERIOD_LOG_MS)) {
      LOG("cell=%umV soc=%u%% mq=%umV env_t=%d state=%d flags=0x%X",
          meas.cell_mv, meas.soc_pct, meas.mq_mv, meas.env.temp_x10,
          meas.safety.state, meas.safety.flags);
    }
  }
  update_led(now);
}
