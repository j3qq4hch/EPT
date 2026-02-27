#ifndef __EPT_SCHEDULER
#define __EPT_SCHEDULER

#include <stddef.h>
#include <stdint.h>
#include "ept.h"

#define PROFILER

//In order for low power mode to work all the threads that are waiting for INTERRUPT-triggered event should wait for it using EPT_WAIT_UNTIL which returns PT_WAITING
//If the event is not triggered by interrupt or the thread is waiting for another thread to finish it should wait using EPT_YIELD_UNTIL which return PT_YIELDED

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

void ept_scheduler();

const thread_record* thread_by_name(char * name);
const thread_record* thread_by_index(uint8_t index);
uint8_t thread_index_by_name(char * name);
uint8_t thread_record_snprintf(char* buf, uint16_t buflen,
                               const thread_record* record);
void thread_cmd(const thread_record* record, ept_state_t state);
void thread_set_debug(const thread_record* record, uint8_t lvl);
void signal_thread_by_index(uint8_t index, uint8_t signal);

uint32_t loop_get_exec_time_us();
uint32_t thread_get_exec_time_us(uint8_t index);
void reset_profiler();
#endif