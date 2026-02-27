#include <string.h>
#include <stdio.h>
#include "resources.h"
#include "ept_cli.h"
#include "serial_pt.h"

#define CLI_NAME "EPT CLI V0.3"

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
  cli_response_buf = response;
  *cli_response_buf = 0;
  cli_max_resp_len = len;
  char  * args[CLI_MAX_ARGNUM] = {0};
  uint8_t argno = split_string(str, " ", args, ARRAY_SIZE(args));
  CLI_cmd_t const * cmd = CLI_cmd_getbyname(args[0]);
  CLI_retval_t retval = (cmd == NULL) ? CLI_UNKNOWN_CMD: cmd->handler(&args[1], argno - 1);
  if(strlen(response) == 0)
  {
    snprintf(response, len, "\r\n%s", retval_str[retval]);
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
    SERIALPT_READLINE(ept, &child_pt, &serpt_ctx, cli_buf, CLI_BUFLEN);
    if(!strlen(cli_buf)){continue;} // Excluding empty strings
    cli_process(cli_buf, cli_buf + serpt_ctx.cnt, CLI_BUFLEN - serpt_ctx.cnt);
    SERIALPT_PRINT(ept, &child_pt, &serpt_ctx, cli_response_buf);
    SERIALPT_PRINT(ept, &child_pt, &serpt_ctx, "\r\n>");
  }
  PT_END(ept);   
}