#ifndef EPT_H_
#define EPT_H_

// EPT - Extended ProtoThreads
// Cooperative threads built on local continuations (switch-based, see lc.h).
// Single-header STB library.
// Define EPT_IMPLEMENTATION in exactly one translation unit before including.
// Define EPT_TICK_FREQ_HZ in preprocessor settings (default: 1000 Hz).
//
// Restrictions (switch-based LC):
//  - no switch() statements inside a thread body across a blocking macro
//  - two blocking macros must not share one source line
//  - locals don't survive blocking points: use statics or the context pointer

#include "lc.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef EPT_TICK_FREQ_HZ
#define EPT_TICK_FREQ_HZ 1000
#pragma message("EPT_TICK_FREQ_HZ not defined. Defaulting to 1000 Hz.")
#endif
#define EPT_TICK_PERIOD_MS (1000 / EPT_TICK_FREQ_HZ)

#define EPT_INVALID_THREAD 0xFF

// ---- Thread status ----------------------------------------------------------
// Returned by every thread invocation. Order matters: EPT_ALIVE relies on
// "running < terminal".
#define EPT_ACTIVE  0 // has more work this pass - scheduler must not sleep
#define EPT_BLOCKED 1 // waiting on a condition or timer - MCU may sleep until next tick/IRQ
#define EPT_DONE    2 // terminal: thread ran to completion (meaningful for spawned children)

#define EPT_ALIVE(status) ((status) < EPT_DONE)

// ---- Timer ------------------------------------------------------------------
// Relative form (set_time + interval) is uint32 wrap-safe.

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

// ---- Thread control structure -----------------------------------------------

typedef enum { STOP = 0, RUN = 1 } ept_state_t; // scheduler on/off switch, not thread status

struct ept {
    lc_t        lc;
    ept_timer_t t;
    ept_state_t state;
};

// ---- Thread definition ------------------------------------------------------

#define EPT_THREAD(name_args) char name_args

#define EPT_INIT(ept) LC_INIT((ept)->lc)

#define EPT_BEGIN(ept) { char __attribute__((unused)) EPT_YIELD_FLAG = 1; LC_RESUME((ept)->lc)

#define EPT_END(ept)              \
        LC_END((ept)->lc);        \
        EPT_YIELD_FLAG = 0;       \
        EPT_INIT(ept);            \
        return EPT_DONE; }

// ---- Blocking waits (report EPT_BLOCKED - MCU may sleep) ----------------------

// Wait until condition is true.
#define EPT_WAIT_UNTIL(ept, condition) \
    do {                               \
        LC_SET((ept)->lc);             \
        if (!(condition)) {            \
            return EPT_BLOCKED;        \
        }                              \
    } while (0)

#define EPT_WAIT_WHILE(ept, condition) EPT_WAIT_UNTIL(ept, !(condition))

// Wait until condition OR timeout.
// The timer is set BEFORE LC_SET, so it is set only on the first entry.
#define EPT_WAIT_UNTIL_TIMEOUT(ept, condition, timeout_ms)     \
    do {                                                       \
        ept_timer_set(&(ept)->t, timeout_ms);                  \
        LC_SET((ept)->lc);                                     \
        if (!(condition) && !ept_timer_expired(&(ept)->t)) {   \
            return EPT_BLOCKED;                                \
        }                                                      \
        (ept)->t.interval = 0;                                 \
    } while (0)

// Sleep for time_ms milliseconds.
#define EPT_SLEEP(ept, time_ms)                  \
    do {                                         \
        ept_timer_set(&(ept)->t, time_ms);       \
        LC_SET((ept)->lc);                       \
        if (!ept_timer_expired(&(ept)->t)) {     \
            return EPT_BLOCKED;                  \
        }                                        \
        (ept)->t.interval = 0;                   \
    } while (0)

// ---- Yields (report EPT_ACTIVE - more work this pass) -------------------------

// Give other threads one pass, then continue.
#define EPT_YIELD(ept)                  \
    do {                                \
        EPT_YIELD_FLAG = 0;             \
        LC_SET((ept)->lc);              \
        if (EPT_YIELD_FLAG == 0) {      \
            return EPT_ACTIVE;          \
        }                               \
    } while (0)

// Yield at least once, then block until condition is true.
#define EPT_YIELD_UNTIL(ept, condition) \
    do {                                \
        EPT_YIELD_FLAG = 0;             \
        LC_SET((ept)->lc);              \
        if (EPT_YIELD_FLAG == 0) {      \
            return EPT_ACTIVE;          \
        }                               \
        if (!(condition)) {             \
            return EPT_BLOCKED;         \
        }                               \
    } while (0)

#define EPT_YIELD_WHILE(ept, condition) EPT_YIELD_UNTIL(ept, !(condition))

// ---- Child threads ------------------------------------------------------------

// Run a child thread to completion, propagating its status (ACTIVE/BLOCKED)
// to the parent - and thus to the scheduler's sleep decision.
// If the child completes on the first call, the parent continues immediately.
#define EPT_WAIT_THREAD(ept, thread)                  \
    do {                                              \
        LC_SET((ept)->lc);                            \
        {                                             \
            char ept_child_status_ = (thread);        \
            if (EPT_ALIVE(ept_child_status_)) {       \
                return ept_child_status_;             \
            }                                         \
        }                                             \
    } while (0)

// Initialize and run a child thread to completion.
#define EPT_SPAWN(ept, child, thread)     \
    do {                                  \
        EPT_INIT(child);                  \
        EPT_WAIT_THREAD((ept), (thread)); \
    } while (0)

// ---- Control ------------------------------------------------------------------

// Restart the thread from EPT_BEGIN on the next pass.
#define EPT_RESTART(ept)        \
    do {                        \
        EPT_INIT(ept);          \
        return EPT_ACTIVE;      \
    } while (0)

// Terminate the thread (as if it reached EPT_END).
#define EPT_EXIT(ept)           \
    do {                        \
        EPT_INIT(ept);          \
        return EPT_DONE;        \
    } while (0)

#endif // EPT_H_

#ifdef EPT_IMPLEMENTATION
#ifndef EPT_IMPLEMENTATION_DONE_
#define EPT_IMPLEMENTATION_DONE_
volatile uint32_t ept_tc = 0;
#endif
#endif
