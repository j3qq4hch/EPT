#ifndef __EPT_TEST
#define __EPT_TEST

#include <stdint.h>
#include "ept.h"

typedef struct
{
  uint16_t counter;
  uint16_t period_ms;
}pw_ctx_t;

PT_THREAD(periodic_writer    (struct ept* ept));
PT_THREAD(ept_event_generator(struct ept* ept));

#endif