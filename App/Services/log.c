#include "log.h"
#include "usart.h"

/* syscalls.c 의 weak __io_putchar 를 재정의 */
int __io_putchar(int ch)
{
  uint8_t c = (uint8_t)ch;
  HAL_UART_Transmit(&huart2, &c, 1, 10);
  return ch;
}

void log_init(void)
{
  setvbuf(stdout, NULL, _IONBF, 0);
}
