#include "mem_console_app_internal.h"

#include <stdio.h>

#include "mem_console_prefs.h"

CoreResult mem_console_app_switch_active_db(CoreMemDb *db,
                                            MemConsoleState *state,
                                            KitRenderContext *render_ctx,
                                            KitUiContext *ui_ctx,
                                            const char *next_db_path,
                                            const char *app_prefs_path,
                                            int app_prefs_path_valid,
                                            char *prefs_path,
                                            size_t prefs_path_cap,
                                            int *prefs_path_valid,
                                            int *prefs_signature_valid,
                                            uint64_t *prefs_last_saved_signature) {
    CoreResult result;
    CoreMemDb next_db = {0};
    int kernel_bridge_enabled;
    char input_root[1024];
    char output_root[1024];

    if (!db || !state || !render_ctx || !ui_ctx || !next_db_path || !next_db_path[0] || !prefs_path ||
        !prefs_path_valid || !prefs_signature_valid || !prefs_last_saved_signature) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid db switch request" };
    }

    kernel_bridge_enabled = state->kernel_bridge_enabled;
    (void)snprintf(input_root, sizeof(input_root), "%s", state->input_root);
    (void)snprintf(output_root, sizeof(output_root), "%s", state->output_root);

    if (!mem_console_ensure_parent_directory(next_db_path)) {
        return (CoreResult){ CORE_ERR_IO, "failed to create DB directory" };
    }

    result = core_memdb_open(next_db_path, &next_db);
    if (result.code != CORE_OK) {
        return result;
    }

    if (*prefs_path_valid && state->pane_prefs_dirty) {
        result = mem_console_prefs_save(prefs_path, state);
        if (result.code != CORE_OK) {
            (void)core_memdb_close(&next_db);
            return result;
        }
    }

    result = core_memdb_close(db);
    if (result.code != CORE_OK) {
        (void)core_memdb_close(&next_db);
        return result;
    }

    seed_state(state, next_db_path);
    mem_console_state_set_path_contract(state, input_root, output_root, next_db_path);
    state->kernel_bridge_enabled = kernel_bridge_enabled;

    *prefs_path_valid = mem_console_build_prefs_path(state->db_path, prefs_path, prefs_path_cap);
    if (*prefs_path_valid) {
        result = mem_console_prefs_load(prefs_path, state);
        if (result.code != CORE_OK) {
            (void)snprintf(state->status_line, sizeof(state->status_line), "UI prefs load failed.");
        }
    }

    *db = next_db;

    result = refresh_state_from_db(db, state);
    if (result.code != CORE_OK) {
        return result;
    }
    sync_edit_buffers_from_selection(state);
    result = kit_render_set_theme_preset(render_ctx, state->theme_preset_id);
    if (result.code != CORE_OK) {
        return result;
    }
    result = kit_render_set_font_preset(render_ctx, state->font_preset_id);
    if (result.code != CORE_OK) {
        return result;
    }
    result = kit_render_set_text_zoom_step(render_ctx, state->text_zoom_step);
    if (result.code != CORE_OK) {
        return result;
    }
    (void)kit_ui_style_apply_theme_scale(ui_ctx);
    mem_console_app_apply_compact_ui_density(ui_ctx, render_ctx);

    if (*prefs_path_valid) {
        *prefs_last_saved_signature = mem_console_prefs_state_signature(state);
        *prefs_signature_valid = 1;
        state->pane_prefs_dirty = 0;
    } else {
        *prefs_signature_valid = 0;
    }

    if (app_prefs_path_valid) {
        result = mem_console_app_prefs_save(app_prefs_path,
                                            state->db_path,
                                            state->input_root,
                                            state->output_root,
                                            state->active_db_path);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    (void)snprintf(state->status_line, sizeof(state->status_line), "Active DB switched to %s.", state->db_path);
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT | MEM_CONSOLE_REDRAW_REASON_LAYOUT);
    return core_result_ok();
}
