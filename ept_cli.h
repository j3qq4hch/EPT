#ifndef EPT_CLI_H_
#define EPT_CLI_H_

#include <stdint.h>
#include "ept.h"

// Provide in resources.h:
//   CLI_UART        — uart_t* for the CLI UART (e.g. &debug_uart)
//   CLI_BUFLEN      — combined input+response buffer size in bytes (e.g. 192)
//   CLI_MAX_ARGNUM  — max number of tokens including the command name (e.g. 8)
//   CLI_NAME        — banner string (e.g. "My Device v1.0")

// Utilities for building responses inside handlers.
#define CLI_INC_CLAMP(var, n, hi) (((hi) - (var) >= (n)) ? ((var) + (n)) : (hi))
#define CLI_DEC_CLAMP(var, n, lo) (((var) >= (lo) + (n)) ? ((var) - (n)) : (lo))

#define CLI_RETVAL_LIST                              \
    X(OK,            "OK")                           \
    X(ERROR,         "ERROR")                        \
    X(UNKNOWN_CMD,   "Unknown command. Try h")       \
    X(LONG_RESPONSE, "")

#define X(a, b) CLI_##a,
typedef enum { CLI_RETVAL_LIST } CLI_retval_t;
#undef X

// Generator for long responses.
// CLI calls gen(0), gen(1), ... writing into buf[0..buflen].
// Returns bytes written. Returns 0 when done.
typedef uint16_t (*cli_gen_fn_t)(uint16_t state, char *buf, uint16_t buflen);

// name must be the first field — required by FIND_BY_NAME.
typedef struct {
    const char    *name;
    CLI_retval_t (*handler)(char *args[], uint8_t argno);
    const char    *help;
} CLI_cmd_t;

extern char        *cli_response_buf;  // write response here inside a handler
extern uint16_t     cli_max_resp_len;  // max bytes available in cli_response_buf
extern cli_gen_fn_t cli_gen;           // set by handler when returning CLI_LONG_RESPONSE

CLI_retval_t cli_process  (char *str, char *response, uint16_t len);
CLI_retval_t h_cmd(char *args[], uint8_t argno);
EPT_THREAD  (cli(struct ept *ept));

#endif // EPT_CLI_H_
