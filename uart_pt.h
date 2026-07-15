#ifndef UART_PT_H_
#define UART_PT_H_

#include <string.h>
#include "pt.h"
#include "drv_basic.h"

#include "snprintf_compat.h"

typedef struct {
    uart_t  *uart;
    char    *buf;
    uint16_t buflen;
    uint16_t cnt;
    char     stop_char;
} uart_pt_ctx_t;

PT_THREAD(uart_pt_write   (struct pt *pt, uart_pt_ctx_t *ctx));
PT_THREAD(uart_pt_readline(struct pt *pt, uart_pt_ctx_t *ctx));

#define UART_PT_READLINE(PT, CHILD, CTX, BUF, BUFLEN)              \
    do {                                                             \
        (CTX)->stop_char = '\r';                                     \
        (CTX)->buf       = (BUF);                                    \
        (CTX)->buflen    = (BUFLEN);                                 \
        EPT_SPAWN((PT), (CHILD), uart_pt_readline((CHILD), (CTX))); \
    } while (0)

#define UART_PT_PRINTF(PT, CHILD, CTX, BUF, BUFLEN, fmt, ...)      \
    do {                                                             \
        (CTX)->buf    = (BUF);                                       \
        (CTX)->buflen = (uint16_t)__SNPRINTF((BUF), (BUFLEN), fmt, ##__VA_ARGS__); \
        EPT_SPAWN((PT), (CHILD), uart_pt_write((CHILD), (CTX)));    \
    } while (0)

#define UART_PT_PRINT(PT, CHILD, CTX, BUF)                         \
    do {                                                             \
        (CTX)->buf    = (BUF);                                       \
        (CTX)->buflen = (uint16_t)strlen(BUF);                      \
        EPT_SPAWN((PT), (CHILD), uart_pt_write((CHILD), (CTX)));    \
    } while (0)

#endif // UART_PT_H_

// ---- Implementation ---------------------------------------------------------
#ifdef UART_PT_IMPLEMENTATION
#ifndef UART_PT_IMPLEMENTATION_DONE_
#define UART_PT_IMPLEMENTATION_DONE_

PT_THREAD(uart_pt_write(struct pt *pt, uart_pt_ctx_t *ctx))
{
    PT_BEGIN(pt);
    PT_WAIT_WHILE(pt, uart_tx_busy(ctx->uart));
    uart_write(ctx->uart, (const uint8_t *)ctx->buf, ctx->buflen);
    PT_WAIT_WHILE(pt, uart_tx_busy(ctx->uart));
    PT_END(pt);
}

PT_THREAD(uart_pt_readline(struct pt *pt, uart_pt_ctx_t *ctx))
{
    PT_BEGIN(pt);
    ctx->cnt = 0;
    while (ctx->cnt < ctx->buflen - 1) {
        PT_WAIT_UNTIL(pt, uart_available(ctx->uart));
        ctx->buf[ctx->cnt] = (char)uart_getc(ctx->uart);
        if ((uint8_t)ctx->buf[ctx->cnt] == 127) {
            if (ctx->cnt) ctx->cnt--;
            continue;
        }
        if (ctx->buf[ctx->cnt] == ctx->stop_char) {
            ctx->buf[ctx->cnt] = 0;
            break;
        }
        ctx->cnt++;
    }
    PT_END(pt);
}

#endif // UART_PT_IMPLEMENTATION_DONE_
#endif // UART_PT_IMPLEMENTATION
