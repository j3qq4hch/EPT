#include <string.h>
#include "ept_scheduler.h"

#define EPT_IMPLEMENTATION
#define EPT_SCHEDULER_IMPLEMENTATION
#include "ept_cfg.h"

#ifdef LOW_POWER_MODE
#include "drv_basic.h" // drv_sleep()
#endif

#include "snprintf_compat.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define THREADNUM       ARRAY_SIZE(threadlist)

void                *thread_ctx;
const thread_record *active_thread   = NULL;
uint8_t              active_dbg_level;
uint8_t              dbg_level[THREADNUM];
static struct ept    ept_arr[THREADNUM];

// ---- Profiler ---------------------------------------------------------------
// Requires CMSIS (SysTick->VAL, SysTick->LOAD).
// Include the platform CMSIS header in ept_cfg.h under #ifdef PROFILER.
#ifdef PROFILER
#ifndef EPT_CPU_FREQ_HZ
#error "EPT_CPU_FREQ_HZ is not defined. Add it to your build system: -DEPT_CPU_FREQ_HZ=48000000UL"
#endif
typedef struct {
    uint32_t ept_tick;
    uint32_t sys_tick;
} timestamp_t;

static timestamp_t loop_begin_ts;
static timestamp_t loop_end_ts;
static timestamp_t thread_entry_ts;
static timestamp_t thread_yield_ts;

static uint32_t loop_max_raw              = 0;
static uint32_t thread_max_raw[THREADNUM] = {0};
static uint32_t profiler_dif;

// Retry if SysTick fires between the two reads — order is critical.
static inline void get_timestamp(timestamp_t *t)
{
    do {
        t->ept_tick = ept_tc;
        t->sys_tick = SysTick->VAL;
    } while (t->ept_tick != ept_tc);
}

static uint32_t timestamp_dif_ticks(timestamp_t *start, timestamp_t *stop)
{
    uint32_t ticks_per_ms = SysTick->LOAD + 1;
    uint32_t c = stop->ept_tick - start->ept_tick;
    c *= ticks_per_ms;
    c += start->sys_tick;
    c -= stop->sys_tick;
    return c;
}
#endif // PROFILER

// ---- Low-power mode ---------------------------------------------------------
#ifdef LOW_POWER_MODE
static uint8_t lp_any_active;
#endif

// ---- Scheduler --------------------------------------------------------------
void ept_scheduler(void)
{
    for (uint8_t i = 0; i < THREADNUM; i++) {
        EPT_INIT(&ept_arr[i]);
        ept_arr[i].state = threadlist[i].init_state;
        dbg_level[i]     = threadlist[i].init_dbg_level;
    }

    while (1) {
#ifdef LOW_POWER_MODE
        lp_any_active = 0;
#endif
#ifdef PROFILER
        get_timestamp(&loop_begin_ts);
#endif

        for (uint8_t i = 0; i < THREADNUM; i++) {
            if (ept_arr[i].state == RUN) {
                active_thread    = &threadlist[i];
                active_dbg_level = dbg_level[i];
                thread_ctx       = threadlist[i].context;

#ifdef PROFILER
                get_timestamp(&thread_entry_ts);
#endif

#ifdef LOW_POWER_MODE
                if (threadlist[i].thread(&ept_arr[i]) == EPT_ACTIVE)
                    lp_any_active = 1;
#else
                threadlist[i].thread(&ept_arr[i]);
#endif

#ifdef PROFILER
                get_timestamp(&thread_yield_ts);
                profiler_dif = timestamp_dif_ticks(&thread_entry_ts, &thread_yield_ts);
                if (profiler_dif > thread_max_raw[i])
                    thread_max_raw[i] = profiler_dif;
#endif
            }
        }

#ifdef PROFILER
        get_timestamp(&loop_end_ts);
        profiler_dif = timestamp_dif_ticks(&loop_begin_ts, &loop_end_ts);
        if (profiler_dif > loop_max_raw)
            loop_max_raw = profiler_dif;
#endif

        // Any wakeup source (SysTick included) is an interrupt, so the sleep
        // is bounded by one tick and every blocked condition is re-polled at
        // tick rate — nothing can be missed.
#ifdef LOW_POWER_MODE
        if (!lp_any_active)
            drv_sleep();
#endif
    }
}

// ---- Utilities --------------------------------------------------------------
static const char *const state_str[] = {"STOP", "RUN"};

uint16_t thread_record_snprint(char *buf, uint16_t buflen, const thread_record *record)
{
    uint32_t index = (uint32_t)(record - threadlist);
    return (uint16_t)__SNPRINTF(buf, buflen, "%i:%-8s:%-4s dbg:%i\r\n",
                                 (unsigned)index,
                                 record->name,
                                 state_str[ept_arr[index].state],
                                 (unsigned)dbg_level[index]);
}

const thread_record *thread_by_name(const char *name)
{
    for (uint8_t i = 0; i < THREADNUM; i++) {
        if (strcmp(threadlist[i].name, name) == 0)
            return &threadlist[i];
    }
    return NULL;
}

const thread_record *thread_by_index(uint8_t index)
{
    if (index < THREADNUM)
        return &threadlist[index];
    return NULL;
}

uint8_t thread_index_by_name(const char *name)
{
    for (uint8_t i = 0; i < THREADNUM; i++) {
        if (strcmp(threadlist[i].name, name) == 0)
            return i;
    }
    return EPT_INVALID_THREAD;
}

void thread_cmd(const thread_record *record, ept_state_t state)
{
    if (record == NULL) return;
    uint32_t index = (uint32_t)(record - threadlist);
    ept_arr[index].state = state;
    EPT_INIT(&ept_arr[index]);
}

void thread_set_debug(const thread_record *record, uint8_t lvl)
{
    uint32_t index = (uint32_t)(record - threadlist);
    dbg_level[index] = lvl;
}

// ---- Profiler API -----------------------------------------------------------
#ifdef PROFILER
uint32_t thread_get_exec_time_us(uint8_t index)
{
    if (index < THREADNUM)
        return thread_max_raw[index] / (EPT_CPU_FREQ_HZ / 1000000UL);
    return UINT32_MAX;
}

uint32_t loop_get_exec_time_us(void)
{
    return loop_max_raw / (EPT_CPU_FREQ_HZ / 1000000UL);
}

void reset_profiler(void)
{
    loop_max_raw = 0;
    for (uint8_t i = 0; i < THREADNUM; i++)
        thread_max_raw[i] = 0;
}
#endif // PROFILER

// ---- CLI commands -----------------------------------------------------------
#include <stdlib.h>
#include "ept_cli.h"

static uint16_t ept_list_gen(uint16_t state, char *buf, uint16_t buflen)
{
    const thread_record *t = thread_by_index((uint8_t)state);
    if (t == NULL) return 0;
    return (uint16_t)thread_record_snprint(buf, buflen, t);
}

CLI_retval_t ept_cmd(char *args[], uint8_t argno)
{
    if (argno == 0) {
        cli_gen = ept_list_gen;
        return CLI_LONG_RESPONSE;
    }
    const thread_record *t = thread_by_name(args[0]);
    if (t == NULL) {
        __SNPRINTF(cli_response_buf, cli_max_resp_len, "\r\nUnknown thread: %s", args[0]);
        return CLI_ERROR;
    }
    if (argno < 2) return CLI_ERROR;
    if      (strcmp(args[1], "run")  == 0) thread_cmd(t, RUN);
    else if (strcmp(args[1], "stop") == 0) thread_cmd(t, STOP);
    else return CLI_ERROR;
    return CLI_OK;
}

CLI_retval_t dbg_cmd(char *args[], uint8_t argno)
{
    if (argno < 2) return CLI_ERROR;
    const thread_record *t = thread_by_name(args[0]);
    if (t == NULL) {
        __SNPRINTF(cli_response_buf, cli_max_resp_len, "\r\nUnknown thread: %s", args[0]);
        return CLI_ERROR;
    }
    thread_set_debug(t, (uint8_t)atoi(args[1]));
    return CLI_OK;
}

#ifdef PROFILER
static uint16_t prof_gen(uint16_t state, char *buf, uint16_t buflen)
{
    const thread_record *t = thread_by_index((uint8_t)state);
    if (t != NULL)
        return (uint16_t)__SNPRINTF(buf, buflen, "%-8s %i us\r\n",
                                    t->name,
                                    (int)thread_get_exec_time_us(state));
    if (thread_by_index((uint8_t)(state - 1)) != NULL)
        return (uint16_t)__SNPRINTF(buf, buflen, "loop     %i us\r\n",
                                    (int)loop_get_exec_time_us());
    return 0;
}

CLI_retval_t prof_cmd(char *args[], uint8_t argno)
{
    (void)args; (void)argno;
    reset_profiler();
    cli_gen = prof_gen;
    return CLI_LONG_RESPONSE;
}
#endif // PROFILER
