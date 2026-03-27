#ifndef __EPT_CFG
#define __EPT_CFG

// This is user-provided file

// These definitions are to be configured for a specific case
// It is presumed here that ANY system has at least one debug CLI
//#define CLI_BUFLEN         192
//#define CLI_MAX_ARGNUM     5
//#define CLI_SERIAL_PTR     (&SERIAL1)
////........................................................////
//#define PROFILER                     //Uncomment this if profiler needed
#define LOW_POWER_MODE

#ifdef LOW_POWER_MODE
extern volatile uint8_t tc_flag;
#define EPT_TICK() { ept_timer_tick(); tc_flag = 1; }
#else
#define EPT_TICK() ept_timer_tick();
#endif

#ifdef EPT_SCHEDULER_IMPLEMENTATION

//threadlist array must be filled manually by the user 
//All the top level threads must be placed in this array
#include "blinker.h"

extern blinker_ctx blctx1, blctx2; //Conetxts for threads.

const static thread_record threadlist[] = 
{
  {.name = "bl1", .thread = blinker,  .context = &blctx1, .init_state = RUN, .init_dbg_level = OFF}, 
  {.name = "bl2", .thread = blinker,  .context = &blctx2, .init_state = RUN, .init_dbg_level = OFF}
};

#endif
#endif