#include "snprintf_compat.h"

#include <string.h>
#include <stdlib.h>

#include "ept_settings.h"

#define FLASH_LL_IMPLEMENTATION
#include "FLASH_LL.h"

settings_t settings;

// ---- Metadata ---------------------------------------------------------------
typedef enum { STYPE_INT32, STYPE_STR } setting_type_t;

typedef struct {
    const char    *name;
    setting_type_t type;
    uint16_t       offset;
    uint16_t       maxlen;
    int32_t        min;
    int32_t        max;
    int32_t        dflt_int;
    const char    *dflt_str;
    void         (*apply_fn)(void);
} setting_meta_t;

static const setting_meta_t meta[] = {
#define SETTING_INT(name, dflt, min, max, fn) \
    { #name, STYPE_INT32, (uint16_t)offsetof(settings_t, name), sizeof(int32_t), (min), (max), (dflt), NULL, (fn) },
#define SETTING_STR(name, maxlen, dflt, fn) \
    { #name, STYPE_STR,   (uint16_t)offsetof(settings_t, name), (maxlen),        0,     0,     0,      (dflt), (fn) },
SETTINGS_LIST
#undef SETTING_INT
#undef SETTING_STR
};

#define META_COUNT (sizeof(meta) / sizeof(meta[0]))

// ---- Apply ------------------------------------------------------------------
void settings_apply(void)
{
    for (uint8_t i = 0; i < META_COUNT; i++)
        if (meta[i].apply_fn) meta[i].apply_fn();
}

// ---- Defaults ---------------------------------------------------------------
void settings_defaults(void)
{
    for (uint8_t i = 0; i < META_COUNT; i++) {
        char *dst = (char *)&settings + meta[i].offset;
        if (meta[i].type == STYPE_INT32) {
            *(int32_t *)dst = meta[i].dflt_int;
        } else {
            strncpy(dst, meta[i].dflt_str, meta[i].maxlen - 1);
            dst[meta[i].maxlen - 1] = '\0';
        }
    }
}

// ---- CRC-32 (software, no table) --------------------------------------------
static uint32_t settings_crc32(const uint8_t *data, uint16_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    while (len--) {
        crc ^= *data++;
        for (uint8_t b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
    }
    return ~crc;
}

// ---- Flash backend ------------------------------------------------------------
// Define SETTINGS_FLASH_ADDR in settings_cfg.h to enable.
// The address must be the start of a flash page not used by the linker.
// Erase/write are provided externally (FLASH_LL.h):
//   void flash_page_erase(uint32_t addr);
//   void flash_write_data(uint32_t addr, uint8_t *data, uint16_t len);
#ifdef SETTINGS_FLASH_ADDR

typedef struct { settings_t s; uint32_t crc; } flash_image_t;

void settings_save(void)
{
    flash_image_t img;
    img.s   = settings;
    img.crc = settings_crc32((const uint8_t *)&settings, (uint16_t)sizeof(settings_t));
    flash_page_erase(SETTINGS_FLASH_ADDR);
    flash_write_data(SETTINGS_FLASH_ADDR, (uint8_t *)&img, (uint16_t)sizeof(img));
}

void settings_load(void)
{
    const flash_image_t *img = (const flash_image_t *)SETTINGS_FLASH_ADDR;
    if (settings_crc32((const uint8_t *)&img->s, (uint16_t)sizeof(settings_t)) == img->crc)
        settings = img->s;
    else
        settings_defaults();
}

#endif // SETTINGS_FLASH_ADDR

// ---- set_cmd ----------------------------------------------------------------
static uint16_t setting_snprint(char *buf, uint16_t buflen, const setting_meta_t *m)
{
    const char *val = (const char *)&settings + m->offset;
    if (m->type == STYPE_INT32)
        return (uint16_t)__SNPRINTF(buf, buflen, "%-16s %i [%i..%i]",
                                    m->name, (int)*(const int32_t *)val,
                                    (int)m->min, (int)m->max);
    return (uint16_t)__SNPRINTF(buf, buflen, "%-16s \"%s\"", m->name, val);
}

static uint16_t set_list_gen(uint16_t state, char *buf, uint16_t buflen)
{
    if (state >= META_COUNT) return 0;
    uint16_t n = setting_snprint(buf, buflen, &meta[state]);
    if (n + 2 < buflen) { buf[n++] = '\r'; buf[n++] = '\n'; buf[n] = '\0'; }
    return n;
}

CLI_retval_t set_cmd(char *args[], uint8_t argno)
{
    if (argno == 0) { cli_gen = set_list_gen; return CLI_LONG_RESPONSE; }

    if (strcmp(args[0], "save") == 0) { settings_save();                        return CLI_OK; }
    if (strcmp(args[0], "load") == 0) { settings_load();    settings_apply();   return CLI_OK; }
    if (strcmp(args[0], "dflt") == 0) { settings_defaults(); settings_apply();  return CLI_OK; }

    const setting_meta_t *m = FIND_BY_NAME(meta, args[0]);

    if (m == NULL) {
        __SNPRINTF(cli_response_buf, cli_max_resp_len, "\r\nUnknown: %s", args[0]);
        return CLI_ERROR;
    }

    if (argno == 1) {
        setting_snprint(cli_response_buf, cli_max_resp_len, m);
        return CLI_OK;
    }

    if (setting_set(args[0], args[1]) != 0) {
        setting_snprint(cli_response_buf, cli_max_resp_len, m);
        return CLI_ERROR;
    }
    return CLI_OK;
}

uint8_t setting_set(const char *name, const char *value_str)
{
    const setting_meta_t *m = FIND_BY_NAME(meta, name);
    if (m == NULL) return 1;

    char *val = (char *)&settings + m->offset;

    if (m->type == STYPE_INT32) {
        char *end;
        long v = strtol(value_str, &end, 10);
        if (end == value_str || v < m->min || v > m->max) return 1;
        *(int32_t *)val = (int32_t)v;
    } else {
        uint16_t slen = (uint16_t)strlen(value_str);
        if (slen >= m->maxlen) return 1;
        memcpy(val, value_str, slen + 1u);
    }

    if (m->apply_fn) m->apply_fn();
    return 0;
}
