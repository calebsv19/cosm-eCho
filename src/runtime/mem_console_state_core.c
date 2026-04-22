#include "mem_console_state.h"

#include <stdio.h>
#include <string.h>

static const CoreThemePresetId k_mem_console_theme_cycle_order[] = {
    CORE_THEME_PRESET_DAW_DEFAULT,
    CORE_THEME_PRESET_MAP_FORGE_DEFAULT,
    CORE_THEME_PRESET_DARK_DEFAULT,
    CORE_THEME_PRESET_LIGHT_DEFAULT,
    CORE_THEME_PRESET_IDE_GRAY,
    CORE_THEME_PRESET_GREYSCALE
};

static const CoreFontPresetId k_mem_console_font_cycle_order[] = {
    CORE_FONT_PRESET_DAW_DEFAULT,
    CORE_FONT_PRESET_IDE
};

enum {
    MEM_CONSOLE_TEXT_ZOOM_STEP_MIN = -4,
    MEM_CONSOLE_TEXT_ZOOM_STEP_MAX = 5
};

static int clamp_text_zoom_step(int step) {
    if (step < MEM_CONSOLE_TEXT_ZOOM_STEP_MIN) {
        return MEM_CONSOLE_TEXT_ZOOM_STEP_MIN;
    }
    if (step > MEM_CONSOLE_TEXT_ZOOM_STEP_MAX) {
        return MEM_CONSOLE_TEXT_ZOOM_STEP_MAX;
    }
    return step;
}

void set_default_detail(MemConsoleState *state) {
    if (!state) {
        return;
    }

    state->selected_item_id = 0;
    state->graph_center_item_id = 0;
    state->selected_created_ns = 0;
    state->selected_pinned = 0;
    state->selected_canonical = 0;
    (void)snprintf(state->selected_title,
                   sizeof(state->selected_title),
                   "No Matching Memory");
    (void)snprintf(state->selected_body,
                   sizeof(state->selected_body),
                   "Type to filter, or use mem_cli add to create records first.");
    state->detail_reference_scan_item_id = 0;
    state->detail_reference_path_available = 0;
    state->detail_reference_path[0] = '\0';
}

void copy_core_str(CoreStr value, char *out_text, size_t out_cap) {
    size_t copy_len;

    if (!out_text || out_cap == 0u) {
        return;
    }

    out_text[0] = '\0';
    if (!value.data || value.len == 0u) {
        return;
    }

    copy_len = value.len;
    if (copy_len >= out_cap) {
        copy_len = out_cap - 1u;
    }

    memcpy(out_text, value.data, copy_len);
    out_text[copy_len] = '\0';
}

static void sync_theme_name(MemConsoleState *state) {
    const char *name;

    if (!state) {
        return;
    }

    name = core_theme_preset_name(state->theme_preset_id);
    if (!name || !name[0]) {
        name = "unknown";
    }
    (void)snprintf(state->theme_name, sizeof(state->theme_name), "%s", name);
}

int cycle_theme_preset(MemConsoleState *state, int direction) {
    size_t i;
    size_t count;

    if (!state) {
        return 0;
    }

    count = sizeof(k_mem_console_theme_cycle_order) / sizeof(k_mem_console_theme_cycle_order[0]);
    if (count == 0u) {
        return 0;
    }

    for (i = 0; i < count; ++i) {
        if (k_mem_console_theme_cycle_order[i] == state->theme_preset_id) {
            if (direction >= 0) {
                state->theme_preset_id = k_mem_console_theme_cycle_order[(i + 1u) % count];
            } else {
                state->theme_preset_id = k_mem_console_theme_cycle_order[(i + count - 1u) % count];
            }
            sync_theme_name(state);
            return 1;
        }
    }

    state->theme_preset_id = k_mem_console_theme_cycle_order[0];
    sync_theme_name(state);
    return 1;
}

int state_set_theme_preset(MemConsoleState *state, CoreThemePresetId preset_id) {
    CoreThemePreset preset;
    CoreResult result;

    if (!state) {
        return 0;
    }

    result = core_theme_get_preset(preset_id, &preset);
    if (result.code != CORE_OK) {
        return 0;
    }

    state->theme_preset_id = preset_id;
    sync_theme_name(state);
    return 1;
}

static void sync_font_name(MemConsoleState *state) {
    const char *name;

    if (!state) {
        return;
    }

    name = core_font_preset_name(state->font_preset_id);
    if (!name || !name[0]) {
        name = "unknown";
    }
    (void)snprintf(state->font_name, sizeof(state->font_name), "%s", name);
}

int cycle_font_preset(MemConsoleState *state, int direction) {
    size_t i;
    size_t count;

    if (!state) {
        return 0;
    }

    count = sizeof(k_mem_console_font_cycle_order) / sizeof(k_mem_console_font_cycle_order[0]);
    if (count == 0u) {
        return 0;
    }

    for (i = 0; i < count; ++i) {
        if (k_mem_console_font_cycle_order[i] == state->font_preset_id) {
            if (direction >= 0) {
                state->font_preset_id = k_mem_console_font_cycle_order[(i + 1u) % count];
            } else {
                state->font_preset_id = k_mem_console_font_cycle_order[(i + count - 1u) % count];
            }
            sync_font_name(state);
            return 1;
        }
    }

    state->font_preset_id = k_mem_console_font_cycle_order[0];
    sync_font_name(state);
    return 1;
}

int state_set_font_preset(MemConsoleState *state, CoreFontPresetId preset_id) {
    CoreFontPreset preset;
    CoreResult result;

    if (!state) {
        return 0;
    }

    result = core_font_get_preset(preset_id, &preset);
    if (result.code != CORE_OK) {
        return 0;
    }

    state->font_preset_id = preset_id;
    sync_font_name(state);
    return 1;
}

int state_set_text_zoom_step(MemConsoleState *state, int step) {
    if (!state) {
        return 0;
    }
    state->text_zoom_step = clamp_text_zoom_step(step);
    return 1;
}

int state_adjust_text_zoom_step(MemConsoleState *state, int delta) {
    if (!state) {
        return 0;
    }
    return state_set_text_zoom_step(state, state->text_zoom_step + delta);
}

int state_reset_text_zoom_step(MemConsoleState *state) {
    return state_set_text_zoom_step(state, 0);
}

void mem_console_state_set_path_contract(MemConsoleState *state,
                                         const char *input_root,
                                         const char *output_root,
                                         const char *active_db_path) {
    char fallback_input_root[1024];
    char fallback_output_root[1024];
    char fallback_active_db_path[1024];

    if (!state) {
        return;
    }

    fallback_input_root[0] = '\0';
    fallback_output_root[0] = '\0';
    fallback_active_db_path[0] = '\0';
    if (!mem_console_path_contract_normalize(input_root,
                                             output_root,
                                             active_db_path,
                                             fallback_input_root,
                                             sizeof(fallback_input_root),
                                             fallback_output_root,
                                             sizeof(fallback_output_root),
                                             fallback_active_db_path,
                                             sizeof(fallback_active_db_path))) {
        fallback_input_root[0] = '\0';
        fallback_output_root[0] = '\0';
        fallback_active_db_path[0] = '\0';
        if (active_db_path && active_db_path[0]) {
            (void)snprintf(fallback_active_db_path, sizeof(fallback_active_db_path), "%s", active_db_path);
            (void)mem_console_path_parent(active_db_path, fallback_input_root, sizeof(fallback_input_root));
        }
    }

    if (fallback_active_db_path[0]) {
        (void)snprintf(state->active_db_path, sizeof(state->active_db_path), "%s", fallback_active_db_path);
    } else {
        state->active_db_path[0] = '\0';
    }
    if (fallback_input_root[0]) {
        (void)snprintf(state->input_root, sizeof(state->input_root), "%s", fallback_input_root);
    } else {
        state->input_root[0] = '\0';
    }
    if (fallback_output_root[0]) {
        (void)snprintf(state->output_root, sizeof(state->output_root), "%s", fallback_output_root);
    } else {
        state->output_root[0] = '\0';
    }

    if (state->active_db_path[0]) {
        (void)snprintf(state->db_path_storage, sizeof(state->db_path_storage), "%s", state->active_db_path);
    } else {
        state->db_path_storage[0] = '\0';
    }
    state->db_path = state->db_path_storage;
}

void seed_state(MemConsoleState *state, const char *db_path) {
    char initial_input_root[1024];
    char initial_output_root[1024];

    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));
    initial_input_root[0] = '\0';
    initial_output_root[0] = '\0';
    if (db_path && db_path[0]) {
        (void)mem_console_path_parent(db_path, initial_input_root, sizeof(initial_input_root));
    }
    (void)mem_console_resolve_app_data_dir(initial_output_root, sizeof(initial_output_root));
    mem_console_state_set_path_contract(state, initial_input_root, initial_output_root, db_path);
    state->theme_preset_id = CORE_THEME_PRESET_DAW_DEFAULT;
    state->font_preset_id = CORE_FONT_PRESET_IDE;
    state->text_zoom_step = 0;
    state->search_text[0] = '\0';
    state->graph_mode_enabled = 1;
    state->list_query_offset = 0;
    state->visible_start_index = 0;
    state->project_filter_scroll = 0.0f;
    state->detail_connection_scroll = 0.0f;
    state->detail_body_scroll = 0.0f;
    state->title_edit_mode = 0;
    state->body_edit_mode = 0;
    state->db_modal_open = 0;
    state->db_modal_create_mode = 0;
    state->db_modal_input_root_mode = 0;
    state->input_target = MEM_CONSOLE_INPUT_SEARCH;
    state->search_cursor = 0;
    state->title_edit_cursor = 0;
    state->body_edit_cursor = 0;
    state->db_modal_cursor = 0;
    state->db_modal_selection_anchor = 0;
    state->db_modal_selection_start = 0;
    state->db_modal_selection_end = 0;
    state->db_modal_drag_select_active = 0;
    state->graph_edge_limit_cursor = 0;
    state->search_refresh_pending = 0;
    state->search_last_input_ms = 0u;
    state->project_filter_option_count = 0;
    state->selected_project_count = 0;
    state->graph_kind_filter[0] = '\0';
    state->graph_kind_filter_mask = mem_console_graph_kind_filter_all_mask();
    state->graph_kind_filter_all_override = 1;
    state->graph_node_kind_filter_mask = mem_console_graph_node_kind_filter_all_mask();
    state->graph_node_kind_filter_all_override = 1;
    mem_console_graph_edge_limit_set(state, MEM_CONSOLE_GRAPH_EDGE_LIMIT_DEFAULT);
    state->graph_query_hops = MEM_CONSOLE_GRAPH_HOPS_MIN;
    state->graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_DAG;
    state->graph_sort_mode = MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST;
    state->graph_scope_full_mode_enabled = 0;
    state->graph_anchor_funnel_enabled = 0;
    state->graph_edge_labels_enabled = 1;
    state->graph_hidden_anchor_count = 0;
    state->graph_center_item_id = 0;
    sync_theme_name(state);
    sync_font_name(state);
    (void)snprintf(state->status_line,
                   sizeof(state->status_line),
                   "Type to search. Click a row to inspect it.");
    (void)snprintf(state->runtime_summary_line,
                   sizeof(state->runtime_summary_line),
                   "Async s0 a0 d0 e0 c0 | if=0 p=0");
    (void)snprintf(state->kernel_summary_line,
                   sizeof(state->kernel_summary_line),
                   "Kernel off");
    (void)snprintf(state->redraw_summary_line,
                   sizeof(state->redraw_summary_line),
                   "Render idle");
    (void)snprintf(state->project_filter_summary_line,
                   sizeof(state->project_filter_summary_line),
                   "Projects: all");
    set_default_detail(state);
    state->title_edit_text[0] = '\0';
    state->body_edit_text[0] = '\0';
    state->db_modal_text[0] = '\0';
    state->db_modal_resolved_path[0] = '\0';
    state->db_modal_visible_text[0] = '\0';
    state->db_modal_resolved_line[0] = '\0';
    state->db_modal_active_line[0] = '\0';
    state->pending_db_path[0] = '\0';
    state->redraw_pending_reasons = MEM_CONSOLE_REDRAW_REASON_BACKGROUND;
    state->redraw_last_reasons = MEM_CONSOLE_REDRAW_REASON_NONE;
    state->redraw_frame_count = 0u;
    state->redraw_last_frame_ms = 0u;
    state->pane_left_ratio = 0.0f;
    state->pane_right_split_ratio = 0.0f;
    state->pane_detail_split_ratio = 0.0f;
    state->pane_detail_top_split_ratio = 0.0f;
    state->left_panel_top_ratio = 0.58f;
    state->pane_drag_active = 0;
    state->pane_drag_splitter_id = 0;
    state->pane_drag_anchor_x = 0.0f;
    state->pane_drag_anchor_y = 0.0f;
    state->pane_drag_start_left_ratio = 0.0f;
    state->pane_drag_start_right_ratio = 0.0f;
    state->pane_drag_start_detail_ratio = 0.0f;
    state->pane_drag_start_detail_top_ratio = 0.0f;
    state->left_panel_drag_active = 0;
    state->left_panel_drag_anchor_y = 0.0f;
    state->left_panel_drag_start_ratio = 0.0f;
    state->pane_left_collapsed = 0;
    state->pane_right_detail_collapsed = 0;
    state->pane_prefs_dirty = 0;
    kit_graph_struct_viewport_default(&state->graph_viewport);
}
