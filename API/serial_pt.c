#include "serial_pt.h"

//These things enable async serial reading and writing

PT_THREAD(serialpt_write(struct pt* pt, serpt_ctx_t * ser_str))
{
  PT_BEGIN(pt);
  PT_WAIT_WHILE(pt, serial_is_tx_busy(ser_str->serial));
  serial_write(ser_str->serial, ser_str->buf, NON_BLOCKING);
  PT_WAIT_WHILE(pt, serial_is_tx_busy(ser_str->serial));
  PT_END(pt);
}

// This reads a string from serial interface until stop_char
// It also processes non-printable BACKSPACE character in an obvious way
PT_THREAD(serialpt_read_until(struct pt * pt, serpt_ctx_t * ser_str))
{
  PT_BEGIN(pt);
  ser_str->cnt = 0;
  while (ser_str->cnt < (ser_str->buflen - 1))
  { 
    PT_WAIT_UNTIL(pt, serial_available (ser_str->serial));
    serial_read(ser_str->serial, (uint8_t*)&ser_str->buf[ser_str->cnt], 1);
    if(ser_str->buf[ser_str->cnt] == 127) //delete processing
    { 
      if(ser_str->cnt) ser_str->cnt--;
      continue;
    }
    if (ser_str->buf[ser_str->cnt] == ser_str->stop_char) 
    {
      ser_str->buf[ser_str->cnt] = 0;
      break;
    }
  ser_str->cnt++;
  }
  PT_END(pt);
}