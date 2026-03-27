#include "mem_console_app_internal.h"

#include <SDL2/SDL.h>
#include <stdio.h>

#include "mem_console_ui_graph.h"

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

    if (action == MEM_CONSOLE_ACTION_BEGIN_DB_PICKER) {
        begin_db_picker_mode(state, 0);
        (void)snprintf(state->status_line, sizeof(state->status_line), "Enter a DB path to load or switch.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_BEGIN_DB_CREATE) {
        begin_db_picker_mode(state, 1);
        (void)snprintf(state->status_line, sizeof(state->status_line), "Enter a DB path to create or switch.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CANCEL_DB_PICKER) {
        cancel_db_picker_mode(state);
        (void)snprintf(state->status_line, sizeof(state->status_line), "Database change cancelled.");
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }

    if (action == MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER) {
        char next_db_path[1024];

        if (!mem_console_db_picker_build_path(state, next_db_path, sizeof(next_db_path))) {
            (void)snprintf(state->status_line, sizeof(state->status_line), "Enter a valid DB path first.");
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            return;
        }
        cancel_db_picker_mode(state);
        (void)snprintf(state->pending_db_path, sizeof(state->pending_db_path), "%s", next_db_path);
        (void)snprintf(state->status_line, sizeof(state->status_line), "Switching DB to %s...", state->pending_db_path);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return;
    }
}
