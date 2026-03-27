#include "main.h"
#define RESOURCES_IMPLEMENTATION
#include "resources.h"
#include "blinker.h"
#include "ept_scheduler.h"


#define EPT_IMPLEMENTATION
#include "ept.h"
blinker_ctx blctx1 = 
{
  .pin = &C9_pin, 
  .period = 250
};

blinker_ctx blctx2 = 
{
  .pin = &C8_pin, 
  .period = 400
};

int main(void)
{
  hw_init();
  ept_scheduler();
}