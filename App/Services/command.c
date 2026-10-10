/*
 * command.c -- 서버 명령 해석 ($CMD,id,SET,key,value)
 * 역할: 블루투스로 받은 한 줄을 쉼표로 잘라 검사하고, 임계값 변경 명령이면 thresholds 에 넘긴다.
 *       응답(ack)은 보내지 않고 결과는 디버그 로그(USART2)로만 남긴다.
 * 용도: app.c 의 on_bt_line 이 받은 줄을 이 모듈로 전달한다. 명령 형식은 command.h 참고.
 */

#include "command.h"
#include "config.h"
#include "thresholds.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

#define MAX_FIELDS 5   /* $CMD, id, SET, key, value */

/* buf 의 쉼표를 '\0' 로 바꿔 필드 포인터를 채우고 개수를 반환 (복사/동적 할당 없음) */
static int split_fields(char *buf, char *fields[], int max)
{
  int n = 0;
  fields[n++] = buf;
  for (char *p = buf; *p != '\0'; p++) {
    if (*p == ',') {
      *p = '\0';
      if (n >= max) {
        return max + 1;       /* 필드가 너무 많음 */
      }
      fields[n++] = p + 1;
    }
  }
  return n;
}

/* 10진 정수 하나로 끝까지 해석되면 true */
static bool parse_int(const char *s, long *out)
{
  char *end;
  if (*s == '\0') {
    return false;
  }
  *out = strtol(s, &end, 10);
  return *end == '\0';
}

void command_handle_line(const char *line)
{
  char buf[BT_RX_LINE_MAX];
  char *f[MAX_FIELDS];
  long id, value;

  strncpy(buf, line, sizeof buf - 1);
  buf[sizeof buf - 1] = '\0';

  if (split_fields(buf, f, MAX_FIELDS) != MAX_FIELDS || strcmp(f[0], "$CMD") != 0) {
    LOG("command ignored: %s", line);
    return;
  }
  if (!parse_int(f[1], &id)) {
    LOG("command rejected: bad id");
    return;
  }
  if (id != 0 && id != APP_DEVICE_ID) {
    return;                    /* 다른 장치에 보낸 명령 */
  }
  if (strcmp(f[2], "SET") != 0) {
    LOG("command rejected: unknown command '%s'", f[2]);
    return;
  }
  if (!parse_int(f[4], &value)) {
    LOG("command rejected: bad value '%s'", f[4]);
    return;
  }

  const char *err = NULL;
  if (!thresholds_set(f[3], (int32_t)value, &err)) {
    LOG("threshold %s=%ld rejected: %s", f[3], value, err);
  }
}
