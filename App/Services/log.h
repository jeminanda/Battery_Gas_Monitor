#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include "main.h"

/* USART2(ST-Link VCP, 115200)로 printf 출력 */
void log_init(void);

#define LOG(fmt, ...) printf("[%lu] " fmt "\r\n", (unsigned long)HAL_GetTick(), ##__VA_ARGS__)

#endif /* LOG_H */
