#ifndef LOG_H_
#define LOG_H_

// Log levels — used in LOG_PRINTF and in thread_record.init_dbg_level.
// Higher value = more important. OFF silences all output for that thread.
#define TRIVIAL   1
#define IMPORTANT 2
#define CRITICAL  3
#define OFF       0xFF

#include "snprintf_compat.h"

// ept_cfg.h must provide:
//   DEBUG_PRINT(str)      — output function (e.g. serial_write); if absent,
//                           logging is fully disabled and all macros compile
//                           to nothing.
// (ept_cfg.h also includes ept_scheduler.h, which LOG_PRINTF needs for
//  per-thread level filtering via active_dbg_level.)
#include "ept_cfg.h"

#define DBGBUF_LEN 128

// ---- Usage in a .c file ----------------------------------------------------
//
// For plain debug output (no per-thread filtering):
//   #define DBG_PREFIX "my_module"
//   #include "log.h"
//
// For per-thread level filtering (uses the active thread's name/level):
//   #define DBG_PREFIX active_thread->name
//   #include "log.h"
//
// ept_cfg.h (pulled in by log.h) always includes ept_scheduler.h, so
// LOG_PRINTF's per-thread filtering via active_dbg_level is always available.
// ----------------------------------------------------------------------------

#if defined(DBG_PREFIX) && defined(DEBUG_PRINT)

#define DBG_PRINTF(fmt, ...) \
    do { \
        __SNPRINTF(dbgbuf, DBGBUF_LEN, "[%s]" fmt, DBG_PREFIX, ##__VA_ARGS__); \
        DEBUG_PRINT(dbgbuf); \
    } while (0)

#ifdef EPT_SCHEDULER_H_

#define LOG_PRINTF(lvl, fmt, ...) \
    do { \
        if ((lvl) >= active_dbg_level) { \
            __SNPRINTF(dbgbuf, DBGBUF_LEN, "[%s]" fmt, DBG_PREFIX, ##__VA_ARGS__); \
            DEBUG_PRINT(dbgbuf); \
        } \
    } while (0)

#else
// Scheduler not included — level filtering unavailable, always print.
#define LOG_PRINTF(lvl, fmt, ...) DBG_PRINTF(fmt, ##__VA_ARGS__)
#endif

#else

#define DBG_PRINTF(fmt, ...)
#define LOG_PRINTF(lvl, fmt, ...)

#endif // DBG_PREFIX && DEBUG_PRINT

extern char dbgbuf[DBGBUF_LEN];

#endif // LOG_H_

#ifdef DBG_IMPLEMENTATION
#ifndef DBG_IMPLEMENTATION_DONE_
#define DBG_IMPLEMENTATION_DONE_
char dbgbuf[DBGBUF_LEN];
#endif
#endif
