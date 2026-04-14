
#include "ept_scheduler.h"
#define EPT_SCHEDULER_IMPLEMENTATION
#include "ept_cfg.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define THREADNUM        ARRAY_SIZE(threadlist)
void * thread_ctx;
thread_record const* active_thread = NULL;
uint8_t active_dbg_level;
uint8_t dbg_level[THREADNUM];
static struct ept ept[THREADNUM];

#ifdef PROFILER
    #include "stm32f0xx.h"

    typedef struct
    {
      uint32_t ept_tick;
      uint32_t sys_tick;
    }timestamp_t;

    static timestamp_t loop_begin_timestamp;
    static timestamp_t loop_end_timestamp;
    static timestamp_t thread_entry_timestamp;
    static timestamp_t thread_yeild_timestamp;

    static uint32_t loop_max_exec_time_raw = 0;
    static uint32_t thread_max_exec_time_raw[THREADNUM] = {0};
    static uint32_t dif;
    static uint32_t SysTick_ARR;

    static inline void get_timestamp(timestamp_t * t)
    {
      do{
        t->ept_tick = ept_tc; // ORDER OF THESE TWO OPERATIONS IS CRITICAL AND MUST NOT BE CHANGED
        t->sys_tick = SysTick->VAL;
      }while(t->ept_tick != ept_tc);
    }

    static uint32_t timestamp_dif_ticks(timestamp_t * start, timestamp_t *stop)
    {
      volatile uint32_t c;
      c = stop->ept_tick - start->ept_tick;
      c *= SysTick_ARR;
      c += start->sys_tick; 
      c -= stop->sys_tick;
      return c;
    }
#endif //#ifdef PROFILER

#ifdef LOW_POWER_MODE
static uint32_t wake_time;
static uint32_t minimal_wake_time; 
ept_timer_t sleep_timer;
uint8_t volatile tc_flag;
static uint8_t state;
static uint16_t active_tasks;
#endif

void ept_scheduler()
{
#ifdef PROFILER
    SysTick_ARR = SysTick->LOAD + 1;
#endif
    for(uint8_t i = 0; i < THREADNUM; i++) 
    {
      PT_INIT(&ept[i]);
      ept[i].state = threadlist[i].init_state;
      dbg_level[i] = threadlist[i].init_dbg_level;
    }
    while(1)
    {    
#ifdef LOW_POWER_MODE
      active_tasks = 0;  
      minimal_wake_time = UINT32_MAX;
#endif
#ifdef PROFILER
        get_timestamp(&loop_begin_timestamp);
#endif
        for(uint8_t i = 0; i < THREADNUM; i++)
        {    
            if(ept[i].state == RUN)
            { 
              active_thread = &threadlist[i];
              active_dbg_level = dbg_level[i];
              thread_ctx = threadlist[i].context;
#ifdef PROFILER
              get_timestamp(&thread_entry_timestamp);
#endif

#ifdef LOW_POWER_MODE
              state = threadlist[i].thread(&ept[i]); //Thread execution               
              if(state == EPT_SLEEPING) 
              {
                wake_time = ept[i].t.interval + ept[i].t.set_time - ept_tc;
                if(wake_time < minimal_wake_time) minimal_wake_time = wake_time;
              } 
              else 
              {
                active_tasks += state; 
              }
#else
             threadlist[i].thread(&ept[i]);
#endif
#ifdef PROFILER              
              get_timestamp(&thread_yeild_timestamp);
              dif = timestamp_dif_ticks(&thread_entry_timestamp, &thread_yeild_timestamp);
              if(dif > thread_max_exec_time_raw[i]) thread_max_exec_time_raw[i] = dif;
#endif
            }
#ifdef PROFILER
        get_timestamp(&loop_end_timestamp);
        dif = timestamp_dif_ticks(&loop_begin_timestamp, &loop_end_timestamp);
        if(dif > loop_max_exec_time_raw) loop_max_exec_time_raw = dif;
#endif

        }
#ifdef LOW_POWER_MODE
        if(!active_tasks) //if all threads returned PT_WAITING
        {
          if(minimal_wake_time != UINT32_MAX) //If there are at least some sleeping threads
          {
            ept_timer_set(&sleep_timer, minimal_wake_time);
          }
          while(!ept_timer_expired(&sleep_timer))
          {
            tc_flag = 0;
            __WFI();
            if(!tc_flag) break; // This means that some other then SysTick ISR happened
          }
        } 
#endif
  }
}

#include <string.h>
#ifndef __SNPRINTF 
#include <stdio.h>
#define __SNPRINTF snprintf
#endif
static const char* const state_str[] = {"STOP", "RUN"};

uint8_t thread_record_snprintf(char* buf, uint16_t buflen, const thread_record* record)
{
uint32_t index = record - threadlist;
  return __SNPRINTF(buf, buflen, "%i:%-8s:%-04s; Debug level: %i\r\n", index, 
                                                                       record->name,
                                                                       state_str[ept[index].state],
                                                                       dbg_level[index]);
}

const thread_record* thread_by_name(char * name)
{
  for(uint8_t i = 0; i < THREADNUM; i++)
  {
    if(strcmp(threadlist[i].name, name)==0) return &threadlist[i];
  }
  return NULL;
}

const thread_record* thread_by_index(uint8_t index)
{
  if(index < ARRAY_SIZE(threadlist)) return &threadlist[index];
  return NULL;
}

//return thread index if found. 
//returnx INVALID_THREAD is not found
uint8_t thread_index_by_name(char * name)
{
  for(uint8_t i = 0; i < THREADNUM; i++)
  {
    if(strcmp(threadlist[i].name, name)==0) return i;
  }
  return EPT_INVALID_THREAD;
}

void thread_cmd(const thread_record* record, ept_state_t state)
{
  if(record == NULL) return;
  uint32_t index = record - threadlist;
  ept[index].state = state;
  PT_INIT(&ept[index]);
}

void thread_set_debug(const thread_record* record, uint8_t lvl)
{
  uint32_t index = record - threadlist;
  dbg_level[index] = lvl;
}

#ifdef PROFILER
void reset_profiler()
{
  loop_max_exec_time_raw = 0;
  for(uint8_t i = 0; i < THREADNUM; i++)
  {
    thread_max_exec_time_raw[i] = 0;
  }
}

//returns the longest time thread had control in microseconds
uint32_t thread_get_exec_time_us(uint8_t index)
{
  if(index < THREADNUM) 
  {
    //return thread_max_exec_time_raw[index] * EPT_TICK_FREQ_HZ / SysTick_ARR;
    return thread_max_exec_time_raw[index] / 48;
  }return INT32_MAX;
}

//returns the longest time that all the loop took in microseconds
uint32_t loop_get_exec_time_us()
{
  //return loop_max_exec_time_raw * EPT_TICK_FREQ_HZ / SysTick_ARR;
  return loop_max_exec_time_raw / 48;
}

#endif