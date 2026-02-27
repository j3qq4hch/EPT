#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ept_cli.h"
#include "resources.h"
#include "name.h"
#include "thermals.h"
#include "settings.h"
#include "ept_scheduler.h"

#ifdef PROFILER
#define IF_PROFILER(arg) arg
#else 
#define IF_PROFILER(arg) 
#endif

#define CMD_LIST\
  X(h, "")\
  X(id, "")\
  X(set, "")\
  X(io, "")\
  X(pwm, "")\
  X(ept, "")\
  X(dbg, "")\
  X(gcr, "")\
  X(t,  "")\
  X(cal, "")\
  IF_PROFILER(X(prf, ""))

#define X(a, d) CLI_retval_t cli_##a##_handler(char * args[], uint8_t argno);
CMD_LIST
#undef X

#define X(a, d) {#a, cli_##a##_handler, d},
static const CLI_cmd_t cmd_map[] = 
{
  CMD_LIST
};
#undef X

const CLI_cmd_t * CLI_cmd_getbyname(char * name)
{
  for(uint8_t i = 0; i < ARRAY_SIZE(cmd_map); i++)
  {
    if(strcmp(cmd_map[i].name, name) == 0) { return &cmd_map[i]; }
  }
  return NULL;
}

////// Further are different command handlers ///////

CLI_retval_t cli_id_handler (char * args[], uint8_t argno)
{
  snprintf(cli_response_buf, cli_max_resp_len, "Hardware: %s\r\nSoftware: %s\r\n", HW_NAME, SW_NAME);
  return CLI_OK;
}

CLI_retval_t cli_h_handler (char * args[], uint8_t argno)
{
  if(argno == 0)
  {
    uint16_t n = 0;
    n += snprintf(cli_response_buf, cli_max_resp_len, "Available commands:\r\n");
    for(uint8_t i = 0; i < ARRAY_SIZE(cmd_map); i++)
    {
      n += snprintf(cli_response_buf + n, DEC_CLAMP(cli_max_resp_len, n, 0), "%s %s\r\n", cmd_map[i].name, cmd_map[i].help);
    }
  }
  return CLI_OK;
}

CLI_retval_t cli_adc_handler (char * args[], uint8_t argno)
{
  uint16_t adc_val = adc_read_single_channel(LL_ADC_CHANNEL_2);
  uint32_t voltage_mv = __LL_ADC_CALC_DATA_TO_VOLTAGE(3300, adc_val, LL_ADC_RESOLUTION_12B);
  snprintf(cli_response_buf, cli_max_resp_len, "ADC_TICKS = %i; V = %i mV\r\n",adc_val, voltage_mv);
  return CLI_OK;
}

static const pin_t * const pin_array[] = 
{
  PIN_LIST(GENERATE_PIN_PTR_ARRAY)
};
static const pin_t * pin_getbyname(const char* name)
{
    for(size_t i = 0; i < ARRAY_SIZE(pin_array); i++)
    {
        if(strcmp(name, pin_array[i]->name) == 0)
        {
            return pin_array[i];
        }
    }
    return NULL;
}

CLI_retval_t cli_io_handler (char * args[], uint8_t argno)
{
  if(argno == 0)
  {
    uint16_t n = 0;
    for(uint8_t i = 0; i < ARRAY_SIZE(pin_array); i++)
    { 
       n += pin_snprint(cli_response_buf + n, DEC_CLAMP(cli_max_resp_len, n, 0), pin_array[i]);
    }
    return CLI_OK;
  }
  if(argno == 2)
  {
     const pin_t * pin = pin_getbyname(args[0]);
     if(pin == NULL){ return CLI_ERROR; }
     char *endptr;
     uint16_t state = strtol(args[1], &endptr, 10);
     if(*endptr != 0)  { return CLI_ERROR; }
     if(state) pin_set(pin);
     else pin_reset(pin);
     return CLI_OK;
  }
  return CLI_ERROR;
}

//This is a constant array of pointers to constants
static const pwm_t * const pwm_array[] = 
{
  PWM_LIST(GENERATE_PWM_PTR_ARRAY)
};

static const pwm_t * pwm_getbyname(const char* name)
{
    for(size_t i = 0; i < ARRAY_SIZE(pwm_array); i++)
    {
        if(strcmp(name, pwm_array[i]->name) == 0)
        {
            return pwm_array[i];
        }
    }
    return NULL;
}

CLI_retval_t cli_pwm_handler (char * args[], uint8_t argno)
{
  if(argno == 0)
  {
    uint16_t n = 0;
    for(uint8_t i = 0; i < ARRAY_SIZE(pwm_array); i++)
    { 
       n += pwm_snprint(cli_response_buf + n, DEC_CLAMP(cli_max_resp_len, n, 0), pwm_array[i]);
    }
    return CLI_OK;
  } 
  if(argno == 2) 
  {
     const pwm_t * pwm = pwm_getbyname(args[0]);
     if(pwm == NULL)  {return CLI_ERROR; }
     char *endptr;
     uint16_t num = strtol(args[1], &endptr, 10);
     if(*endptr != 0)  { return CLI_ERROR; }
     pwm_set(pwm, num);
     return CLI_OK;
  }
  return CLI_ERROR;
}

CLI_retval_t cli_ept_handler (char * args[], uint8_t argno)
{
  if(argno == 0)
  {
    uint16_t n = 0;
    uint16_t i = 0;
    while(1)
    {
      const thread_record* thread = thread_by_index(i++);
      if(thread == NULL) break;
      n += thread_record_snprintf(cli_response_buf + n, DEC_CLAMP(cli_max_resp_len, n, 0), thread);
    }
    return CLI_OK;
  }
  if(argno == 2)
  {
    const thread_record* thread = thread_by_name(args[0]);
    if(thread == NULL) return CLI_ERROR;
    if(strcmp(args[1], "start") == 0)
    {
      thread_cmd(thread, RUN);
      return CLI_OK;
    }
    if(strcmp(args[1], "stop") == 0)
    {
      thread_cmd(thread, STOP);
      return CLI_OK;
    }
  }
  return CLI_ERROR;
}

CLI_retval_t cli_dbg_handler (char * args[], uint8_t argno)
{
  if(argno == 2)
  {
    const thread_record* thread = thread_by_name(args[0]);
    if(thread == NULL) return CLI_ERROR;
    char *endptr;
    uint8_t lvl = strtol(args[1], &endptr, 10);
    if(*endptr != 0)  { return CLI_ERROR; }
    thread_set_debug(thread, lvl);  
    return CLI_OK;
  }
  return CLI_ERROR;
}

CLI_retval_t cli_t_handler(char * args[], uint8_t argno)
{
  snprintf(cli_response_buf, cli_max_resp_len, "Temp = %i\r\n", sys_temperature);
  return CLI_OK;
}
//CLI cannot restart itself. It is also stupid to stop it as commands will not be processd
#ifdef PROFILER
CLI_retval_t cli_prf_handler(char * args[], uint8_t argno)
{ 
  uint16_t i = 0;
  uint16_t n = 0;
  uint32_t pv;
  n += snprintf(cli_response_buf + n, DEC_CLAMP(cli_max_resp_len, n, 0), "Worst loop time: %i uS\r\n", loop_get_exec_time_us());
  while(1)
    {
      const thread_record* thread = thread_by_index(i);
      if(thread == NULL) break;
      pv = thread_get_exec_time_us(i++);
      n += snprintf(cli_response_buf + n, DEC_CLAMP(cli_max_resp_len, n, 0), "%-6s: %i uS\r\n", thread->name, pv);
    }
  reset_profiler();
  return CLI_OK;
}
#endif