#ifndef SETTINGS_CFG_H_
#define SETTINGS_CFG_H_

// Template for the project-provided settings_cfg.h (required by ept_settings).
// Copy into your project, rename to settings_cfg.h, fill in.

// ---- Flash location -----------------------------------------------------------
// One flash page reserved for settings. The address must be page-aligned and
// the page must be excluded from the linker's ROM region (reserve it in your
// linker script / .icf). Page size: see FLASH_PAGE_SIZE in your drv_basic port.
// Omit SETTINGS_FLASH_ADDR to build ept_settings without the flash backend
// (settings_save()/settings_load() then unavailable, the rest still works).

// #define SETTINGS_FLASH_PAGE_SIZE 2048
// #define SETTINGS_FLASH_ADDR      (0x08004000 - SETTINGS_FLASH_PAGE_SIZE) // last page of 16K

// ---- Settings list --------------------------------------------------------------
// SETTING_INT(name, default, min, max, apply_fn)
// SETTING_STR(name, maxlen, default, apply_fn)
// apply_fn: void fn(void) called when the value changes (and by settings_apply());
// NULL if nothing needs to react.

// void brightness_apply(void);

#define SETTINGS_LIST \
    SETTING_INT(brightness, 50, 0, 100, NULL)          /* example */ \
    SETTING_STR(dev_name,   16, "EPT", NULL)           /* example */

#endif // SETTINGS_CFG_H_
