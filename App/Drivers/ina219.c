#include "ina219.h"
#include "config.h"
#include "i2c.h"

/* INA219 전류/전압 모니터 (I2C1). 션트 전압(10uV/LSB)으로 전류를, 버스 전압(4mV/LSB)을 읽는다 */

#define REG_CONFIG   0x00
#define REG_SHUNT    0x01
#define REG_BUS      0x02

#define I2C_TIMEOUT_MS 5
#define ADDR           ((uint16_t)(INA219_I2C_ADDR_7BIT << 1))

/* BRNG=16V, PGA, BADC=12bit(532us), SADC=12bit(532us), 션트+버스 연속 변환 */
#define CONFIG_WORD ((uint16_t)((0u << 13) | ((INA219_PGA & 3u) << 11) | \
                                (0x3u << 7) | (0x3u << 3) | 0x7u))

void ina219_bus_init(void)
{
  HAL_I2C_DeInit(&hi2c1);
  hi2c1.Init.ClockSpeed = APP_I2C_SPEED_HZ;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  HAL_I2C_Init(&hi2c1);
}

/* 버스 오류(SDA 고착 등)가 났을 때만 I2C 를 다시 초기화. NACK(미연결)은 무시 */
static void recover_if_needed(void)
{
  uint32_t err = HAL_I2C_GetError(&hi2c1);
  if (err & (HAL_I2C_ERROR_BERR | HAL_I2C_ERROR_ARLO | HAL_I2C_ERROR_TIMEOUT)) {
    ina219_bus_init();
  }
}

static bool read_reg(uint8_t reg, uint16_t *val)
{
  uint8_t b[2];
  if (HAL_I2C_Mem_Read(&hi2c1, ADDR, reg, I2C_MEMADD_SIZE_8BIT, b, 2, I2C_TIMEOUT_MS) != HAL_OK) {
    recover_if_needed();
    return false;
  }
  *val = (uint16_t)((b[0] << 8) | b[1]);
  return true;
}

static bool write_reg(uint8_t reg, uint16_t val)
{
  uint8_t b[2] = { (uint8_t)(val >> 8), (uint8_t)val };
  if (HAL_I2C_Mem_Write(&hi2c1, ADDR, reg, I2C_MEMADD_SIZE_8BIT, b, 2, I2C_TIMEOUT_MS) != HAL_OK) {
    recover_if_needed();
    return false;
  }
  return true;
}

bool ina219_probe(void)
{
  if (HAL_I2C_IsDeviceReady(&hi2c1, ADDR, 1, I2C_TIMEOUT_MS) != HAL_OK) {
    recover_if_needed();
    return false;
  }
  uint16_t rb = 0;
  if (!write_reg(REG_CONFIG, CONFIG_WORD) || !read_reg(REG_CONFIG, &rb)) {
    return false;
  }
  return rb == CONFIG_WORD;
}

bool ina219_read_shunt_uv(int32_t *uv)
{
  uint16_t raw;
  if (!read_reg(REG_SHUNT, &raw)) {
    return false;
  }
  *uv = (int32_t)(int16_t)raw * 10;   /* LSB = 10 uV */
  return true;
}

bool ina219_read_bus_mv(uint16_t *mv)
{
  uint16_t raw;
  if (!read_reg(REG_BUS, &raw)) {
    return false;
  }
  if (raw & 0x0001u) {                /* OVF: 범위 초과 */
    return false;
  }
  *mv = (uint16_t)((raw >> 3) * 4u);  /* LSB = 4 mV */
  return true;
}

bool ina219_read_current_ua(int32_t *ua)
{
  int32_t uv;
  if (!ina219_read_shunt_uv(&uv)) {
    return false;
  }
  /* uV / mohm = mA -> x1000 = uA */
  *ua = (uv * 1000 / INA219_SHUNT_MOHM) * INA219_CURRENT_SIGN;
  return true;
}
