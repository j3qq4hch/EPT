
//This is user-provided file
// Threadlist array is a constant static arrray
// It must be filled manually by the user 

//#define LOW_POWER_MODE //2DO


EPT_THREAD(cli(struct ept *ept));
EPT_THREAD(tlogger(struct ept *ept));
EPT_THREAD(tkeeper(struct ept *ept));

//If there are important local variables in the thread, they too should be places in context stuct

#include "blinker.h"
blinker_ctx blred_ctx  = {.name = "blred",  .pwm = &pwm_red,  .period = 1000};
blinker_ctx blblue_ctx = {.name = "blblue", .pwm = &pwm_blue, .period = 330};

//You should probably always place CLI thread first and then never try to turn it off
//as the system will stop responding to commands then
const thread_record threadlist[] = 
{
  {.name = "cli",    .thread = cli,      .context = NULL,        .init_state = RUN},
  {.name = "blred",  .thread = blinker,  .context = &blred_ctx,  .init_state = RUN},
  {.name = "blblue", .thread = blinker,  .context = &blblue_ctx, .init_state = RUN},
  {.name = "tlog",   .thread = tlogger,  .context = NULL,        .init_state = STOP},
  {.name = "tkeep",  .thread = tkeeper,  .context = NULL,        .init_state = STOP},
    //{.name = "anim",  .thread = anim_ept, .init_state = STOP},
};


