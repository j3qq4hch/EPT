#ifndef __SERIAL
#define __SERIAL

#include <stdint.h>
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_usart.h"

//This may be adjusted for different device
#define DMA_IN_USE    DMA1
#define DMA_CHANNEL   LL_DMA_CHANNEL_4
//DMA data should be added to hardware handler if more than one USART interface is used
typedef struct
{
    uint8_t *buffer;
    uint16_t idxIn;
    uint16_t idxOut;
    uint16_t size;           
} RING_buffer_t;

typedef struct
{
  USART_TypeDef *hw_desc;
  RING_buffer_t rx_ring_buf;
}serial_t;

typedef enum {NON_BLOCKING = 1, BLOCKING = 0} serint_block_flag_t;

static inline uint8_t serial_is_tx_busy (serial_t *serial)  { return LL_DMA_IsEnabledChannel(DMA_IN_USE, DMA_CHANNEL); }

static inline void RING_Put(RING_buffer_t* buf, uint8_t symbol)
{
    buf->buffer[buf->idxIn++] = symbol;
    if (buf->idxIn >= buf->size) buf->idxIn = 0;
}

static inline void serial_isr(serial_t *serial)
{
  if(LL_USART_IsActiveFlag_RXNE(serial->hw_desc))
  {
    RING_Put(&serial->rx_ring_buf, LL_USART_ReceiveData8(serial->hw_desc));
  }

  if(LL_USART_IsActiveFlag_ORE(serial->hw_desc))
  {
      LL_USART_ClearFlag_ORE(serial->hw_desc);
  }
  if(LL_USART_IsActiveFlag_TC(serial->hw_desc))
  {
    LL_DMA_DisableChannel(DMA_IN_USE, DMA_CHANNEL);
    LL_USART_ClearFlag_TC(serial->hw_desc);
    LL_USART_EnableDirectionRx(serial->hw_desc); 
  }
}

void serial_open           (serial_t *serial);
uint16_t serial_read       (serial_t *serial,  uint8_t *data, uint16_t len);
uint16_t serial_available  (serial_t *serial);
uint8_t  serial_write_bin  (serial_t *serial, const uint8_t * data, uint16_t len, serint_block_flag_t block_flag);
uint8_t  serial_write      (serial_t *serial, const char    * data, serint_block_flag_t block_flag);
void serial_flush_rx_buf   (serial_t *serial);
//void serial_isr            (serial_t *serial);

#endif