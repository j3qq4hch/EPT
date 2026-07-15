#ifndef EPT_SETTINGS_H_
#define EPT_SETTINGS_H_

#include <stdint.h>
#include <stddef.h>

#include "settings_cfg.h"
#include "ept_cli.h"
#include "named_lookup.h"

// Provide in settings_cfg.h:
//   SETTINGS_LIST — X-macro defining all settings, e.g.:
//
//   void baudrate_apply(void);
//   void brightness_apply(void);
//
//   #define SETTINGS_LIST \
//       SETTING_INT(baudrate,   9600, 300, 115200, baudrate_apply)   \
//       SETTING_INT(brightness, 50,   0,   100,    brightness_apply) \
//       SETTING_STR(dev_name,   16,   "EPT",       NULL)
//
//   SETTING_INT(name, default, min, max, apply_fn)
//   SETTING_STR(name, maxlen, default, apply_fn)
//   apply_fn: void fn(void), or NULL

// ---- settings_t (generated from SETTINGS_LIST) ------------------------------
typedef struct {
#define SETTING_INT(name, dflt, min, max, fn)  int32_t name;
#define SETTING_STR(name, maxlen, dflt, fn)    char    name[maxlen];
SETTINGS_LIST
#undef SETTING_INT
#undef SETTING_STR
} settings_t;

#ifdef SETTINGS_FLASH_PAGE_SIZE
_Static_assert(sizeof(settings_t) + sizeof(uint32_t) <= SETTINGS_FLASH_PAGE_SIZE,
               "settings_t + CRC must fit in one flash page (SETTINGS_FLASH_PAGE_SIZE)");
#endif

extern settings_t settings;

void         settings_load    (void);
void         settings_save    (void);
void         settings_defaults(void);
void         settings_apply   (void);
uint8_t      setting_set      (const char *name, const char *value_str);
CLI_retval_t set_cmd          (char *args[], uint8_t argno);

#endif // EPT_SETTINGS_H_
