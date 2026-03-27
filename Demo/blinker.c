#include "blinker.h"

PT_THREAD(blinker(struct ept * ept))
{
    blinker_ctx * ctx = thread_ctx; 
    PT_BEGIN(ept);
    while(1)
    {
        EPT_SLEEP(ept, ctx->period);
        pin_toggle(ctx->pin);
    }
    PT_END(ept);
}