#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stddef.h>
#include <stdint.h>
#include "measurement.h"
#include "dcir.h"

/*
 * 패킷 형식 (한 줄 텍스트, 무효 값은 NA):
 *   $BAT,id,packet_index,tick,cell_mv,cell_temp,air_temp,air_humi,air_pres,gas,
 *        vbus_mv,current_ma,charge_state,soc_permil,charge_enabled*CC<LF>
 *   온도/습도/기압은 0.1 단위 정수, CC 는 '$' 와 '*' 사이 문자의 XOR(16진 2자리).
 *   gas: BME680 가스 저항(ohm)이 있으면 그 값, 없으면 MQ 센서 전압(mV).
 *   vbus_mv/current_ma: INA219 (충전 +, 방전 -), 없으면 NA.
 *   charge_state: 0 미연결, 1 충전 중, 2 완충, 3 비정상.  soc_permil: 0~1000, 없으면 NA.
 *   charge_enabled: 0/1 (충전 차단 MOSFET).
 *
 * DCIR 결과 패킷 (측정 1세트 완료 시 1회, 값은 모두 정수/uV/uA/uohm):
 *   $DCIR,id,tick,v0_uv,v1_uv,i0_ua,i1_ua,dcir_uohm,dcir_std_uohm,repeat_n,vcell_mv,temp_mc*CC<LF>
 *   temp_mc: 0.001 C, 온도 정보가 없으면 NA.
 */

/* buf 에 패킷을 만들고 길이를 반환 (공간 부족 시 0) */
size_t telemetry_format(char *buf, size_t size, const measurement_t *m, uint32_t packet_index);

/* 패킷을 만들어 bt_link 로 송신 */
void telemetry_send(const measurement_t *m);

/* DCIR 결과 패킷을 만들고(공간 부족 시 0) / 송신 */
size_t telemetry_format_dcir(char *buf, size_t size, const dcir_result_t *r);
void telemetry_send_dcir(const dcir_result_t *r);

#endif /* TELEMETRY_H */
