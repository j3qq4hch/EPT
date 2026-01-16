#ifndef __EPT
#define __EPT

#include "pt.h"
#include <stdint.h>

#define EPT_TICK_PERIOD_mS  (1000/EPT_TICK_FREQ_HZ)
typedef struct
{
  uint32_t set_time;
  uint32_t interval;
}ept_timer_t;

extern volatile uint32_t ept_tc; //tick counter  
void ept_timer_tick();

static inline void ept_timer_set(ept_timer_t * t, uint32_t interval_ms)
{
  t->set_time = ept_tc;
  t->interval = interval_ms;
}

static inline uint8_t ept_timer_expired(ept_timer_t * t)
{
    return (ept_tc - t->set_time >= t->interval) ? 1 : 0;
}

typedef enum {STOP = 0, RUN = 1} ept_state_t;

struct ept
{
  lc_t    lc;       
  ept_timer_t t;
  uint8_t event;    
  void*   ctx;   
  const char* name;
  ept_state_t state;   
};
                
#define EPT_WAIT_UNTIL(ept, condition)  \
  do {                                  \
    LC_SET((ept)->lc);                  \
    if(!(condition)&&(!(ept)->event)) { \
      return PT_WAITING;                \
    }                                   \
    if((ept)->event) break;             \
  } while(0)                  

#define EPT_WAIT_WHILE(ept, cond)  EPT_WAIT_UNTIL((ept), !(cond))

#define EPT_WAIT_THREAD(ept, thread) EPT_WAIT_WHILE((ept), PT_SCHEDULE(thread))

#define EPT_SPAWN(ept, ept_child, thread)       \
  do {                                          \
    PT_INIT((ept_child));                       \
    EPT_WAIT_THREAD((pt), (thread));            \
  } while(0)


#define EPT_SLEEP(ept, time)                           \
  do {                                                 \
     ept_timer_set(&(ept)->t, time);                   \
     EPT_WAIT_UNTIL(ept, ept_timer_expired(&(ept)->t));\
     } while(0)                                    

#define EPT_SPIN(ept, f)                \
     do{                                \
      if((ept)->state == RUN) (f);      \
     }while(0)

#ifdef EPT_TICK_FREQ_HZ
volatile uint32_t ept_tc = 0; //tick counter
void ept_timer_tick(){  ept_tc += EPT_TICK_PERIOD_mS; }
#endif
#endif