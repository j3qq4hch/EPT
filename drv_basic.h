#ifdef STM32C011xx
#include "drv_basic_stm32c0xx.h"
#elif STM32F0xx
#include "drv_basic_stm32f0xx.h"
#else
#error "MCU family not specified"
#endif