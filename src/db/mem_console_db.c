#include "mem_console_db_internal.h"

#include <stdio.h>

CoreResult refresh_selected_detail_from_db(CoreMemDb *db, MemConsoleState *state) {
    CoreResult result;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }

    result = read_selected_detail(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = read_selected_relationships(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    return core_result_ok();
}

CoreResult refresh_state_from_db(CoreMemDb *db, MemConsoleState *state) {
    CoreResult result;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }

    result = read_schema_version(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = read_active_count(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = read_project_filter_options(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = read_matching_count(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    clamp_list_query_offset(state);

    result = read_visible_items(db, state);
    if (result.code != CORE_OK) {
        return result;
    }
    if (state->matching_count > 0 && state->visible_count == 0 && state->list_query_offset > 0) {
        state->list_query_offset = 0;
        state->visible_start_index = 0;
        result = read_visible_items(db, state);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    if (state->matching_count == 0) {
        mem_console_selection_clear(state);
        state->list_query_offset = 0;
        state->visible_start_index = 0;
        state->list_scroll = 0.0f;
        set_default_detail(state);
    } else if (state->selected_item_id == 0 && state->visible_count > 0) {
        mem_console_selection_set(state, state->visible_items[0].id);
    }

    if (state->graph_center_item_id == 0 && state->selected_item_id != 0) {
        mem_console_graph_center_set(state, state->selected_item_id);
    }

    result = read_selected_detail(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = read_selected_relationships(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = load_graph_neighborhood(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    if (state->selected_project_count > 0) {
        (void)snprintf(state->project_filter_summary_line,
                       sizeof(state->project_filter_summary_line),
                       "Projects: %d selected",
                       state->selected_project_count);
    } else if (state->project_filter_option_count > 0) {
        (void)snprintf(state->project_filter_summary_line,
                       sizeof(state->project_filter_summary_line),
                       "Projects: all (%d options)",
                       state->project_filter_option_count);
    } else {
        (void)snprintf(state->project_filter_summary_line,
                       sizeof(state->project_filter_summary_line),
                       "Projects: none");
    }

    if (state->search_text[0] != '\0') {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Active %lld | Match %lld | Proj %d | %s",
                       (long long)state->active_count,
                       (long long)state->matching_count,
                       state->selected_project_count,
                       state->search_text);
    } else {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Active %lld | Match %lld | Proj %d",
                       (long long)state->active_count,
                       (long long)state->matching_count,
                       state->selected_project_count);
    }

    return core_result_ok();
}
