#include "mem_console_app_internal.h"

#include <stdarg.h>
#include <stdio.h>

void mem_console_app_set_statusf(MemConsoleState *state, const char *fmt, ...) {
    va_list args;

    if (!state || !fmt) {
        return;
    }

    va_start(args, fmt);
    (void)vsnprintf(state->status_line, sizeof(state->status_line), fmt, args);
    va_end(args);
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

void mem_console_app_set_path_result_status(MemConsoleState *state,
                                            const char *operation,
                                            const char *path,
                                            CoreResult result) {
    const char *message;

    if (!state || !operation) {
        return;
    }

    message = result.message ? result.message : "error";
    if (path && path[0]) {
        mem_console_app_set_statusf(state, "%s failed for %s: %s", operation, path, message);
    } else {
        mem_console_app_set_statusf(state, "%s failed: %s", operation, message);
    }
}
