#ifndef __EPT 
#define __EPT

#include "pt.h"
#include <stdint.h>
#define EPT_INVALID_THREAD      0xFF
#define EPT_TICK_FREQ_HZ        1000
#define EPT_TICK_PERIOD_mS      (1000 / EPT_TICK_FREQ_HZ)

// #define PT_WAITING 0 //means that thread is waiting for some condition. Can sleep
// #define PT_YIELDED 1 //Thread is active and has something to do
// #define PT_EXITED  2 //can never happen with schedulable threads
// #define PT_ENDED   3 //can never happen with schedulable threads

typedef struct
{
  uint32_t set_time; 
  uint32_t interval; //If this is 0, the timer is not set
}ept_timer_t;

extern volatile uint32_t ept_tc; //tick counter  

static inline void ept_timer_tick(){ ept_tc += EPT_TICK_PERIOD_mS; }

static inline void ept_timer_set(ept_timer_t * t, uint32_t interval_ms)
{
  t->set_time = ept_tc;
  t->interval = interval_ms;
}

static inline uint8_t ept_is_timer_set(ept_timer_t * t)
{
  return (t->interval) ? 1 : 0; 
}

static inline uint8_t ept_timer_expired(ept_timer_t * t)
{
  return (ept_tc - t->set_time >= t->interval) ? 1 : 0;
}

typedef enum {STOP = 0, RUN = 1} ept_state_t;

struct ept
{
 // const char* name;
  lc_t    lc;       
  ept_timer_t t;
  uint8_t event;    
  void*   ctx;   
  ept_state_t state;   
};

#define EPT_BEGIN       PT_BEGIN
#define EPT_END         PT_END
#define EPT_THREAD      PT_THREAD
                
#define EPT_WAIT_UNTIL(ept, condition)  \
  do {                                  \
    LC_SET((ept)->lc);                  \
    if(!(condition)) {                  \
      return PT_WAITING;                \
    }                                   \
  } while(0)                  

#define EPT_WAIT_UNTIL_TIMEOUT(ept, condition, timeout_ms)  \
  do {                                   \
    ept_timer_set(&(ept)->t, timeout_ms);\
    LC_SET((ept)->lc);                   \
    if(!(condition)&&(!ept_timer_expired(&(ept)->t))){\
      return PT_WAITING;                 \
    }                                    \
  } while(0)                  

#define EPT_WAIT_WHILE(pt, cond)  EPT_WAIT_UNTIL((pt), !(cond))

#define EPT_YIELD_UNTIL(pt, cond) \
  do {                            \
    PT_YIELD_FLAG = 0;            \
    LC_SET((pt)->lc);             \
    if((PT_YIELD_FLAG == 0) ||    \
       (!(cond))){                \
      return PT_YIELDED;          \
    }                             \
  } while(0)
    
#define EPT_YIELD_WHILE(pt, cond)  EPT_YIELD_UNTIL((pt), !(cond))

#define EPT_WAIT_THREAD(ept, thread) EPT_YIELD_WHILE((ept), PT_SCHEDULE(thread))

#define EPT_SPAWN(ept, ept_child, thread)       \
  do {                                          \
    PT_INIT((ept_child));                       \
    EPT_WAIT_THREAD((ept), (thread));           \
  } while(0)

#define EPT_SLEEP(ept, time)                            \
  do {                                                  \
     ept_timer_set(&(ept)->t, time);                    \
     EPT_WAIT_UNTIL(ept, ept_timer_expired(&(ept)->t)); \
     ept->t.interval = 0;                               \
    } while(0)                                    

#define EPT_INIT(ept)  PT_INIT(ept)

#ifdef EPT_IMPLEMENTATION
       
volatile uint32_t ept_tc = 0; //tick counter

#endif
#endif