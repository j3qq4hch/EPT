#ifndef DRV_BASIC_SMT32F0XX_
#define DRV_BASIC_SMT32F0XX_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f0xx_ll_usart.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_gpio.h"

// ---- GPIO -------------------------------------------------------------------

typedef struct {
    const char   *name;
    GPIO_TypeDef *port;
    uint32_t      pin;   // LL_GPIO_PIN_x
} pin_t;

static inline void     pin_set   (const pin_t *p) { LL_GPIO_SetOutputPin  (p->port, p->pin); }
static inline void     pin_reset (const pin_t *p) { LL_GPIO_ResetOutputPin(p->port, p->pin); }
static inline void     pin_toggle(const pin_t *p) { LL_GPIO_TogglePin     (p->port, p->pin); }
static inline uint32_t pin_read  (const pin_t *p) { return LL_GPIO_IsInputPinSet(p->port, p->pin); }

// ---- UART -------------------------------------------------------------------
// TX: zero-copy DMA (non-blocking). Keep the source buffer alive until !uart_tx_busy().
// RX: interrupt-driven ring buffer. Call uart_isr() from the USART IRQ handler.
// Half-duplex (single-wire): set .half_duplex = true in the uart_t initializer.

typedef struct {
    const char    *name;
    USART_TypeDef *usart;
    DMA_TypeDef   *dma;
    uint32_t       dma_tx_ch;       // LL_DMA_CHANNEL_x
    bool           half_duplex;
    uint8_t       *rx_buf;
    uint16_t       rx_buf_len;
    volatile uint16_t rx_head;      // written by ISR
    uint16_t          rx_tail;      // read by application
} uart_t;

void     uart_open     (uart_t *u);
bool     uart_write    (uart_t *u, const uint8_t *data, uint16_t len);
void     uart_isr      (uart_t *u);
uint16_t uart_available(uart_t *u);
uint8_t  uart_getc     (uart_t *u);
uint16_t uart_read     (uart_t *u, uint8_t *dst, uint16_t len);
bool     uart_tx_busy  (uart_t *u);

static inline void uart_flush_rx(uart_t *u) { u->rx_tail = u->rx_head; }

uint16_t pin_snprint (char *buf, uint16_t buflen, const pin_t  *p);
uint16_t uart_snprint(char *buf, uint16_t buflen, const uart_t *u);

#endif // DRV_BASIC_H_

// ---- Implementation ---------------------------------------------------------
#ifdef DRV_BASIC_IMPLEMENTATION
#ifndef DRV_BASIC_IMPLEMENTATION_DONE_
#define DRV_BASIC_IMPLEMENTATION_DONE_

// Call after peripheral clock and pin init. Enables interrupts and DMA request.
void uart_open(uart_t *u)
{
    u->rx_head = 0;
    u->rx_tail = 0;
    LL_DMA_SetPeriphAddress(u->dma, u->dma_tx_ch, (uint32_t)&u->usart->TDR);
    LL_USART_EnableDMAReq_TX(u->usart);
    LL_USART_EnableIT_RXNE(u->usart);
    LL_USART_EnableIT_TC(u->usart);
    LL_USART_Enable(u->usart);
}

bool uart_tx_busy(uart_t *u)
{
    return (bool)LL_DMA_IsEnabledChannel(u->dma, u->dma_tx_ch);
}

// Returns false if DMA is busy � caller retries or drops.
bool uart_write(uart_t *u, const uint8_t *data, uint16_t len)
{
    if (uart_tx_busy(u)) return false;
    if (u->half_duplex) {
        LL_USART_DisableDirectionRx(u->usart);
        LL_USART_EnableDirectionTx(u->usart);
    }
    LL_DMA_SetMemoryAddress(u->dma, u->dma_tx_ch, (uint32_t)data);
    LL_DMA_SetDataLength   (u->dma, u->dma_tx_ch, len);
    LL_DMA_EnableChannel   (u->dma, u->dma_tx_ch);
    return true;
}

void uart_isr(uart_t *u)
{
    if (LL_USART_IsActiveFlag_RXNE(u->usart)) {
        uint16_t next = u->rx_head + 1;
        if (next >= u->rx_buf_len) next = 0;
        uint8_t byte = LL_USART_ReceiveData8(u->usart); // clears RXNE
        if (next != u->rx_tail) {                        // drop on overflow
            u->rx_buf[u->rx_head] = byte;
            u->rx_head = next;
        }
    }
    if (LL_USART_IsActiveFlag_ORE(u->usart))
        LL_USART_ClearFlag_ORE(u->usart);
    if (LL_USART_IsActiveFlag_TC(u->usart)) {
        LL_DMA_DisableChannel(u->dma, u->dma_tx_ch);
        LL_USART_ClearFlag_TC(u->usart);
        if (u->half_duplex) {
            LL_USART_DisableDirectionTx(u->usart);
            LL_USART_EnableDirectionRx(u->usart);
        }
    }
}

uint16_t uart_available(uart_t *u)
{
    uint16_t head = u->rx_head; // snapshot volatile
    if (head >= u->rx_tail)
        return head - u->rx_tail;
    return u->rx_buf_len - u->rx_tail + head;
}

uint8_t uart_getc(uart_t *u)
{
    uint8_t c = u->rx_buf[u->rx_tail];
    if (++u->rx_tail >= u->rx_buf_len) u->rx_tail = 0;
    return c;
}

uint16_t uart_read(uart_t *u, uint8_t *dst, uint16_t len)
{
    uint16_t n = 0;
    while (n < len && uart_available(u))
        dst[n++] = uart_getc(u);
    return n;
}

#include "snprintf_compat.h"

uint16_t pin_snprint(char *buf, uint16_t buflen, const pin_t *p)
{
    uint32_t mode = LL_GPIO_GetPinMode(p->port, p->pin);
    if (mode == LL_GPIO_MODE_OUTPUT)
        return (uint16_t)__SNPRINTF(buf, buflen, "%s out:%u", p->name,
                                    (unsigned)LL_GPIO_IsOutputPinSet(p->port, p->pin));
    if (mode == LL_GPIO_MODE_INPUT)
        return (uint16_t)__SNPRINTF(buf, buflen, "%s in:%u", p->name,
                                    (unsigned)LL_GPIO_IsInputPinSet(p->port, p->pin));
    if (mode == LL_GPIO_MODE_ALTERNATE)
        return (uint16_t)__SNPRINTF(buf, buflen, "%s alt", p->name);
    return (uint16_t)__SNPRINTF(buf, buflen, "%s analog", p->name);
}

uint16_t uart_snprint(char *buf, uint16_t buflen, const uart_t *u)
{
    return (uint16_t)__SNPRINTF(buf, buflen, "%s rx:%u tx:%s",
                                u->name,
                                (unsigned)uart_available((uart_t *)u),
                                uart_tx_busy((uart_t *)u) ? "busy" : "idle");
}

#endif // DRV_BASIC_IMPLEMENTATION_DONE_
#endif