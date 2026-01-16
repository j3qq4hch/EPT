#include "ept.h"
#include "resources.h"
#include "ept_test.h"
#include <string.h>
#include <stdio.h>

static char tmpbuf[64];

PT_THREAD(periodic_writer(struct ept* ept))
{
  pw_ctx_t* ctx = (pw_ctx_t*)ept->ctx;
  PT_BEGIN(ept);
  ctx->counter = 0;
  while(1)
  {
    while(!ept->event)
    { //This is main body of the thread
    sprintf(tmpbuf, "Periodic writer %s counted to %i\r\n", ept->name, ctx->counter );
    serial_write(&serial1, tmpbuf, BLOCKING);
    ctx->counter++;
    EPT_SLEEP(ept, ctx->period_ms);
    serial_write(&serial1, "Tick\r\n", BLOCKING);
    }
    { //event flag handler must be here
      sprintf(tmpbuf, "%s: EVENT %i\r\n", ept->name, ept->event);
      serial_write(&serial1, tmpbuf, BLOCKING);
      ept->event = 0;
      return PT_YIELDED; //This way local continuation is not updated. 
                         //So on the next entry to the thread it will start 
                         //from the place it stopped when enent happened
      //PT_YIELD(ept);
    }
  }
  PT_END(ept);
}

PT_THREAD(ept_event_generator(struct ept* ept))
{ 
  struct ept *ept__ = (struct ept*)ept->ctx;
  PT_BEGIN(ept);
  while(1)
  {
    EPT_SLEEP(ept, 1000);
    ept__->event = 10;
  }
  PT_END(ept);
}