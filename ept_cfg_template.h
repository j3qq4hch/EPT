#ifndef EPT_CFG_H_
#define EPT_CFG_H_

// ---- Tick frequency ---------------------------------------------------------
// Must match the SysTick ISR call rate.
#define EPT_TICK_FREQ_HZ  1000

// ---- Optional features ------------------------------------------------------
// #define PROFILER        // measure per-thread and loop execution time
// #define LOW_POWER_MODE  // enter WFI when all threads are idle

// ---- Profiler: platform CMSIS header ----------------------------------------
// EPT_CPU_FREQ_HZ must be defined in your build system, NOT here:
//   e.g. -DEPT_CPU_FREQ_HZ=48000000UL
// Include the CMSIS header that exposes SysTick registers:
#ifdef PROFILER
// #include "stm32f0xx.h"  // replace with your platform header
#endif

// ---- Thread list ------------------------------------------------------------
#ifdef EPT_SCHEDULER_IMPLEMENTATION

// Forward-declare all top-level thread functions:
// EPT_THREAD(my_thread(struct ept *ept));

// Define thread contexts if needed:
// my_ctx_t my_ctx = { ... };

// init_dbg_level: 0 = off (no debug output for this thread).
// See log.h for named level constants and further details.
const static thread_record threadlist[] = {
//  { .name = "name", .thread = my_thread, .context = NULL,    .init_state = RUN, .init_dbg_level = 0 },
//  { .name = "name", .thread = my_thread, .context = &my_ctx, .init_state = RUN, .init_dbg_level = 0 },
};

#endif // EPT_SCHEDULER_IMPLEMENTATION
#endif // EPT_CFG_H_
