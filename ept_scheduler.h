#ifndef EPT_SCHEDULER_H_
#define EPT_SCHEDULER_H_

#include <stdint.h>
#include "ept.h"

typedef char (*ept_thread)(struct ept *ept);

typedef struct {
    const char    *name;
    ept_thread     thread;
    void          *context;
    ept_state_t    init_state;
    uint8_t        init_dbg_level;
} thread_record;

extern const thread_record *active_thread;
extern uint8_t              active_dbg_level;
extern void                *thread_ctx;

void ept_scheduler(void);

#ifdef LOW_POWER_MODE
extern volatile uint8_t ept_tc_flag;
#endif

static inline void ept_scheduler_tick(void) {
    ept_tick();
#ifdef LOW_POWER_MODE
    ept_tc_flag = 1;
#endif
}

const thread_record *thread_by_name(const char *name);
const thread_record *thread_by_index(uint8_t index);
uint8_t              thread_index_by_name(const char *name);
uint16_t             thread_record_snprint(char *buf, uint16_t buflen,
                                           const thread_record *record);
void thread_cmd(const thread_record *record, ept_state_t state);
void thread_set_debug(const thread_record *record, uint8_t lvl);

#ifdef PROFILER
uint32_t loop_get_exec_time_us(void);
uint32_t thread_get_exec_time_us(uint8_t index);
void     reset_profiler(void);
#endif

#endif // EPT_SCHEDULER_H_
