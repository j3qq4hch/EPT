#include "serial.h"
#include <stddef.h>
#include <string.h>

////Hardware dependant parts/////
static inline void start_dma_transaction(const char * data, uint16_t len)
{
    LL_DMA_DisableChannel  (DMA_IN_USE, DMA_CHANNEL);
    LL_DMA_SetMemoryAddress(DMA_IN_USE, DMA_CHANNEL, (uint32_t)data);
    LL_DMA_SetDataLength   (DMA_IN_USE, DMA_CHANNEL, len);  
    LL_DMA_EnableChannel   (DMA_IN_USE, DMA_CHANNEL); 
}
////////////////////////////////

//We don't need RING_Init as I do it manually
//static inline uint8_t RING_Init(RING_buffer_t *ring, uint8_t *buf, uint16_t size)
//{
//    ring->size = size;
//    ring->buffer = buf;
//    RING_Clear(ring);
//    return 0;
//}

static inline void RING_Clear(RING_buffer_t* buf)
{
    buf->idxIn = 0;
    buf->idxOut = 0;
}

static inline uint16_t RING_GetCount(const RING_buffer_t *buf)
{
    uint16_t retval = 0;
    if (buf->idxIn < buf->idxOut) retval = buf->size + buf->idxIn - buf->idxOut;
    else retval = buf->idxIn - buf->idxOut;
    return retval;
}

static inline uint8_t RING_Pop(RING_buffer_t *buf)
{
    uint8_t retval = buf->buffer[buf->idxOut++];
    if (buf->idxOut >= buf->size) buf->idxOut = 0;
    return retval;
}

void serial_open(serial_t *serial)
{
  RING_Clear(&serial->rx_ring_buf);
  //Hardware dependant section below//
  LL_DMA_SetPeriphAddress(DMA_IN_USE, DMA_CHANNEL, (uint32_t)&serial->hw_desc->TDR);
  LL_USART_EnableDMAReq_TX(serial->hw_desc);
  LL_USART_Enable(serial->hw_desc); // For some reason this should happen BEFORE enabling the interrupts
  LL_USART_EnableIT_RXNE(serial->hw_desc);
  LL_USART_EnableIT_TC(serial->hw_desc);
}

uint8_t serial_write(serial_t *serial,  const char * data, serint_block_flag_t block_flag)
{
  if(serial_is_tx_busy (serial)) return 1; //Transmit cannot be started as it is already in progress
  size_t len = strlen(data);
  if(len)
  {
    LL_USART_DisableDirectionRx(serial->hw_desc); //This is needed due to half-duplex uart
    start_dma_transaction(data, len);
    if(block_flag == BLOCKING)
    {
      while(serial_is_tx_busy(serial)){}
    }
  }
  return 0;
}

uint16_t serial_read(serial_t *serial,  uint8_t *data, uint16_t len)
{
  if(RING_GetCount(&serial->rx_ring_buf) < len)
  {
    len = RING_GetCount(&serial->rx_ring_buf);
  }
  uint16_t retval = len;
  while(len--)
  {
    *data++ = RING_Pop(&serial->rx_ring_buf);
  }
  return retval;
}

uint16_t serial_available(serial_t *serial)
{
  return RING_GetCount(&serial->rx_ring_buf); 
}

void serial_flush_rx_buf(serial_t *serial) 
{
  RING_Clear(&serial->rx_ring_buf);
}

void serial_write_hardfaulf(serial_t *serial, const char * data)
{
  LL_USART_DisableDirectionRx(serial->hw_desc); //This is needed due to half-duplex uart
  while(*data)
  {
    LL_USART_TransmitData8(serial->hw_desc, *data++);
    while(!LL_USART_IsActiveFlag_TXE(serial->hw_desc)){}
  }
    while(!LL_USART_IsActiveFlag_TC(serial->hw_desc)){}
  LL_USART_EnableDirectionRx(serial->hw_desc); 
}