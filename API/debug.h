#ifndef __DEBUG
#define __DEBUG

#ifndef __SNPRINTF 
#include <stdio.h>
#define __SNPRINTF snprintf
#else
#include "eprintf_stb.h"
#endif

#include "resources.h"
#include "ept_scheduler.h"

#define DBGBUF_LEN   128

#if defined(GLOBAL_DEBUG_FLAG)
#ifndef DEBUG_PRINT
#error "Define debug DEBUG_PRINT function"
#endif
#endif

#define PRINT(str) serial_write(&SERIAL1, str, BLOCKING);

#if defined (DBG_PREFIX) && defined(GLOBAL_DEBUG_FLAG)

#define DBG_PRINTF(fmt,  ...)\
do{\
  __SNPRINTF(dbgbuf, DBGBUF_LEN, "[%s]"fmt, DBG_PREFIX, ##__VA_ARGS__);\
  DEBUG_PRINT(dbgbuf);\
}while(0)

#define EPT_DBG_PRINTF(lvl, fmt,  ...)\
if(lvl + 1> active_dbg_level){\
  __SNPRINTF(dbgbuf, DBGBUF_LEN, "[%s]"fmt, DBG_PREFIX, ##__VA_ARGS__);\
  DEBUG_PRINT(dbgbuf);\
}

#else

#define DBG_PRINTF(fmt,  ...)
#define EPT_DBG_PRINTF(fmt,  ...)

#endif //DBG_PREFIX

extern char dbgbuf[DBGBUF_LEN];

#endif //__DEBUG

#ifdef DBG_IMPLEMENTATION
#ifndef __DBG_IMPLEMENTATION__
#define __DBG_IMPLEMENTATION__
char dbgbuf[DBGBUF_LEN];

#endif
#endif //DBG_IMPLEMENTATION


//set active_dbg_level to 0xFF to disable debug output at all
//set it 0 to enable all

