#ifndef COMMAND_H
#define COMMAND_H

/*
 * 서버 -> 단말 명령 (블루투스로 한 줄, 쉼표 구분, 체크섬/응답 없음):
 *   $CMD,<id>,SET,<key>,<value>
 *     id    : 장치 ID (APP_DEVICE_ID). 0 이면 모든 장치
 *     key   : thresholds.c 의 8개 임계값 이름 (예: gas_drop_alarm_pm)
 *     value : 정수 (key 이름의 단위: _mv, _x10, _pm)
 *   예) $CMD,1,SET,cell_high_alarm_mv,4300
 * 범위/순서 검증에 실패하면 적용하지 않고 이유를 디버그 로그로만 남긴다.
 */

/* bt_link 가 줄 하나를 받을 때마다 호출 */
void command_handle_line(const char *line);

#endif /* COMMAND_H */
