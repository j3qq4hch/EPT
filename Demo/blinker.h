#ifndef __BLINKER
#define __BLINKER

#include "pin.h"
#include "ept_scheduler.h"

typedef struct
{
  const pin_t * pin;
  uint32_t period;
}blinker_ctx;

EPT_THREAD(blinker(struct ept *ept));

#endif