#ifndef ENV_SENSOR_H
#define ENV_SENSOR_H

/*
 * 환경센서(BME680: 온도/습도/기압/가스저항).
 * 상위 모듈은 valid_mask 로 어떤 필드가 유효한지 확인한다.
 */

#include <stdint.h>
#include <stdbool.h>

#define ENV_HAS_TEMP   (1u << 0)
#define ENV_HAS_HUM    (1u << 1)
#define ENV_HAS_PRESS  (1u << 2)
#define ENV_HAS_GAS    (1u << 3)

typedef struct {
  int16_t  temp_x10;        /* 0.1 C */
  uint16_t hum_x10;         /* 0.1 %RH */
  uint16_t press_hpa_x10;   /* 0.1 hPa */
  uint32_t gas_ohm;         /* BME680 가스 저항 */
  uint8_t  valid_mask;      /* ENV_HAS_* 조합. 0이면 전부 무효 */
  uint32_t sample_id;       /* 새 측정이 저장될 때마다 증가 */
} env_data_t;

bool env_sensor_init(void);
void env_sensor_task(uint32_t now_ms);
const env_data_t *env_sensor_get(void);

#endif /* ENV_SENSOR_H */
