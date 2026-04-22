#include "mem_console_app_internal.h"

#include <SDL2/SDL.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "mem_console_prefs.h"
#include "mem_console_ui_graph.h"

extern char **environ;

static int mem_console_path_has_md_suffix(const char *path) {
    size_t len;

    if (!path) {
        return 0;
    }
    len = strlen(path);
    if (len < 3u) {
        return 0;
    }
    return path[len - 3u] == '.' &&
           (path[len - 2u] == 'm' || path[len - 2u] == 'M') &&
           (path[len - 1u] == 'd' || path[len - 1u] == 'D');
}

static int mem_console_path_is_regular_file(const char *path) {
    struct stat st = {0};

    if (!path || !path[0]) {
        return 0;
    }
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode) ? 1 : 0;
}

static int mem_console_open_path_with_system_default(const char *path) {
#if defined(__APPLE__)
    pid_t pid = (pid_t)0;
    char *const argv[] = { "open", (char *)path, 0 };
    int spawn_rc;

    if (!path || !path[0]) {
        return 0;
    }

    spawn_rc = posix_spawnp(&pid, "open", 0, 0, argv, environ);
    if (spawn_rc != 0) {
        return 0;
    }
    return 1;
#else
    (void)path;
    return 0;
#endif
}

static int mem_console_pick_folder_macos(char *out_path, size_t out_cap) {
#if defined(__APPLE__)
    FILE *pipe = 0;
    size_t len = 0u;
    int c = 0;

    if (!out_path || out_cap == 0u) {
        return 0;
    }
    out_path[0] = '\0';
    pipe = popen("/usr/bin/osascript -e 'POSIX path of (choose folder with prompt \"Choose MemConsole Input Root\")'",
                 "r");
    if (!pipe) {
        return 0;
    }
    while ((c = fgetc(pipe)) != EOF && len + 1u < out_cap) {
        out_path[len++] = (char)c;
    }
    out_path[len] = '\0';
    (void)pclose(pipe);
    while (len > 0u && (out_path[len - 1u] == '\n' || out_path[len - 1u] == '\r')) {
        out_path[len - 1u] = '\0';
        len -= 1u;
    }
    return out_path[0] != '\0';
#else
    (void)out_path;
    (void)out_cap;
    return 0;
#endif
}

void mem_console_app_set_action_error_status(MemConsoleState *state,
                                             const char *prefix,
                                             CoreResult result) {
    const char *message;

    if (!state || !prefix) {
        return;
    }

    message = result.message ? result.message : "error";
    (void)snprintf(state->status_line,
                   sizeof(state->status_line),
                   "%s: %s",
                   prefix,
                   message);
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

void mem_console_app_refresh_and_report(CoreMemDb *db,
                                        MemConsoleState *state,
                                        const char *error_prefix) {
    CoreResult result;

    if (!db || !state || !error_prefix) {
        return;
    }

    result = refresh_state_from_db(db, state);
    if (result.code != CORE_OK) {
        mem_console_app_set_action_error_status(state, error_prefix, result);
        return;
    }
    sync_edit_buffers_from_selection(state);
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

void mem_console_app_apply_pending_action(CoreMemDb *db,
                                          MemConsoleState *state,
                                          MemConsoleRuntime *runtime,
                                          const char *app_prefs_path,
                                          int app_prefs_path_valid,
                                          MemConsoleAction action) {
    CoreResult result;

    if (!db || !state) {
        return;
    }

    if (action == MEM_CONSOLE_ACTION_NONE) {
        return;
    }

    if (action == MEM_CONSOLE_ACTION_REFRESH) {
        mem_console_app_refresh_and_report(db, state, "Refresh failed");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CREATE_FROM_SEARCH) {
        int64_t created_id = 0;
        result = create_item_from_search(db, state, &created_id);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Create failed", result);
            return;
        }

        state->selected_item_id = created_id;
        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Refresh failed", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        mem_console_runtime_note_local_write(runtime, SDL_GetTicks64());

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Created memory %lld from search text.",
                       (long long)created_id);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_TITLE_EDIT) {
        begin_title_edit_mode(state);
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Title edit mode enabled.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CANCEL_TITLE_EDIT) {
        cancel_title_edit_mode(state);
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Title edit cancelled.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_SAVE_TITLE_EDIT) {
        int64_t edited_id = state->selected_item_id;
        result = rename_selected_from_title_buffer(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Save title failed", result);
            return;
        }

        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Refresh failed", result);
            return;
        }
        cancel_title_edit_mode(state);
        sync_edit_buffers_from_selection(state);
        mem_console_runtime_note_local_write(runtime, SDL_GetTicks64());

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Saved title for memory %lld.",
                       (long long)edited_id);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_BODY_EDIT) {
        begin_body_edit_mode(state);
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Body edit mode enabled.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CANCEL_BODY_EDIT) {
        cancel_body_edit_mode(state);
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Body edit cancelled.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_SAVE_BODY_EDIT) {
        int64_t edited_id = state->selected_item_id;
        result = replace_selected_body_from_body_buffer(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Save body failed", result);
            return;
        }

        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Refresh failed", result);
            return;
        }
        cancel_body_edit_mode(state);
        sync_edit_buffers_from_selection(state);
        mem_console_runtime_note_local_write(runtime, SDL_GetTicks64());

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Saved body for memory %lld.",
                       (long long)edited_id);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_TOGGLE_PINNED) {
        int next_value = state->selected_pinned ? 0 : 1;
        int64_t toggled_id = state->selected_item_id;

        result = set_selected_flag(db, state, "pinned", next_value);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Pinned toggle failed", result);
            return;
        }

        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Refresh failed", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        mem_console_runtime_note_local_write(runtime, SDL_GetTicks64());

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Pinned %s for memory %lld.",
                       next_value ? "enabled" : "disabled",
                       (long long)toggled_id);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_TOGGLE_CANONICAL) {
        int next_value = state->selected_canonical ? 0 : 1;
        int64_t toggled_id = state->selected_item_id;

        result = set_selected_flag(db, state, "canonical", next_value);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Canonical toggle failed", result);
            return;
        }

        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Refresh failed", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        mem_console_runtime_note_local_write(runtime, SDL_GetTicks64());

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Canonical %s for memory %lld.",
                       next_value ? "enabled" : "disabled",
                       (long long)toggled_id);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_TOGGLE_GRAPH_MODE) {
        state->graph_mode_enabled = 1;
        state->graph_scope_full_mode_enabled = state->graph_scope_full_mode_enabled ? 0 : 1;

        result = load_graph_neighborhood(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Graph load failed", result);
            return;
        }

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Graph scope %s (%d nodes, %d edges, kind=%s, limit=%d, hops=%d).",
                       state->graph_scope_full_mode_enabled ? "full" : "focus",
                       state->graph_node_count,
                       state->graph_edge_count,
                       state->graph_kind_filter[0] ? state->graph_kind_filter : "all",
                       state->graph_query_edge_limit,
                       state->graph_query_hops);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_REFRESH_GRAPH) {
        result = load_graph_neighborhood(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Graph refresh failed", result);
            return;
        }

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Graph refreshed (%d nodes, %d edges, kind=%s, limit=%d, hops=%d).",
                       state->graph_node_count,
                       state->graph_edge_count,
                       state->graph_kind_filter[0] ? state->graph_kind_filter : "all",
                       state->graph_query_edge_limit,
                       state->graph_query_hops);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CENTER_GRAPH) {
        result = mem_console_ui_graph_center_layout_view(state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Center graph failed", result);
            return;
        }

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Graph viewport centered.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CENTER_SELECTED) {
        result = mem_console_ui_graph_center_selected_view(state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Center selected failed", result);
            return;
        }

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Centered selected memory %lld.",
                       (long long)state->selected_item_id);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_OPEN_REFERENCE_PATH) {
        if (!state->detail_reference_path_available || !state->detail_reference_path[0]) {
            (void)snprintf(state->status_line, sizeof(state->status_line), "No markdown reference path available.");
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            return;
        }
        if (!mem_console_path_has_md_suffix(state->detail_reference_path) ||
            !mem_console_path_is_regular_file(state->detail_reference_path)) {
            (void)snprintf(state->status_line,
                           sizeof(state->status_line),
                           "Reference path is unavailable: %s",
                           state->detail_reference_path);
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            return;
        }
        if (!mem_console_open_path_with_system_default(state->detail_reference_path)) {
            (void)snprintf(state->status_line, sizeof(state->status_line), "Failed to open reference path.");
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            return;
        }

        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Opened reference path in default app.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_DB_PICKER) {
        begin_db_picker_mode(state, 0);
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Select .sqlite from input root list or enter exact DB path.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_DB_CREATE) {
        begin_db_picker_mode(state, 1);
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Enter DB name/path to create (name -> input root, path -> explicit).");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CANCEL_DB_PICKER) {
        int canceling_input_root = state->db_modal_input_root_mode;
        cancel_db_picker_mode(state);
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       canceling_input_root ? "Input root change cancelled." : "Database change cancelled.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER) {
        char next_db_path[1024];
        int create_mode = state->db_modal_create_mode ? 1 : 0;

        if (!mem_console_db_picker_build_path(state, next_db_path, sizeof(next_db_path))) {
            (void)snprintf(state->status_line, sizeof(state->status_line), "Enter a valid DB path first.");
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            return;
        }
        if (state->db_modal_input_root_mode) {
            CoreResult save_result = core_result_ok();
            if (!mem_console_ensure_directory(next_db_path)) {
                (void)snprintf(state->status_line, sizeof(state->status_line), "Input root must be a valid directory.");
                mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                return;
            }
            (void)snprintf(state->input_root, sizeof(state->input_root), "%s", next_db_path);
            cancel_db_picker_mode(state);
            if (app_prefs_path_valid && app_prefs_path && app_prefs_path[0]) {
                save_result = mem_console_app_prefs_save(app_prefs_path,
                                                         state->db_path,
                                                         state->input_root,
                                                         state->output_root,
                                                         state->active_db_path);
            }
            if (save_result.code != CORE_OK) {
                (void)snprintf(state->status_line, sizeof(state->status_line), "Input root set; app prefs save failed.");
            } else {
                (void)snprintf(state->status_line, sizeof(state->status_line), "Input root set to %s.", state->input_root);
            }
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            return;
        }
        cancel_db_picker_mode(state);
        (void)snprintf(state->pending_db_path, sizeof(state->pending_db_path), "%s", next_db_path);
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       create_mode ? "Creating/switching DB to %s..." : "Switching DB to %s...",
                       state->pending_db_path);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_INPUT_ROOT_PICKER) {
        begin_input_root_picker_mode(state);
        (void)snprintf(state->status_line, sizeof(state->status_line), "Enter input root path and press Enter.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_PICK_INPUT_ROOT_FOLDER) {
        char picked_root[1024];
        CoreResult save_result = core_result_ok();

        if (!mem_console_pick_folder_macos(picked_root, sizeof(picked_root))) {
            (void)snprintf(state->status_line, sizeof(state->status_line), "Folder dialog canceled/unavailable.");
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            return;
        }
        if (!mem_console_ensure_directory(picked_root)) {
            (void)snprintf(state->status_line, sizeof(state->status_line), "Failed to use selected input root.");
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            return;
        }
        (void)snprintf(state->input_root, sizeof(state->input_root), "%s", picked_root);
        if (app_prefs_path_valid && app_prefs_path && app_prefs_path[0]) {
            save_result = mem_console_app_prefs_save(app_prefs_path,
                                                     state->db_path,
                                                     state->input_root,
                                                     state->output_root,
                                                     state->active_db_path);
        }
        if (save_result.code != CORE_OK) {
            (void)snprintf(state->status_line, sizeof(state->status_line), "Input root set; app prefs save failed.");
        } else {
            (void)snprintf(state->status_line, sizeof(state->status_line), "Input root updated from folder dialog.");
        }
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }
}
