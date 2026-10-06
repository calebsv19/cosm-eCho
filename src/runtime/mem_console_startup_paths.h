#ifndef MEM_CONSOLE_STARTUP_PATHS_H
#define MEM_CONSOLE_STARTUP_PATHS_H

#include "core_base.h"

typedef struct MemConsoleStartupPaths {
    char input_root[1024];
    char output_root[1024];
    char db_path[1024];
} MemConsoleStartupPaths;

/* Reads preference metadata and creates directories, but never opens a DB. */
CoreResult mem_console_startup_paths_resolve(const char *explicit_db,
                                            MemConsoleStartupPaths *out);

#endif
