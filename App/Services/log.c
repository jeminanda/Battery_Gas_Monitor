/*
 * log.c -- 디버그 로그 출력 (USART2, ST-Link 가상 COM 포트)
 * 역할: printf 의 출력 한 글자(__io_putchar)를 USART2 로 보내 LOG() 매크로가 동작하게 한다.
 * 용도: 시리얼 모니터(115200 baud)로 보드 상태와 센서 연결 여부를 확인한다.
 */

#include "log.h"
#include "usart.h"

/* syscalls.c 의 weak __io_putchar 를 재정의 */
int __io_putchar(int ch)
{
  uint8_t c = (uint8_t)ch;
  HAL_UART_Transmit(&huart2, &c, 1, 10);
  return ch;
}

/* printf 를 버퍼 없이(_IONBF) 바로 UART 로 내보내도록 설정 */
void log_init(void)
{
  setvbuf(stdout, NULL, _IONBF, 0);
}
