#ifndef BT_LINK_H
#define BT_LINK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 수신된 한 줄(개행 제외, NUL 종료)을 전달하는 콜백 */
typedef void (*bt_line_cb_t)(const char *line);

/* USART1 인터럽트 기반 송수신 시작 */
void bt_link_init(void);

/* 패킷 전체를 송신 큐에 넣는다. 공간이 부족하면 패킷 전체를 버리고 false. */
bool bt_link_send(const char *data, size_t len);

void bt_link_set_line_cb(bt_line_cb_t cb);

/* 메인 루프에서 호출: 수신된 줄 처리 */
void bt_link_task(void);

#endif /* BT_LINK_H */
