#ifndef __PIN
#define __PIN

#include "stm32f0xx_ll_gpio.h"
#include <stdint.h>

// Macros for automated pin variables generation using X-macro 
// X-macro PIN_LIST should be defined in resources.h file
#define GENERATE_EXTERN_PIN_VARS(name, a, b) extern const pin_t name##_pin;
#define GENERATE_PIN_VARS(name, a, b) const pin_t name##_pin = {#name, a, b};
#define GENERATE_PIN_PTR_ARRAY(name, a, b) &(name##_pin),
typedef struct 
{
  char * name; // This field is used to access pin via CLI 
  GPIO_TypeDef * port; 
  uint32_t pin;
}pin_t;

// X macro way
#define PINMODE_LIST\
  X(INPUT)\
  X(OUTPUT)\
  X(ALT)\
  X(ANALOG)\
  X(UNKNOWN)

#define X(name) name,
typedef enum{ PINMODE_LIST } pin_mode_t;
#undef X

uint8_t pin_snprint(char* buf, uint16_t buflen, const pin_t *pin);

static inline void pin_set(const pin_t * pin)
{
  LL_GPIO_SetOutputPin(pin->port, pin->pin);
}

static inline void pin_reset(const pin_t * pin)
{
  LL_GPIO_ResetOutputPin(pin->port, pin->pin);
}

static inline void pin_toggle(const pin_t * pin)
{
  LL_GPIO_TogglePin(pin->port, pin->pin);
}

static inline uint32_t pin_read(const pin_t * pin)
{
  return LL_GPIO_IsInputPinSet(pin->port, pin->pin);
}

static inline pin_mode_t pin_get_mode(const pin_t * pin)
{
  uint32_t mode = LL_GPIO_GetPinMode(pin->port, pin->pin);
  switch (mode) {
  case LL_GPIO_MODE_INPUT:      return INPUT;
  case LL_GPIO_MODE_OUTPUT:     return OUTPUT;
  case LL_GPIO_MODE_ALTERNATE:  return ALT;
  case LL_GPIO_MODE_ANALOG:     return ANALOG;
  default:                      return UNKNOWN;
  }
}

static inline void pin_set_mode(const pin_t * pin, pin_mode_t dir)
{
  if(dir == OUTPUT) LL_GPIO_SetPinMode(pin->port, pin->pin, LL_GPIO_MODE_OUTPUT);
  if(dir == INPUT)  LL_GPIO_SetPinMode(pin->port, pin->pin, LL_GPIO_MODE_INPUT);
}

#endif

#ifdef PIN_IMPLEMENTATION
#ifndef __PIN_IMPLEMENTATION__
#define __PIN_IMPLEMENTATION__
#define X(name) #name,
static const char *pinmode_str[] = {
  PINMODE_LIST
};
#undef X

#ifndef __SNPRINTF 
#include <stdio.h>
#define __SNPRINTF snprintf
#endif

#include "eprintf_stb.h"

uint8_t pin_snprint(char* buf, uint16_t buflen, const pin_t *pin)
{
  pin_mode_t mode = pin_get_mode(pin);
  
  if((mode != INPUT)&&(mode != OUTPUT))
  {
    return __SNPRINTF(buf, buflen, "%-03s: %-07s\r\n", pin->name, pinmode_str[mode]);
  }
  uint32_t state;
  if(mode == OUTPUT) state = LL_GPIO_IsOutputPinSet(pin->port, pin->pin);
  if(mode == INPUT)  state = LL_GPIO_IsInputPinSet(pin->port, pin->pin);  
  return __SNPRINTF(buf, buflen, "%-03s: %-06s: %i\r\n", pin->name, pinmode_str[mode], state);
}
#endif
#endif