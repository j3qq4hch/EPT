#ifndef __SERIAL_STR
#define __SERIAL_STR

#include <stdint.h>
#include <stdio.h>
#include "serial.h"
#include "pt.h"

typedef struct 
{
  serial_t* serial; // Pointer to actual serial interface 
  char* buf;      
  uint16_t buflen;
  uint16_t cnt;
  char stop_char;
}serpt_ctx_t;

PT_THREAD(serialpt_read_until(struct pt*, serpt_ctx_t*));
PT_THREAD(serialpt_write(struct pt*, serpt_ctx_t*));

#define SERIALPT_READLINE(PT, CHILD_PT, CTX, BUF, BUFLEN)\
do{\
  (CTX)->stop_char = '\r';\
  (CTX)->buflen = BUFLEN;\
  (CTX)->buf = BUF;\
  EPT_SPAWN(PT, CHILD_PT, serialpt_read_until(CHILD_PT, CTX));\
}while(0)

#define SERIALPT_PRINTF(PT, CHILD_PT, CTX, BUF, BUFLEN, fmt, ...)\
do{\
  (CTX)->buf = BUF;\
  (CTX)->buflen = snprintf(BUF, BUFLEN, fmt, ##__VA_ARGS__);\
  EPT_SPAWN(PT, CHILD_PT, serialpt_write(CHILD_PT, CTX));\
}while(0)

#define SERIALPT_PRINT(PT, CHILD_PT, CTX, BUF)\
do{\
  (CTX)->buf = BUF;\
  EPT_SPAWN(PT, CHILD_PT, serialpt_write(CHILD_PT, CTX));\
}while(0)

#endif
