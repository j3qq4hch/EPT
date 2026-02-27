#ifndef __CLI_PT
#define __CLI_PT

#include <stdint.h>
#include "ept_cfg.h"
#include "ept.h"

#define INC_CLAMP(var, n, clampH) ((clampH) - (var) >= (n)) ? ((var) + (n)) : (clampH)
#define DEC_CLAMP(var, n, clampL) ((var) >= (clampL) + (n)) ? ((var) - (n)) : (clampL)

#define CLI_RETVAL_LIST\
  X(OK, "OK")\
  X(ERROR, "ERROR")\
  X(UNKNOWN_CMD, "Unknown command. Try h")
  
#define X(a,b) CLI_##a,
typedef enum{ CLI_RETVAL_LIST } CLI_retval_t;
#undef X

typedef struct
{
    const char * name;
    CLI_retval_t (*handler)(char * args[], uint8_t argno);
    const char * help;
} CLI_cmd_t;

extern char* cli_response_buf;
extern uint16_t cli_max_resp_len;

CLI_retval_t cli_process(char* str, char* response, uint16_t len);
#endif