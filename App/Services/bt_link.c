/*
 * bt_link.c -- 블루투스 링크 (USART1)
 * 역할: 송신은 링 버퍼에 쌓고 인터럽트로 보내며(메인 루프 블로킹 없음),
 *       수신은 1바이트씩 받아 개행(\n, \r)에서 한 줄을 완성한다.
 * 용도: telemetry.c 가 패킷을 bt_link_send 로 넣고, 받은 한 줄은 bt_link_task 가
 *       메인 루프에서 콜백(app.c 의 on_bt_line)으로 전달한다.
 */

#include "bt_link.h"
#include "config.h"
#include "main.h"
#include "usart.h"

#define TX_MASK (BT_TX_BUF_SIZE - 1)   /* 크기가 2의 거듭제곱이라 마스크로 순환 */

/* 송신 링 버퍼: head 에 쓰고(메인), tail 에서 보낸다(ISR) */
static uint8_t           tx_buf[BT_TX_BUF_SIZE];
static volatile uint16_t tx_head;      /* 메인 컨텍스트만 갱신 */
static volatile uint16_t tx_tail;      /* ISR 만 갱신 */
static volatile bool     tx_busy;
static uint16_t          tx_chunk;

/* 수신: rx_line 에 모으다 한 줄이 끝나면 rx_ready 로 복사하고 플래그를 세운다 */
static uint8_t           rx_byte;
static char              rx_line[BT_RX_LINE_MAX];
static uint8_t           rx_len;
static char              rx_ready[BT_RX_LINE_MAX];
static volatile bool     rx_ready_flag;

static bt_line_cb_t      line_cb;

/* 인터럽트가 막힌 상태(또는 ISR)에서 호출 */
static void tx_kick(void)
{
  if (tx_busy || tx_head == tx_tail) {
    return;
  }
  uint16_t tail = tx_tail;
  uint16_t head = tx_head;
  uint16_t chunk = (head > tail) ? (head - tail) : (BT_TX_BUF_SIZE - tail);

  if (HAL_UART_Transmit_IT(&huart1, &tx_buf[tail], chunk) == HAL_OK) {
    tx_chunk = chunk;
    tx_busy = true;
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart1) {
    tx_tail = (uint16_t)((tx_tail + tx_chunk) & TX_MASK);
    tx_busy = false;
    tx_kick();
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart != &huart1) {
    return;
  }
  char c = (char)rx_byte;
  if (c == '\n' || c == '\r') {
    if (rx_len > 0 && !rx_ready_flag) {
      rx_line[rx_len] = '\0';
      for (uint8_t i = 0; i <= rx_len; i++) {
        rx_ready[i] = rx_line[i];
      }
      rx_ready_flag = true;
    }
    rx_len = 0;
  } else if (rx_len < BT_RX_LINE_MAX - 1) {
    rx_line[rx_len++] = c;
  } else {
    rx_len = 0;   /* 줄이 너무 길면 버림 */
  }
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart1) {
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  }
}

void bt_link_init(void)
{
  tx_head = tx_tail = 0;
  tx_busy = false;
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

bool bt_link_send(const char *data, size_t len)
{
  uint16_t head = tx_head;
  uint16_t used = (uint16_t)((head - tx_tail) & TX_MASK);
  if (len == 0 || len > (size_t)(BT_TX_BUF_SIZE - 1 - used)) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    tx_buf[head] = (uint8_t)data[i];
    head = (uint16_t)((head + 1) & TX_MASK);
  }

  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  tx_head = head;
  tx_kick();
  __set_PRIMASK(primask);
  return true;
}

void bt_link_set_line_cb(bt_line_cb_t cb)
{
  line_cb = cb;
}

void bt_link_task(void)
{
  if (rx_ready_flag) {
    if (line_cb) {
      line_cb(rx_ready);
    }
    rx_ready_flag = false;
  }
}
