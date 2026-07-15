#define UART_PT_IMPLEMENTATION
#include "uart_pt.h"

#define NAMED_LOOKUP_IMPLEMENTATION
#include "named_lookup.h"

#include "snprintf_compat.h"

#include <string.h>
#include "ept_cli.h"
#include "ept_cfg.h"
#include "cli_cmd_map.h"

// ---- Globals ----------------------------------------------------------------
char        *cli_response_buf;
uint16_t     cli_max_resp_len;
cli_gen_fn_t cli_gen;

// ---- Thread state -----------------------------------------------------------
static char          cli_buf[CLI_BUFLEN + 1];
static struct pt     child_pt;
static uart_pt_ctx_t ctx = { .uart = CLI_UART };

// ---- Retval strings ---------------------------------------------------------
#define X(a, b) b,
static const char *const retval_str[] = { CLI_RETVAL_LIST };
#undef X

// ---- Command parsing --------------------------------------------------------
static uint8_t split_string(char *s, const char *delim, char *args[], uint8_t max)
{
    uint8_t n = 0;
    char *saveptr;
    args[n++] = strtok_r(s, delim, &saveptr);
    while (1) {
        args[n] = strtok_r(NULL, delim, &saveptr);
        if (args[n] == NULL || n == max) break;
        n++;
    }
    return n;
}

CLI_retval_t cli_process(char *str, char *response, uint16_t len)
{
    cli_response_buf = response ? response : cli_buf;
    cli_max_resp_len = response ? len : (uint16_t)sizeof(cli_buf);
    *cli_response_buf = '\0';

    char    *args[CLI_MAX_ARGNUM] = {0};
    uint8_t  argno = split_string(str, " ", args, ARRAY_SIZE(args));

    if (args[0] == NULL) return CLI_OK;

    CLI_cmd_t const *cmd = FIND_BY_NAME(cmd_map, args[0]);
    CLI_retval_t retval  = (cmd == NULL) ? CLI_UNKNOWN_CMD
                                         : cmd->handler(&args[1], argno - 1);

    if (retval != CLI_LONG_RESPONSE && *cli_response_buf == '\0')
        __SNPRINTF(cli_response_buf, cli_max_resp_len, "\r\n%s", retval_str[retval]);

    return retval;
}

// ---- CLI thread -------------------------------------------------------------
EPT_THREAD(cli(struct ept *ept))
{
    // Statics required: PT locals don't survive yields.
    static CLI_retval_t s_retval;
    static uint16_t     s_gen_state;
    static uint16_t     s_gen_written;

    EPT_BEGIN(ept);

    UART_PT_PRINTF(ept, &child_pt, &ctx,
                   cli_buf, sizeof(cli_buf), "\r\n%s\r\n>", CLI_GREETINGS_STR);

    while (1) {
        EPT_WAIT_UNTIL(ept, !uart_tx_busy(ctx.uart));
        uart_flush_rx(ctx.uart);
        memset(cli_buf, 0, sizeof(cli_buf));

        UART_PT_READLINE(ept, &child_pt, &ctx, cli_buf, CLI_BUFLEN);
        if (!ctx.cnt) continue;

        cli_gen  = NULL;
        s_retval = cli_process(cli_buf,
                               cli_buf + ctx.cnt + 1,
                               CLI_BUFLEN - ctx.cnt - 1);

        if (s_retval == CLI_LONG_RESPONSE) {
            if (cli_gen == NULL) {
                UART_PT_PRINT(ept, &child_pt, &ctx,
                              (char *)"\r\nERR: CLI_LONG_RESPONSE without generator");
            } else {
                s_gen_state = 0;
                for (;;) {
                    s_gen_written = cli_gen(s_gen_state,
                                            cli_response_buf, cli_max_resp_len);
                    if (!s_gen_written) break;
                    UART_PT_PRINT(ept, &child_pt, &ctx, cli_response_buf);
                    s_gen_state++;
                }
            }
        } else {
            UART_PT_PRINT(ept, &child_pt, &ctx, cli_buf + ctx.cnt + 1);
        }

        UART_PT_PRINT(ept, &child_pt, &ctx, (char *)"\r\n>");
    }
    
    EPT_END(ept);
}

// ---- Built-in help command --------------------------------------------------
static uint16_t help_gen(uint16_t state, char *buf, uint16_t buflen)
{
    if (state >= ARRAY_SIZE(cmd_map)) return 0;
    return (uint16_t)__SNPRINTF(buf, buflen, "%-12s %s\r\n",
                                cmd_map[state].name, cmd_map[state].help);
}

CLI_retval_t h_cmd(char *args[], uint8_t argno)
{
    (void)args; (void)argno;
    cli_gen = help_gen;
    return CLI_LONG_RESPONSE;
}
