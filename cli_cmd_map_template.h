#ifndef CLI_CMD_MAP_H_
#define CLI_CMD_MAP_H_

#include "ept_cli.h"

// Add one line per command: X(name, "description")
// The handler must be named name_cmd. Library-provided handlers:
//   h, ept, dbg, loops — always (ept_cli.c / ept_scheduler.c)
//   set                — if ept_settings.c is in the build
//   prof               — if PROFILER is enabled
// Any other command needs a CLI_retval_t name_cmd(char *args[], uint8_t argno)
// defined somewhere in the project.
#define CMD_LIST            \
    X(h,     "help")       \
    X(ept,   "list/control threads: ept [<name> run|stop]") \
    X(dbg,   "set thread debug level: dbg <name> <level>")  \
    X(loops, "scheduler passes per second (call twice, >=1s apart)")
//  X(set,   "show/change settings: set [<name> [<value>]] | save | load | dflt")
//  X(prof,  "profiler output")

#define X(name, help) CLI_retval_t name##_cmd(char *args[], uint8_t argno);
CMD_LIST
#undef X

static const CLI_cmd_t cmd_map[] = {
#define X(name, help) { #name, name##_cmd, help },
CMD_LIST
#undef X
};

#endif // CLI_CMD_MAP_H_
