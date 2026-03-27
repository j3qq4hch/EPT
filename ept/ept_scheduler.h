#ifndef __EPT_SCHEDULER
#define __EPT_SCHEDULER

#include <stddef.h>
#include <stdint.h>
#include "ept.h"

typedef char (*ept_thread)(struct ept * ept);

//debug levels
#define TRIVIAL         1
#define IMPORTANT       2
#define CRITICAL        3
#define OFF             0xFF

typedef struct
{   
    const char* name;
    ept_thread thread;
    void * context;
    ept_state_t init_state;
    struct ept * ept;
    uint8_t init_dbg_level;
}thread_record;

extern thread_record const* active_thread;
extern uint8_t active_dbg_level;
extern void *thread_ctx;
void ept_scheduler();

const thread_record* thread_by_name(char * name);
const thread_record* thread_by_index(uint8_t index);
uint8_t thread_index_by_name(char * name);
uint8_t thread_record_snprint(char* buf, uint16_t buflen,
                              const thread_record* record);
void thread_cmd(const thread_record* record, ept_state_t state);
void thread_set_debug(const thread_record* record, uint8_t lvl);
void signal_thread_by_index(uint8_t index, uint8_t signal);

uint32_t loop_get_exec_time_us();
uint32_t thread_get_exec_time_us(uint8_t index);
void reset_profiler();
#endif