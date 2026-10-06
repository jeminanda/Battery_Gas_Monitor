#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stddef.h>
#include <stdint.h>
#include "measurement.h"

/*
 * 패킷 형식 (한 줄 텍스트, 무효 값은 NA):
 *   $BAT,id,seq,tick,vdda,cell_mv,soc,batt_t,env_t,env_h,env_p,gas_ohm,mq_mv,state,flags*CC\n
 *   온도/습도/기압은 0.1 단위 정수, CC 는 '$' 와 '*' 사이 문자의 XOR(16진 2자리).
 */

/* buf 에 패킷을 만들고 길이를 반환 (공간 부족 시 0) */
size_t telemetry_format(char *buf, size_t size, const measurement_t *m, uint32_t seq);

/* 패킷을 만들어 bt_link 로 송신 */
void telemetry_send(const measurement_t *m);

#endif /* TELEMETRY_H */
