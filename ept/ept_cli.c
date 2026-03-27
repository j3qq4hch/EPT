#include <string.h>
#include "resources.h"
#include "ept_cli.h"
#include "serial_pt.h"
#include "named_arrays.h"
#include "cli_cmd_map.h"

#define CLI_NAME "EPT CLI V0.3"

#ifndef __SNPRINTF 
#include <stdio.h>
#define __SNPRINTF snprintf
#else 
#include "eprintf_stb.h"
#endif

static char cli_buf[CLI_BUFLEN + 1] = {0}; // +1 is to have at least one zero in the end always
char * cli_response_buf;
uint16_t cli_max_resp_len;

serpt_ctx_t serpt_ctx = 
{
  .serial = CLI_SERIAL_PTR 
};

const CLI_cmd_t * CLI_cmd_getbyname(char * name);

static uint8_t split_string(char *s, char const * delimeter, char *args[], uint8_t max_arg_no)
{
  uint8_t n = 0;
  char *saveptr;
  args[n++] = strtok_r(s, delimeter, &saveptr);
  while (1)
  {
    args[n] = strtok_r(NULL, delimeter, &saveptr);
    if((args[n] == NULL)||(n == max_arg_no))
    {
      break;
    }
    n++;
  }
  return n;
}

#define X(a,b) b,
static const char *retval_str[] = {
  CLI_RETVAL_LIST
};
#undef X

CLI_retval_t cli_process(char* str, char* response, uint16_t len)
{
  if(response)
  {
    cli_response_buf = response;
    cli_max_resp_len = len;
  }
  else 
  {
    cli_response_buf = cli_buf;
    cli_max_resp_len = ARRAY_SIZE(cli_buf);
  }
  char  * args[CLI_MAX_ARGNUM] = {0};
  uint8_t argno = split_string(str, " ", args, ARRAY_SIZE(args));
  CLI_cmd_t const * cmd = GET_BY_NAME_IN_STRUCT_ARRAY(args[0], cmd_map);
  CLI_retval_t retval = (cmd == NULL) ? CLI_UNKNOWN_CMD: cmd->handler(&args[1], argno - 1);
  if(strlen(response) == 0)
  {
    __SNPRINTF(response, len, "\r\n%s", retval_str[retval]);
  }
  return retval;
}

static struct pt child_pt;
EPT_THREAD(cli(struct ept *ept))
{
  PT_BEGIN(ept);
  SERIALPT_PRINTF(ept, &child_pt, &serpt_ctx, cli_buf, CLI_BUFLEN, "\r\n"CLI_NAME"\r\n>");
  while(1)
  {
    serial_flush_rx_buf(serpt_ctx.serial);
    memset(cli_buf, 0x00, sizeof(cli_buf));
    SERIALPT_READLINE(ept, &child_pt, &serpt_ctx, cli_buf, CLI_BUFLEN);
    if(!strlen(cli_buf)){continue;} // Excluding empty strings
    cli_process(cli_buf, cli_buf + serpt_ctx.cnt, CLI_BUFLEN - serpt_ctx.cnt);
    SERIALPT_PRINT(ept, &child_pt, &serpt_ctx, cli_response_buf);
    SERIALPT_PRINT(ept, &child_pt, &serpt_ctx, "\r\n>");
  }
  PT_END(ept);   
}


CLI_retval_t cli_h_handler (char * args[], uint8_t argno)
{
  if(argno == 0)
  {
    uint16_t n = 0;
    n += __SNPRINTF(cli_response_buf, cli_max_resp_len, "Available commands:\r\n");
    for(uint8_t i = 0; i < ARRAY_SIZE(cmd_map); i++)
    {
      n += __SNPRINTF(cli_response_buf + n, DEC_CLAMP(cli_max_resp_len, n, 0), "%s %s\r\n", cmd_map[i].name, cmd_map[i].help);
    }
  }
  return CLI_OK;
}