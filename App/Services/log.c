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
