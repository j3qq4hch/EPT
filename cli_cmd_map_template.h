#ifndef CLI_CMD_MAP_H_
#define CLI_CMD_MAP_H_

#include "ept_cli.h"

// Add one line per command: X(name, "description")
// Handler must be named name_cmd and defined somewhere in the project.
// h, ept, dbg are built-in — provided by the library.
// If PROFILER is enabled, add prof to the list.
#define CMD_LIST            \
    X(h,   "help")         \
    X(ept, "list/control threads: ept [<name> run|stop]") \
    X(dbg, "set thread debug level: dbg <name> <level>")  \
    X(id,  "show firmware id")

#define X(name, help) CLI_retval_t name##_cmd(char *args[], uint8_t argno);
CMD_LIST
#undef X

static const CLI_cmd_t cmd_map[] = {
#define X(name, help) { #name, name##_cmd, help },
CMD_LIST
#undef X
};

#endif // CLI_CMD_MAP_H_
