#include "mem_console_prefs.h"

#include <stdio.h>
#include <string.h>

#include "core_pack.h"
#include "runtime/mem_console_prefs_app_io_internal.h"

typedef struct MemConsoleAppPrefsV1 {
    uint32_t version;
    char last_db_path[1024];
} MemConsoleAppPrefsV1;

typedef struct MemConsoleAppPrefsV2 {
    uint32_t version;
    char last_db_path[1024];
    char input_root[1024];
    char output_root[1024];
    char active_db_path[1024];
} MemConsoleAppPrefsV2;

enum {
    MEM_CONSOLE_APP_PREFS_VERSION = 2u
};

int mem_console_build_prefs_path_impl(const char *db_path, char *out_path, size_t out_cap) {
    int written = 0;

    if (!db_path || !out_path || out_cap == 0u) {
        return 0;
    }

    written = snprintf(out_path, out_cap, "%s.ui.pack", db_path);
    if (written <= 0 || (size_t)written >= out_cap) {
        if (out_cap > 0u) {
            out_path[0] = '\0';
        }
        return 0;
    }
    return 1;
}

int mem_console_build_app_prefs_path_impl(char *out_path, size_t out_cap) {
    char data_dir[1024];

    if (!out_path || out_cap == 0u) {
        return 0;
    }
    if (!mem_console_resolve_app_data_dir(data_dir, sizeof(data_dir))) {
        out_path[0] = '\0';
        return 0;
    }
    return mem_console_build_app_prefs_path_for_output_root_impl(data_dir, out_path, out_cap);
}

int mem_console_build_app_prefs_path_for_output_root_impl(const char *output_root,
                                                          char *out_path,
                                                          size_t out_cap) {
    int written = 0;

    if (!output_root || !output_root[0] || !out_path || out_cap == 0u) {
        return 0;
    }

    written = snprintf(out_path, out_cap, "%s/mem_console.app.pack", output_root);
    if (written <= 0 || (size_t)written >= out_cap) {
        out_path[0] = '\0';
        return 0;
    }
    return 1;
}

CoreResult mem_console_app_prefs_load(const char *prefs_path,
                                      char *db_path,
                                      size_t db_path_cap,
                                      char *input_root,
                                      size_t input_root_cap,
                                      char *output_root,
                                      size_t output_root_cap,
                                      char *active_db_path,
                                      size_t active_db_path_cap) {
    CorePackReader reader = {0};
    CorePackChunkInfo chunk = {0};
    CoreResult result;
    MemConsoleAppPrefsV2 prefs = {0};
    FILE *probe = 0;

    if (!prefs_path || !db_path || db_path_cap == 0u || !input_root || input_root_cap == 0u ||
        !output_root || output_root_cap == 0u || !active_db_path || active_db_path_cap == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid app prefs load request" };
    }

    db_path[0] = '\0';
    input_root[0] = '\0';
    output_root[0] = '\0';
    active_db_path[0] = '\0';
    probe = fopen(prefs_path, "rb");
    if (!probe) {
        return core_result_ok();
    }
    fclose(probe);

    result = core_pack_reader_open(prefs_path, &reader);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_pack_reader_find_chunk(&reader, "MCAP", 0, &chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return core_result_ok();
    }

    if (chunk.size != (uint64_t)sizeof(MemConsoleAppPrefsV1) &&
        chunk.size != (uint64_t)sizeof(MemConsoleAppPrefsV2)) {
        (void)core_pack_reader_close(&reader);
        return (CoreResult){ CORE_ERR_FORMAT, "invalid mem_console app prefs payload size" };
    }

    result = core_pack_reader_read_chunk_data(&reader, &chunk, &prefs, sizeof(prefs));
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return result;
    }

    result = core_pack_reader_close(&reader);
    if (result.code != CORE_OK) {
        return result;
    }

    if ((prefs.version != 1u && prefs.version != MEM_CONSOLE_APP_PREFS_VERSION) ||
        prefs.last_db_path[0] == '\0') {
        return core_result_ok();
    }

    if (snprintf(db_path, db_path_cap, "%s", prefs.last_db_path) <= 0 ||
        strlen(prefs.last_db_path) >= db_path_cap) {
        db_path[0] = '\0';
        return (CoreResult){ CORE_ERR_FORMAT, "app prefs db path too long" };
    }

    if (prefs.version >= 2u) {
        if (prefs.input_root[0] &&
            (snprintf(input_root, input_root_cap, "%s", prefs.input_root) <= 0 ||
             strlen(prefs.input_root) >= input_root_cap)) {
            input_root[0] = '\0';
            return (CoreResult){ CORE_ERR_FORMAT, "app prefs input root too long" };
        }
        if (prefs.output_root[0] &&
            (snprintf(output_root, output_root_cap, "%s", prefs.output_root) <= 0 ||
             strlen(prefs.output_root) >= output_root_cap)) {
            output_root[0] = '\0';
            return (CoreResult){ CORE_ERR_FORMAT, "app prefs output root too long" };
        }
        if (prefs.active_db_path[0] &&
            (snprintf(active_db_path, active_db_path_cap, "%s", prefs.active_db_path) <= 0 ||
             strlen(prefs.active_db_path) >= active_db_path_cap)) {
            active_db_path[0] = '\0';
            return (CoreResult){ CORE_ERR_FORMAT, "app prefs active db path too long" };
        }
    }

    return (CoreResult){ CORE_OK, "app prefs loaded" };
}

CoreResult mem_console_app_prefs_save(const char *prefs_path,
                                      const char *db_path,
                                      const char *input_root,
                                      const char *output_root,
                                      const char *active_db_path) {
    CorePackWriter writer = {0};
    CoreResult result;
    MemConsoleAppPrefsV2 prefs = {0};

    if (!prefs_path || !db_path || !db_path[0]) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid app prefs save request" };
    }
    if (!mem_console_ensure_parent_directory(prefs_path)) {
        return (CoreResult){ CORE_ERR_IO, "failed to create app prefs directory" };
    }

    prefs.version = MEM_CONSOLE_APP_PREFS_VERSION;
    (void)snprintf(prefs.last_db_path, sizeof(prefs.last_db_path), "%s", db_path);
    if (input_root && input_root[0]) {
        (void)snprintf(prefs.input_root, sizeof(prefs.input_root), "%s", input_root);
    }
    if (output_root && output_root[0]) {
        (void)snprintf(prefs.output_root, sizeof(prefs.output_root), "%s", output_root);
    }
    if (active_db_path && active_db_path[0]) {
        (void)snprintf(prefs.active_db_path, sizeof(prefs.active_db_path), "%s", active_db_path);
    }

    result = core_pack_writer_open(prefs_path, &writer);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_pack_writer_add_chunk(&writer, "MCAP", &prefs, (uint64_t)sizeof(prefs));
    if (result.code != CORE_OK) {
        (void)core_pack_writer_close(&writer);
        return result;
    }

    result = core_pack_writer_close(&writer);
    if (result.code != CORE_OK) {
        return result;
    }

    return core_result_ok();
}
