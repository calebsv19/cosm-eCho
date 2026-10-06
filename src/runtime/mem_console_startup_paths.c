#include "runtime/mem_console_startup_paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem_console_prefs.h"
#include "runtime/mem_console_prefs_app_io_internal.h"

static int load_preferences(const char *path, MemConsoleStartupPaths *prefs) {
    char active_db[1024];
    CoreResult result = mem_console_app_prefs_load(path,
        prefs->db_path, sizeof(prefs->db_path),
        prefs->input_root, sizeof(prefs->input_root),
        prefs->output_root, sizeof(prefs->output_root), active_db, sizeof(active_db));
    if (result.code == CORE_OK && result.message && strcmp(result.message, "app prefs loaded") == 0) {
        return 1;
    }
    memset(prefs, 0, sizeof(*prefs));
    return 0;
}

CoreResult mem_console_startup_paths_resolve(const char *explicit_db,
                                            MemConsoleStartupPaths *out) {
    char runtime_root[1024];
    char prefs_path[1200];
    char default_db[1024];
    MemConsoleStartupPaths prefs = {0};
    const char *home_path = getenv("HOME");
    const char *runtime_override = getenv("MEM_CONSOLE_RUNTIME_DIR");
    const char *env_db = getenv("CODEWORK_MEMDB_PATH");
    const char *launcher_default = getenv("MEM_CONSOLE_LAUNCHER_DEFAULT_DB");
    const char *selected_db;
    int isolated = mem_console_runtime_is_isolated();
    int loaded;

    if (!out) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "missing startup paths" };
    }
    memset(out, 0, sizeof(*out));
    if (!mem_console_resolve_app_data_dir(runtime_root, sizeof(runtime_root)) ||
        !mem_console_build_app_prefs_path_for_output_root_impl(runtime_root, prefs_path, sizeof(prefs_path))) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid runtime root" };
    }
    loaded = load_preferences(prefs_path, &prefs);
    /* Only the standard runtime retains its historical preference migration. */
    if (!loaded && !isolated && home_path && home_path[0]) {
        int written = snprintf(prefs_path, sizeof(prefs_path),
            "%s/.local/share/mem_console/mem_console.app.pack", home_path);
        if (written > 0 && (size_t)written < sizeof(prefs_path)) {
            loaded = load_preferences(prefs_path, &prefs);
        }
    }
    selected_db = explicit_db && explicit_db[0] ? explicit_db : 0;
    if (!selected_db && env_db && env_db[0] &&
        (!launcher_default || strcmp(env_db, launcher_default) != 0)) {
        selected_db = env_db;
    }
    if (!selected_db && loaded) {
        selected_db = prefs.db_path;
    }
    if (!selected_db) {
        if (!resolve_default_db_path(default_db, sizeof(default_db))) {
            return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid default database path" };
        }
        selected_db = default_db;
    }
    /* Keep intentional DB choices, but never let saved output roots escape an
       explicitly selected runtime. Existing files are neither moved nor removed. */
    if (!mem_console_ensure_parent_directory(runtime_root) ||
        !mem_console_path_contract_normalize(prefs.input_root,
            isolated || (runtime_override && runtime_override[0]) ? runtime_root : prefs.output_root,
            selected_db, out->input_root, sizeof(out->input_root),
            out->output_root, sizeof(out->output_root), out->db_path, sizeof(out->db_path))) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "failed to normalize runtime path contract" };
    }
    return core_result_ok();
}
