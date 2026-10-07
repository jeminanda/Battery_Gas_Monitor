#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ---- 하드웨어 존재 여부 (부품이 없으면 0) ------------------------------ */
#define APP_HAVE_NTC            0   /* NTC 미도착: 0이면 배터리 온도는 "없음"으로 처리 */
#define ENV_BACKEND_BME680      0   /* BME680 미도착: 0이면 DHT11 백엔드 사용 */

/* ---- 장치/통신 -------------------------------------------------------- */
#define APP_DEVICE_ID           1
#define BT_TX_BUF_SIZE          512 /* 2의 거듭제곱이어야 함 */
#define BT_RX_LINE_MAX          64

/* ---- 주기 (ms) -------------------------------------------------------- */
#define PERIOD_ADC_SAMPLE_MS    10
#define PERIOD_ENV_MS           2000    /* DHT11 최소 간격 1s 이상 */
#define PERIOD_EVAL_MS          100
#define PERIOD_TELEMETRY_MS     1000
#define PERIOD_LOG_MS           1000
#define PERIOD_POWER_MS         100     /* INA219 읽기(= SOC 적분) 주기 */
#define PERIOD_INA_REPROBE_MS   1000    /* INA219 미검출 시 재탐색 주기 (꽂으면 자동 인식) */

/* ---- ADC -------------------------------------------------------------- */
#define ADC_FULL_SCALE          4095U
#define ADC_AVG_SAMPLES         16
#define VREFINT_CAL_ADDR        ((const volatile uint16_t *)0x1FFF7A2AUL)
#define VREFINT_CAL_VDDA_MV     3300U   /* 공장 캘리브레이션 시 VDDA */
#define VDDA_NOMINAL_MV         3300U

/* ---- 셀 전압 분압 (TODO: 실제 저항값으로 수정) ------------------------- */
#define CELL_DIV_R_TOP_OHM      10000UL
#define CELL_DIV_R_BOTTOM_OHM   10000UL

/* ---- MQ 가스센서 ------------------------------------------------------ */
#define MQ_WARMUP_MS            60000UL /* TODO: 센서 데이터시트 기준으로 조정 */

/* ---- 안전 임계값 (Li-ion 1셀 가정) ------------------------------------- */
#define SAFE_CELL_LOW_WARN_MV   3300
#define SAFE_CELL_LOW_ALARM_MV  3000
#define SAFE_CELL_HIGH_WARN_MV  4250
#define SAFE_CELL_HIGH_ALARM_MV 4350
#define SAFE_TEMP_WARN_X10      450     /* 45.0 C */
#define SAFE_TEMP_ALARM_X10     600     /* 60.0 C */
#define SAFE_GAS_WARN_MV        1500    /* TODO: 실측 후 보정 */
#define SAFE_GAS_ALARM_MV       2200

/* ---- I2C / INA219 (전류 측정, 없으면 자동으로 "NA" 처리) --------------- */
#define APP_I2C_SPEED_HZ        400000UL
#define INA219_I2C_ADDR_7BIT    0x40
#define INA219_SHUNT_MOHM       100     /* TODO: 모듈 션트 저항 확인 (R100 = 100 mohm) */
#define INA219_PGA              3       /* 0:+-40mV 1:+-80mV 2:+-160mV 3:+-320mV (0.1ohm 에서 3.2A) */
#define INA219_CURRENT_SIGN     (+1)    /* 충전 + / 방전 - 가 되도록 배선에 맞게 +1/-1 */

/* ---- TP4056 충전 상태 (CHRG/STDBY 는 open-drain, Low 가 활성) ----------- */
#define CHG_DEBOUNCE_MS         500     /* 이 시간 동안 같은 값이어야 상태 전환 (CHRG 점멸 무시) */

/* ---- 충전 차단 (MOSFET1, PB10 High = 충전 허용) -------------------------- */
#define CHARGE_CUT_HOLD_MS      10000UL /* 경보가 풀려도 이 시간 동안은 차단 유지 (채터링 방지) */

/* ---- SOC (전류 적분 + 완충 리셋 + 휴지 시 OCV 보정) --------------------- */
#define BATT_CAPACITY_MAH       2200    /* ZM18650P-2200 공칭 용량 */
#define SOC_BATT_PRESENT_MV     2500    /* 이 전압 미만이면 배터리 없음으로 보고 SOC 무효 */
#define SOC_REST_CURRENT_MA     10      /* |전류| 가 이 값 이하이면 휴지 상태 */
#define SOC_REST_CORRECT_MS     1800000UL /* 휴지가 이 시간 지속되면 OCV 로 보정 */
#define SOC_MAX_DT_MS           2000UL  /* 적분 간격이 이보다 길면 해당 구간은 적분하지 않음 */

/* ---- BME680 (도착 후 사용) -------------------------------------------- */
#define BME680_I2C_ADDR_7BIT    0x76    /* SDO=GND:0x76, SDO=VDD:0x77 */

#endif /* APP_CONFIG_H */
