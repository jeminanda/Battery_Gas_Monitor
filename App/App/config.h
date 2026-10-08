#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ---- 하드웨어 존재 여부 (부품이 없으면 0) ------------------------------ */
#define APP_HAVE_NTC            1   /* 1: 배터리 온도 NTC 사용(PA1), 0이면 배터리 온도는 "없음"으로 처리 */

/* ---- 장치/통신 -------------------------------------------------------- */
#define APP_DEVICE_ID           1
#define BT_TX_BUF_SIZE          512 /* 2의 거듭제곱이어야 함 */
#define BT_RX_LINE_MAX          64

/* ---- 주기 (ms) -------------------------------------------------------- */
#define PERIOD_ADC_SAMPLE_MS    10
#define PERIOD_ENV_MS           2000    /* BME680 측정 주기 */
#define PERIOD_EVAL_MS          100
#define PERIOD_TELEMETRY_MS     1000
#define PERIOD_LOG_MS           1000
#define PERIOD_POWER_MS         100     /* INA219 읽기(= SOC 적분) 주기 */
#define PERIOD_INA_REPROBE_MS   1000    /* INA219 미검출 시 재탐색 주기 (꽂으면 자동 인식) */

/* ---- ADC -------------------------------------------------------------- */
#define ADC_FULL_SCALE          4095U
#define ADC_AVG_SAMPLES         16
#define APP_VREFINT_CAL_ADDR    ((const volatile uint16_t *)0x1FFF7A2AUL)
#define VREFINT_CAL_VDDA_MV     3300U   /* 공장 캘리브레이션 시 VDDA */
#define VDDA_NOMINAL_MV         3300U

/* ---- 셀 전압 분압 (TODO: 실제 저항값으로 수정) ------------------------- */
#define CELL_DIV_R_TOP_OHM      10000UL
#define CELL_DIV_R_BOTTOM_OHM   10000UL

/* ---- 안전 임계값 (Li-ion 1셀 가정) ------------------------------------- */
#define SAFE_CELL_LOW_WARN_MV   3300
#define SAFE_CELL_LOW_ALARM_MV  3000
#define SAFE_CELL_HIGH_WARN_MV  4250
#define SAFE_CELL_HIGH_ALARM_MV 4350
#define SAFE_TEMP_WARN_X10      450     /* 45.0 C */
#define SAFE_TEMP_ALARM_X10     600     /* 60.0 C */

/* ---- 가스 이상 감지 (BME680 가스 저항의 기준값 대비 감소율) ------------ */
/* TODO: 실제 케이스 내부에서 실측 후 보정 */
#define GAS_WARMUP_MS           1800000UL /* 첫 유효 샘플 후 기준값 안정화 시간 30분 (판정 보류). 47k~48k 로 안정되는 데 약 30분 */
#define GAS_DROP_WARN_PM        300     /* 기준 대비 30.0% 이상 감소 -> 경고 */
#define GAS_DROP_ALARM_PM       500     /* 기준 대비 50.0% 이상 감소 -> 경보 */
#define GAS_DROP_HYST_PM        50      /* 단계를 내릴 때 5.0% 히스테리시스 */
#define GAS_CONFIRM_SAMPLES     2       /* 같은 판정이 연속으로 이만큼 나와야 단계 변경 (노이즈 방지) */
#define GAS_BASE_UP_SHIFT       3       /* 기준값 상승 추적: 1/8 씩 (공기가 깨끗해지는 방향은 빠르게) */
#define GAS_BASE_DOWN_SHIFT     9       /* 기준값 하락 추적: 1/512 씩 (매우 느리게) */
#define GAS_STALE_MS            10000UL /* 새 가스 측정이 이 시간 동안 없으면 판정 무효 */

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

/* ---- DCIR 측정 (TIM3 원펄스 부하 + 충전 차단 인터록) -------------------- */
/* 부하 펄스 폭은 CubeMX 의 TIM3 설정(Prescaler/Period)으로 결정된다. (현재 약 100ms) */
#define DCIR_WINDOW_START_MS    80      /* 0~80ms 는 버리고 */
#define DCIR_WINDOW_END_MS      98      /* 80~98ms 구간을 V1, I1 로 평균 */
#define DCIR_BASELINE_SAMPLES   64      /* V0, I0 평균 횟수 */
#define DCIR_REST_MS            30000UL /* 충전기 분리 후 휴지 */
#define DCIR_REPEAT_REST_MS     10000UL /* 반복 측정 사이 회복 시간 */
#define DCIR_REPEAT_N           5       /* 측정 반복 횟수 (5~10) */
#define DCIR_MIN_VALID          3       /* 유효한 반복이 이보다 적으면 결과 폐기 */
#define DCIR_MIN_INTERVAL_MS    60000UL /* 직전 측정 후 최소 간격 */
#define DCIR_MIN_CELL_MV        3300    /* 이 전압 미만이면 측정 취소 */
#define DCIR_MIN_DELTA_UA       20000   /* 부하 ON/OFF 전류 변화가 이보다 작으면 무효 (예상 약 75mA) */
#define DCIR_MIN_WINDOW_SAMPLES 8       /* 80~98ms 구간 최소 샘플 수 */
#define DCIR_AUTO_INTERVAL_MS   0UL     /* 0 이면 자동 측정 없음 (B1 버튼 / BT "DCIR" 명령으로 시작) */

/* ---- NTC 서미스터 (PA1, 분압 회로) ------------------------------------ */
/* TODO: 실제 부품/회로에 맞게 확인. 아래는 흔한 10k, B3950 + 10k 고정저항 가정 */
#define NTC_R_FIXED_OHM         10000UL /* 분압 고정저항 */
#define NTC_R0_OHM              10000UL /* 25 C 에서의 NTC 저항 */
#define NTC_T0_C                25.0f
#define NTC_BETA                3950UL  /* 데이터시트 B 값 */
#define NTC_ON_LOW_SIDE         1       /* 1: 고정저항-VDD, NTC-GND (PA1 은 중간점) / 0: 반대 */
#define NTC_RAW_MIN             50      /* 이보다 작으면 단락/단선으로 판정 */
#define NTC_RAW_MAX             4045
#define NTC_TEMP_MIN_C          (-40.0f)
#define NTC_TEMP_MAX_C          125.0f

/* ---- BME680 ----------------------------------------------------------- */
#define BME680_I2C_ADDR_7BIT    0x76    /* SDO=GND:0x76, SDO=VDD:0x77 */
#define BME680_HEATER_TEMP_C    300     /* 가스 측정용 히터 온도 */
#define BME680_HEATER_DUR_MS    100     /* 히터 가열 시간 */

#endif /* APP_CONFIG_H */
