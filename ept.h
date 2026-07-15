#ifndef EPT_H_
#define EPT_H_

// EPT - Extended ProtoThreads
// Single-header STB library.
// Define EPT_IMPLEMENTATION in exactly one translation unit before including.
// Define EPT_TICK_FREQ_HZ in preprocessor settings (default: 1000 Hz).

#include "pt.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef EPT_TICK_FREQ_HZ
#define EPT_TICK_FREQ_HZ 1000
#pragma message("EPT_TICK_FREQ_HZ not defined. Defaulting to 1000 Hz.")
#endif
#define EPT_TICK_PERIOD_MS (1000 / EPT_TICK_FREQ_HZ)

#define EPT_INVALID_THREAD 0xFF

// Aliases so EPT threads don't mix PT_ and EPT_ prefixes
#define EPT_BEGIN  PT_BEGIN
#define EPT_END    PT_END
#define EPT_THREAD PT_THREAD

#define EPT_WAITING  PT_WAITING  // 0: waiting for condition, may sleep
#define EPT_YIELDED  PT_YIELDED  // 1: active, has more work this cycle
#define EPT_SLEEPING 2           // sleeping until internal timer fires (still running)
#define EPT_EXITED   PT_EXITED   // 3: unused with schedulable threads
#define EPT_ENDED    PT_ENDED    // 4: unused with schedulable threads

// ---- Timer ----------------------------------------------------------------

typedef struct {
    uint32_t set_time;
    uint32_t interval; // 0 = not armed
} ept_timer_t;

extern volatile uint32_t ept_tc; // tick counter in milliseconds

// Call from SysTick ISR at EPT_TICK_FREQ_HZ rate.
static inline void ept_tick(void) {
    ept_tc += EPT_TICK_PERIOD_MS;
}

static inline void ept_timer_set(ept_timer_t *t, uint32_t interval_ms) {
    t->set_time = ept_tc;
    t->interval = interval_ms;
}

static inline bool ept_timer_armed(const ept_timer_t *t) {
    return t->interval != 0;
}

static inline bool ept_timer_expired(const ept_timer_t *t) {
    return (ept_tc - t->set_time) >= t->interval;
}

// ---- Thread control structure ---------------------------------------------

typedef enum { STOP = 0, RUN = 1 } ept_state_t;

struct ept {
    lc_t        lc;
    ept_timer_t t;
    ept_state_t state;
};

// ---- Macros ---------------------------------------------------------------

// Wait until condition is true.
#define EPT_WAIT_UNTIL(ept, condition) \
    do {                               \
        LC_SET((ept)->lc);             \
        if (!(condition)) {            \
            return EPT_WAITING;        \
        }                              \
    } while (0)

#define EPT_WAIT_WHILE(ept, condition) EPT_WAIT_UNTIL(ept, !(condition))

// Wait until condition OR timeout.
// The timer is set BEFORE LC_SET, so it is set only on the first entry.
#define EPT_WAIT_UNTIL_TIMEOUT(ept, condition, timeout_ms)         \
    do {                                                           \
        ept_timer_set(&(ept)->t, timeout_ms);                     \
        LC_SET((ept)->lc);                                        \
        if (!(condition) && !ept_timer_expired(&(ept)->t)) {      \
            return EPT_WAITING;                                    \
        }                                                          \
        (ept)->t.interval = 0;                                    \
    } while (0)

// Sleep for time_ms milliseconds.
#define EPT_SLEEP(ept, time_ms)                      \
    do {                                             \
        ept_timer_set(&(ept)->t, time_ms);           \
        LC_SET((ept)->lc);                           \
        if (!ept_timer_expired(&(ept)->t)) {         \
            return EPT_SLEEPING;                     \
        }                                            \
        (ept)->t.interval = 0;                       \
    } while (0)

#define EPT_YIELD(ept) PT_YIELD(ept)

#define EPT_YIELD_UNTIL(pt, cond)     \
    do {                              \
        PT_YIELD_FLAG = 0;            \
        LC_SET((pt)->lc);             \
        if ((PT_YIELD_FLAG == 0) ||   \
            (!(cond))) {              \
            return EPT_YIELDED;       \
        }                             \
    } while (0)

#define EPT_YIELD_WHILE(pt, cond) EPT_YIELD_UNTIL(pt, !(cond))

#define EPT_WAIT_THREAD(ept, thread) EPT_YIELD_WHILE((ept), PT_SCHEDULE(thread))

#define EPT_SPAWN(ept, child, thread)  \
    do {                               \
        PT_INIT((child));              \
        EPT_WAIT_THREAD((ept), (thread)); \
    } while (0)

#define EPT_INIT(ept) PT_INIT(ept)

#define EPT_RESTART(ept) PT_RESTART(ept)

#endif // EPT_H_

#ifdef EPT_IMPLEMENTATION
#ifndef EPT_IMPLEMENTATION_DONE_
#define EPT_IMPLEMENTATION_DONE_
volatile uint32_t ept_tc = 0;
#endif
#endif
