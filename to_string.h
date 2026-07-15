#ifndef TO_STRING_H_
#define TO_STRING_H_

#include <stdint.h>
#include "drv_basic.h"
#include "ept_scheduler.h"
#include "resources.h"
#include "color.h"
// Provide in resources.h (optional — omit if no project-specific types):
//   #define EPT_TYPES_LIST \
//       my_sensor_t *: my_sensor_snprint, \
//       const my_sensor_t *: my_sensor_snprint,

#ifndef EPT_TYPES_LIST
#define EPT_TYPES_LIST
#endif

#include "snprintf_compat.h"

static inline uint16_t unknown_snprint(char *buf, uint16_t buflen, const void *p)
{
    (void)p;
    return (uint16_t)__SNPRINTF(buf, buflen, "?");
}

#define to_string(buf, len, ptr) _Generic((ptr),              \
    const thread_record *: thread_record_snprint,             \
          thread_record *: thread_record_snprint,             \
    const pin_t *:         pin_snprint,                       \
          pin_t *:         pin_snprint,                       \
    const uart_t *:        uart_snprint,                      \
          uart_t *:        uart_snprint,                      \
    EPT_TYPES_LIST                                            \
    default:               unknown_snprint                    \
)(buf, len, ptr)

#endif // TO_STRING_H_
