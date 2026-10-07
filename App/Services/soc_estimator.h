#ifndef SOC_ESTIMATOR_H
#define SOC_ESTIMATOR_H

#include <stdint.h>
#include <stdbool.h>

void soc_estimator_init(void);

/*
 * 전류 적분(사다리꼴) 기반 SOC 갱신. power_valid(INA219)와 배터리가 모두 있어야 동작한다.
 *  - 시작 시 OCV 테이블로 초기값 설정
 *  - 완충(STDBY) 시 100% 리셋
 *  - 휴지가 SOC_REST_CORRECT_MS 이상 지속되면 OCV 로 보정
 */
void soc_estimator_update(uint32_t now_ms, bool power_valid, int32_t current_ma,
                          uint16_t cell_mv, uint8_t charge_state);

bool     soc_estimator_valid(void);
uint16_t soc_estimator_permil(void);   /* 0~1000 (0.1% 단위) */

#endif /* SOC_ESTIMATOR_H */
