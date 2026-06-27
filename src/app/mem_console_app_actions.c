#include "mem_console_app_internal.h"

#include <SDL2/SDL.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "mem_console_action_roles.h"
#include "mem_console_prefs.h"
#include "mem_console_ui_graph.h"

extern char **environ;

static const char *mem_console_graph_view_mode_status_text(const MemConsoleState *state) {
    int view_mode = mem_console_graph_view_mode_get(state);

    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_PODS) {
        return "pods";
    }
    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        return "web";
    }
    return "focus";
}

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

static void mem_console_app_note_db_write(MemConsoleRuntime *runtime) {
    mem_console_runtime_note_local_write(runtime, SDL_GetTicks64());
}

static void mem_console_app_set_post_mutation_refresh_error_status(MemConsoleState *state,
                                                                   const char *mutation,
                                                                   CoreResult result) {
    char prefix[96];

    if (!state || !mutation) {
        return;
    }

    (void)snprintf(prefix, sizeof(prefix), "Refresh after %s failed", mutation);
    mem_console_app_set_action_error_status(state, prefix, result);
}

static void mem_console_app_set_graph_action_error_status(MemConsoleState *state,
                                                          const char *operation,
                                                          CoreResult result) {
    const char *message;
    const char *kind;

    if (!state || !operation) {
        return;
    }

    message = result.message ? result.message : "error";
    kind = state->graph_kind_filter[0] ? state->graph_kind_filter : "all";
    mem_console_app_set_statusf(state,
                                "Graph %s failed (selected=%lld center=%lld kind=%s limit=%d hops=%d): %s",
                                operation,
                                (long long)state->selected_item_id,
                                (long long)state->graph_center_item_id,
                                kind,
                                state->graph_query_edge_limit,
                                state->graph_query_hops,
                                message);
}

static void mem_console_app_set_relationship_action_error_status(MemConsoleState *state,
                                                                 const char *operation,
                                                                 CoreResult result) {
    const char *message;
    const char *target_text;

    if (!state || !operation) {
        return;
    }

    message = result.message ? result.message : "error";
    target_text = state->relationship_target_text[0] ? state->relationship_target_text : "-";
    mem_console_app_set_statusf(state,
                                "Relationship %s failed (selected=%lld link=%lld target=%s): %s",
                                operation,
                                (long long)state->selected_item_id,
                                (long long)state->relationship_action_link_id,
                                target_text,
                                message);
}

void mem_console_app_set_action_error_status(MemConsoleState *state,
                                             const char *prefix,
                                             CoreResult result) {
    const char *message;

    if (!state || !prefix) {
        return;
    }

    message = result.message ? result.message : "error";
    mem_console_app_set_statusf(state, "%s: %s", prefix, message);
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

    if (mem_console_action_role(action) == MEM_CONSOLE_ACTION_ROLE_UNKNOWN) {
        mem_console_app_set_statusf(state, "Unsupported action ignored.");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_REFRESH) {
        mem_console_app_refresh_and_report(db, state, "Refresh failed");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_REFRESH_DETAIL) {
        result = refresh_selected_detail_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Detail refresh failed", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        mem_console_app_set_statusf(state, "Inspecting memory %lld.", (long long)state->selected_item_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CREATE_FROM_SEARCH) {
        int64_t created_id = 0;
        result = create_item_from_search(db, state, &created_id);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Create failed", result);
            return;
        }

        mem_console_selection_set(state, created_id);
        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_post_mutation_refresh_error_status(state, "create", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        mem_console_app_note_db_write(runtime);

        mem_console_app_set_statusf(state, "Created memory %lld from search text.", (long long)created_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_TITLE_EDIT) {
        begin_title_edit_mode(state);
        mem_console_app_set_statusf(state, "Title edit mode enabled.");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CANCEL_TITLE_EDIT) {
        cancel_title_edit_mode(state);
        mem_console_app_set_statusf(state, "Title edit cancelled.");
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
            mem_console_app_set_post_mutation_refresh_error_status(state, "title save", result);
            return;
        }
        cancel_title_edit_mode(state);
        sync_edit_buffers_from_selection(state);
        mem_console_app_note_db_write(runtime);

        mem_console_app_set_statusf(state, "Saved title for memory %lld.", (long long)edited_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_BODY_EDIT) {
        begin_body_edit_mode(state);
        mem_console_app_set_statusf(state, "Body edit mode enabled.");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CANCEL_BODY_EDIT) {
        cancel_body_edit_mode(state);
        mem_console_app_set_statusf(state, "Body edit cancelled.");
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
            mem_console_app_set_post_mutation_refresh_error_status(state, "body save", result);
            return;
        }
        cancel_body_edit_mode(state);
        sync_edit_buffers_from_selection(state);
        mem_console_app_note_db_write(runtime);

        mem_console_app_set_statusf(state, "Saved body for memory %lld.", (long long)edited_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_TOGGLE_PINNED) {
        int next_value = state->selected_pinned ? 0 : 1;
        int64_t toggled_id = state->selected_item_id;

        result = set_selected_item_flag(db, state, MEM_CONSOLE_ITEM_FLAG_PINNED, next_value);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Pinned toggle failed", result);
            return;
        }

        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_post_mutation_refresh_error_status(state, "pinned toggle", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        mem_console_app_note_db_write(runtime);

        mem_console_app_set_statusf(state,
                                    "Pinned %s for memory %lld.",
                                    next_value ? "enabled" : "disabled",
                                    (long long)toggled_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_TOGGLE_CANONICAL) {
        int next_value = state->selected_canonical ? 0 : 1;
        int64_t toggled_id = state->selected_item_id;

        result = set_selected_item_flag(db, state, MEM_CONSOLE_ITEM_FLAG_CANONICAL, next_value);
        if (result.code != CORE_OK) {
            mem_console_app_set_action_error_status(state, "Canonical toggle failed", result);
            return;
        }

        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_post_mutation_refresh_error_status(state, "canonical toggle", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        mem_console_app_note_db_write(runtime);

        mem_console_app_set_statusf(state,
                                    "Canonical %s for memory %lld.",
                                    next_value ? "enabled" : "disabled",
                                    (long long)toggled_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_TOGGLE_GRAPH_MODE) {
        state->graph_mode_enabled = 1;
        {
            int current_view_mode = mem_console_graph_view_mode_get(state);
            int next_view_mode = MEM_CONSOLE_GRAPH_VIEW_FOCUS;

            if (current_view_mode == MEM_CONSOLE_GRAPH_VIEW_FOCUS) {
                next_view_mode = MEM_CONSOLE_GRAPH_VIEW_PODS;
            } else if (current_view_mode == MEM_CONSOLE_GRAPH_VIEW_PODS) {
                next_view_mode = MEM_CONSOLE_GRAPH_VIEW_WEB;
            }
            (void)mem_console_graph_view_mode_set(state, next_view_mode);
        }

        result = load_graph_neighborhood(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_graph_action_error_status(state, "load", result);
            return;
        }

        mem_console_app_set_statusf(state,
                                    "Graph mode %s (%d nodes, %d edges, kind=%s, limit=%d, hops=%d).",
                                    mem_console_graph_view_mode_status_text(state),
                                    state->graph_node_count,
                                    state->graph_edge_count,
                                    state->graph_kind_filter[0] ? state->graph_kind_filter : "all",
                                    state->graph_query_edge_limit,
                                    state->graph_query_hops);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_REFRESH_GRAPH) {
        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_graph_action_error_status(state, "refresh", result);
            return;
        }
        sync_edit_buffers_from_selection(state);

        mem_console_app_set_statusf(state,
                                    "Graph refreshed (%d nodes, %d edges, kind=%s, limit=%d, hops=%d).",
                                    state->graph_node_count,
                                    state->graph_edge_count,
                                    state->graph_kind_filter[0] ? state->graph_kind_filter : "all",
                                    state->graph_query_edge_limit,
                                    state->graph_query_hops);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CENTER_GRAPH) {
        result = mem_console_ui_graph_center_layout_view(state);
        if (result.code != CORE_OK) {
            mem_console_app_set_graph_action_error_status(state, "center", result);
            return;
        }

        mem_console_app_set_statusf(state, "Graph viewport centered.");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CENTER_SELECTED) {
        result = mem_console_ui_graph_center_selected_view(state);
        if (result.code != CORE_OK) {
            mem_console_app_set_graph_action_error_status(state, "center selected", result);
            return;
        }

        mem_console_app_set_statusf(state, "Centered selected memory %lld.", (long long)state->selected_item_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_OPEN_REFERENCE_PATH) {
        if (!state->detail_reference_path_available || !state->detail_reference_path[0]) {
            mem_console_app_set_statusf(state, "No markdown reference path available.");
            return;
        }
        if (!mem_console_path_has_md_suffix(state->detail_reference_path) ||
            !mem_console_path_is_regular_file(state->detail_reference_path)) {
            mem_console_app_set_statusf(state,
                                        "Reference path is unavailable: %s",
                                        state->detail_reference_path);
            return;
        }
        if (!mem_console_open_path_with_system_default(state->detail_reference_path)) {
            mem_console_app_set_statusf(state, "Failed to open reference path.");
            return;
        }

        mem_console_app_set_statusf(state, "Opened reference path in default app.");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_ADD_RELATIONSHIP) {
        int64_t link_id = 0;

        result = create_selected_relationship_to_target(db, state, &link_id);
        if (result.code != CORE_OK) {
            mem_console_app_set_relationship_action_error_status(state, "add", result);
            return;
        }

        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_post_mutation_refresh_error_status(state, "relationship add", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        state->relationship_target_text[0] = '\0';
        state->relationship_target_cursor = 0;
        state->relationship_action_link_id = 0;
        mem_console_app_note_db_write(runtime);

        mem_console_app_set_statusf(state, "Added related link %lld.", (long long)link_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CYCLE_RELATIONSHIP_KIND) {
        int64_t link_id = 0;

        result = cycle_selected_relationship_kind(db, state, &link_id);
        if (result.code != CORE_OK) {
            mem_console_app_set_relationship_action_error_status(state, "kind change", result);
            return;
        }

        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_post_mutation_refresh_error_status(state, "relationship kind change", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        state->relationship_action_link_id = 0;
        mem_console_app_note_db_write(runtime);

        mem_console_app_set_statusf(state, "Changed relationship kind for link %lld.", (long long)link_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_REMOVE_RELATIONSHIP) {
        int64_t link_id = 0;

        result = remove_selected_relationship(db, state, &link_id);
        if (result.code != CORE_OK) {
            mem_console_app_set_relationship_action_error_status(state, "remove", result);
            return;
        }

        result = refresh_state_from_db(db, state);
        if (result.code != CORE_OK) {
            mem_console_app_set_post_mutation_refresh_error_status(state, "relationship remove", result);
            return;
        }
        sync_edit_buffers_from_selection(state);
        state->relationship_action_link_id = 0;
        mem_console_app_note_db_write(runtime);

        mem_console_app_set_statusf(state, "Removed relationship link %lld.", (long long)link_id);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_TOGGLE_BROWSE_PINNED) {
        state->browse_pinned_only = state->browse_pinned_only ? 0 : 1;
        mem_console_browse_reset_window(state);
        mem_console_app_refresh_and_report(db, state, "Browse filter failed");
        mem_console_app_set_statusf(state,
                                    "Browse pinned filter %s.",
                                    state->browse_pinned_only ? "enabled" : "disabled");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_TOGGLE_BROWSE_CANONICAL) {
        state->browse_canonical_only = state->browse_canonical_only ? 0 : 1;
        mem_console_browse_reset_window(state);
        mem_console_app_refresh_and_report(db, state, "Browse filter failed");
        mem_console_app_set_statusf(state,
                                    "Browse canonical filter %s.",
                                    state->browse_canonical_only ? "enabled" : "disabled");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CYCLE_BROWSE_KIND) {
        const char *kind = "";

        (void)mem_console_browse_kind_cycle(state);
        mem_console_browse_reset_window(state);
        mem_console_app_refresh_and_report(db, state, "Browse filter failed");
        kind = mem_console_browse_kind_for_index(state->browse_kind_index);
        mem_console_app_set_statusf(state, "Browse kind filter: %s.", kind[0] ? kind : "all");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_DB_PICKER) {
        begin_db_picker_mode(state, 0);
        mem_console_app_set_statusf(state, "Select .sqlite from input root list or enter exact DB path.");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_DB_CREATE) {
        begin_db_picker_mode(state, 1);
        mem_console_app_set_statusf(state, "Enter DB name/path to create (name -> input root, path -> explicit).");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CANCEL_DB_PICKER) {
        int canceling_input_root = state->db_modal_input_root_mode;
        cancel_db_picker_mode(state);
        mem_console_app_set_statusf(state,
                                    canceling_input_root ? "Input root change cancelled." : "Database change cancelled.");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER) {
        char next_db_path[1024];
        int create_mode = state->db_modal_create_mode ? 1 : 0;

        if (!mem_console_db_picker_build_path(state, next_db_path, sizeof(next_db_path))) {
            mem_console_app_set_statusf(state, "Enter a valid DB path first.");
            return;
        }
        if (state->db_modal_input_root_mode) {
            CoreResult save_result = core_result_ok();
            if (!mem_console_ensure_directory(next_db_path)) {
                mem_console_app_set_statusf(state, "Input root must be a valid directory.");
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
                mem_console_app_set_path_result_status(state,
                                                       "Input root app prefs save",
                                                       app_prefs_path,
                                                       save_result);
            } else {
                mem_console_app_set_statusf(state, "Input root set to %s.", state->input_root);
            }
            return;
        }
        cancel_db_picker_mode(state);
        (void)snprintf(state->pending_db_path, sizeof(state->pending_db_path), "%s", next_db_path);
        mem_console_app_set_statusf(state,
                                    create_mode ? "Creating/switching DB to %s..." : "Switching DB to %s...",
                                    state->pending_db_path);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_INPUT_ROOT_PICKER) {
        begin_input_root_picker_mode(state);
        mem_console_app_set_statusf(state, "Enter input root path and press Enter.");
        return;
    }

    if (action == MEM_CONSOLE_ACTION_PICK_INPUT_ROOT_FOLDER) {
        char picked_root[1024];
        CoreResult save_result = core_result_ok();

        if (!mem_console_pick_folder_macos(picked_root, sizeof(picked_root))) {
            mem_console_app_set_statusf(state, "Folder dialog canceled/unavailable.");
            return;
        }
        if (!mem_console_ensure_directory(picked_root)) {
            mem_console_app_set_statusf(state, "Failed to use selected input root.");
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
            mem_console_app_set_path_result_status(state,
                                                   "Input root app prefs save",
                                                   app_prefs_path,
                                                   save_result);
        } else {
            mem_console_app_set_statusf(state, "Input root updated from folder dialog.");
        }
        return;
    }
}
