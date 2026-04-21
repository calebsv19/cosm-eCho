#include "mem_console_prefs.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core_base.h"
#include "core_pack.h"
#include "runtime/mem_console_prefs_app_io_internal.h"

typedef struct MemConsoleUiPrefsV1 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
} MemConsoleUiPrefsV1;

typedef struct MemConsoleUiPrefsV2 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
} MemConsoleUiPrefsV2;

typedef struct MemConsoleUiPrefsV3 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
} MemConsoleUiPrefsV3;

typedef struct MemConsoleUiPrefsV4 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    float pane_detail_split_ratio;
    float pane_detail_top_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
} MemConsoleUiPrefsV4;

typedef struct MemConsoleUiPrefsV5 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    float pane_detail_split_ratio;
    float pane_detail_top_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
    int32_t graph_kind_filter_all_override;
} MemConsoleUiPrefsV5;

typedef struct MemConsoleUiPrefsV6 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    float pane_detail_split_ratio;
    float pane_detail_top_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
    int32_t graph_kind_filter_all_override;
    int32_t graph_layout_mode;
    int32_t graph_sort_mode;
    uint32_t graph_node_kind_filter_mask;
    int32_t graph_node_kind_filter_all_override;
} MemConsoleUiPrefsV6;

typedef struct MemConsoleUiPrefsV7 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    float pane_detail_split_ratio;
    float pane_detail_top_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
    int32_t graph_kind_filter_all_override;
    int32_t graph_layout_mode;
    int32_t graph_sort_mode;
    uint32_t graph_node_kind_filter_mask;
    int32_t graph_node_kind_filter_all_override;
    int32_t graph_scope_full_mode_enabled;
} MemConsoleUiPrefsV7;

typedef struct MemConsoleUiPrefsV8 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    float pane_detail_split_ratio;
    float pane_detail_top_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
    int32_t graph_kind_filter_all_override;
    int32_t graph_layout_mode;
    int32_t graph_sort_mode;
    uint32_t graph_node_kind_filter_mask;
    int32_t graph_node_kind_filter_all_override;
    int32_t graph_scope_full_mode_enabled;
    uint32_t graph_kind_filter_mask;
    int32_t graph_anchor_funnel_enabled;
    int32_t graph_edge_labels_enabled;
} MemConsoleUiPrefsV8;

typedef struct MemConsoleUiPrefsV9 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    float pane_detail_split_ratio;
    float pane_detail_top_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
    int32_t graph_kind_filter_all_override;
    int32_t graph_layout_mode;
    int32_t graph_sort_mode;
    uint32_t graph_node_kind_filter_mask;
    int32_t graph_node_kind_filter_all_override;
    int32_t graph_scope_full_mode_enabled;
    uint32_t graph_kind_filter_mask;
    int32_t graph_anchor_funnel_enabled;
    int32_t graph_edge_labels_enabled;
    int32_t graph_hidden_anchor_count;
    int64_t graph_hidden_anchor_item_ids[MEM_CONSOLE_GRAPH_NODE_LIMIT];
} MemConsoleUiPrefsV9;

typedef struct MemConsoleUiPrefsV10 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    float pane_detail_split_ratio;
    float pane_detail_top_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
    int32_t graph_kind_filter_all_override;
    int32_t graph_layout_mode;
    int32_t graph_sort_mode;
    uint32_t graph_node_kind_filter_mask;
    int32_t graph_node_kind_filter_all_override;
    int32_t graph_scope_full_mode_enabled;
    uint32_t graph_kind_filter_mask;
    int32_t graph_anchor_funnel_enabled;
    int32_t graph_edge_labels_enabled;
    int32_t graph_hidden_anchor_count;
    int64_t graph_hidden_anchor_item_ids[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float left_panel_top_ratio;
} MemConsoleUiPrefsV10;

typedef struct MemConsoleUiPrefsV11 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    float pane_detail_split_ratio;
    float pane_detail_top_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
    int32_t graph_kind_filter_all_override;
    int32_t graph_layout_mode;
    int32_t graph_sort_mode;
    uint32_t graph_node_kind_filter_mask;
    int32_t graph_node_kind_filter_all_override;
    int32_t graph_scope_full_mode_enabled;
    uint32_t graph_kind_filter_mask;
    int32_t graph_anchor_funnel_enabled;
    int32_t graph_edge_labels_enabled;
    int32_t graph_hidden_anchor_count;
    int64_t graph_hidden_anchor_item_ids[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float left_panel_top_ratio;
    int32_t text_zoom_step;
} MemConsoleUiPrefsV11;

enum {
    MEM_CONSOLE_UI_PREFS_VERSION = 11u
};

static float prefs_ratio_or_default(float ratio) {
    if (!isfinite(ratio)) {
        return 0.0f;
    }
    if (ratio <= 0.01f || ratio >= 0.99f) {
        return 0.0f;
    }
    return ratio;
}

static float prefs_viewport_component_or_default(float value, float fallback) {
    if (!isfinite(value)) {
        return fallback;
    }
    return value;
}

static float prefs_viewport_zoom_or_default(float zoom) {
    float next_zoom = zoom;

    if (!isfinite(next_zoom) || next_zoom <= 0.0f) {
        return 1.0f;
    }
    if (next_zoom < 0.1f) {
        next_zoom = 0.1f;
    }
    if (next_zoom > 8.0f) {
        next_zoom = 8.0f;
    }
    return next_zoom;
}

static void prefs_copy_project_filters_from_state(MemConsoleUiPrefsV5 *prefs,
                                                  const MemConsoleState *state) {
    int i;
    int write_index = 0;

    if (!prefs || !state) {
        return;
    }

    for (i = 0; i < state->selected_project_count && i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        if (state->selected_project_keys[i][0] == '\0') {
            continue;
        }
        (void)snprintf(prefs->selected_project_keys[write_index],
                       sizeof(prefs->selected_project_keys[write_index]),
                       "%s",
                       state->selected_project_keys[i]);
        write_index += 1;
        if (write_index >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
            break;
        }
    }

    prefs->selected_project_count = write_index;
}

static void prefs_copy_project_filters_to_state(const MemConsoleUiPrefsV5 *prefs,
                                                MemConsoleState *state) {
    int i;
    int write_index = 0;
    int load_count;

    if (!prefs || !state) {
        return;
    }

    load_count = prefs->selected_project_count;
    if (load_count < 0) {
        load_count = 0;
    }
    if (load_count > MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
        load_count = MEM_CONSOLE_SCOPE_FILTER_LIMIT;
    }

    for (i = 0; i < load_count; ++i) {
        if (prefs->selected_project_keys[i][0] == '\0') {
            continue;
        }
        (void)snprintf(state->selected_project_keys[write_index],
                       sizeof(state->selected_project_keys[write_index]),
                       "%s",
                       prefs->selected_project_keys[i]);
        write_index += 1;
        if (write_index >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
            break;
        }
    }

    for (i = write_index; i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        state->selected_project_keys[i][0] = '\0';
    }
    state->selected_project_count = write_index;
}

static void prefs_copy_hidden_anchors_from_state(const MemConsoleState *state,
                                                 MemConsoleUiPrefsV9 *prefs) {
    int i;
    int count = 0;

    if (!state || !prefs) {
        return;
    }

    count = state->graph_hidden_anchor_count;
    if (count < 0) {
        count = 0;
    }
    if (count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }

    prefs->graph_hidden_anchor_count = count;
    for (i = 0; i < count; ++i) {
        prefs->graph_hidden_anchor_item_ids[i] = state->graph_hidden_anchor_item_ids[i];
    }
    for (i = count; i < MEM_CONSOLE_GRAPH_NODE_LIMIT; ++i) {
        prefs->graph_hidden_anchor_item_ids[i] = 0;
    }
}

static void prefs_copy_hidden_anchors_to_state(const MemConsoleUiPrefsV9 *prefs,
                                               MemConsoleState *state) {
    int i;
    int write_index = 0;
    int load_count = 0;

    if (!prefs || !state) {
        return;
    }

    load_count = prefs->graph_hidden_anchor_count;
    if (load_count < 0) {
        load_count = 0;
    }
    if (load_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        load_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }

    for (i = 0; i < load_count; ++i) {
        int64_t item_id = prefs->graph_hidden_anchor_item_ids[i];
        int j;
        int duplicate = 0;
        if (item_id <= 0) {
            continue;
        }
        for (j = 0; j < write_index; ++j) {
            if (state->graph_hidden_anchor_item_ids[j] == item_id) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate) {
            continue;
        }
        state->graph_hidden_anchor_item_ids[write_index] = item_id;
        write_index += 1;
        if (write_index >= MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            break;
        }
    }
    for (i = write_index; i < MEM_CONSOLE_GRAPH_NODE_LIMIT; ++i) {
        state->graph_hidden_anchor_item_ids[i] = 0;
    }
    state->graph_hidden_anchor_count = write_index;
}

static void prefs_build_v5_from_state(const MemConsoleState *state, MemConsoleUiPrefsV5 *out_prefs) {
    if (!state || !out_prefs) {
        return;
    }

    memset(out_prefs, 0, sizeof(*out_prefs));
    out_prefs->version = MEM_CONSOLE_UI_PREFS_VERSION;
    out_prefs->theme_preset_id = (int32_t)state->theme_preset_id;
    out_prefs->font_preset_id = (int32_t)state->font_preset_id;
    out_prefs->pane_left_ratio = prefs_ratio_or_default(state->pane_left_ratio);
    out_prefs->pane_right_split_ratio = prefs_ratio_or_default(state->pane_right_split_ratio);
    out_prefs->pane_detail_split_ratio = prefs_ratio_or_default(state->pane_detail_split_ratio);
    out_prefs->pane_detail_top_split_ratio = prefs_ratio_or_default(state->pane_detail_top_split_ratio);
    out_prefs->pane_left_collapsed = state->pane_left_collapsed ? 1 : 0;
    out_prefs->pane_right_detail_collapsed = state->pane_right_detail_collapsed ? 1 : 0;
    out_prefs->selected_item_id = state->selected_item_id;
    out_prefs->list_query_offset = state->list_query_offset > 0 ? state->list_query_offset : 0;
    prefs_copy_project_filters_from_state(out_prefs, state);
    (void)snprintf(out_prefs->graph_kind_filter,
                   sizeof(out_prefs->graph_kind_filter),
                   "%s",
                   state->graph_kind_filter);
    out_prefs->graph_edge_limit = mem_console_graph_edge_limit_clamp(state->graph_query_edge_limit);
    out_prefs->graph_hops = mem_console_graph_hops_clamp(state->graph_query_hops);
    out_prefs->graph_mode_enabled = state->graph_mode_enabled ? 1 : 0;
    out_prefs->graph_pan_x = prefs_viewport_component_or_default(state->graph_viewport.pan_x, 0.0f);
    out_prefs->graph_pan_y = prefs_viewport_component_or_default(state->graph_viewport.pan_y, 0.0f);
    out_prefs->graph_zoom = prefs_viewport_zoom_or_default(state->graph_viewport.zoom);
    out_prefs->graph_kind_filter_all_override = state->graph_kind_filter_all_override ? 1 : 0;
    (void)snprintf(out_prefs->search_text,
                   sizeof(out_prefs->search_text),
                   "%s",
                   state->search_text);
}

static void prefs_build_v6_from_state(const MemConsoleState *state, MemConsoleUiPrefsV6 *out_prefs) {
    MemConsoleUiPrefsV5 legacy_prefs = {0};
    uint32_t all_mask = 0u;

    if (!state || !out_prefs) {
        return;
    }

    memset(out_prefs, 0, sizeof(*out_prefs));
    prefs_build_v5_from_state(state, &legacy_prefs);
    memcpy(out_prefs, &legacy_prefs, sizeof(legacy_prefs));
    out_prefs->version = MEM_CONSOLE_UI_PREFS_VERSION;
    out_prefs->graph_layout_mode = mem_console_graph_layout_mode_clamp(state->graph_layout_mode);
    out_prefs->graph_sort_mode = mem_console_graph_sort_mode_clamp(state->graph_sort_mode);
    all_mask = mem_console_graph_node_kind_filter_all_mask();
    out_prefs->graph_node_kind_filter_all_override = state->graph_node_kind_filter_all_override ? 1 : 0;
    out_prefs->graph_node_kind_filter_mask = state->graph_node_kind_filter_mask & all_mask;
    if (out_prefs->graph_node_kind_filter_all_override) {
        out_prefs->graph_node_kind_filter_mask = all_mask;
    }
}

static void prefs_build_v7_from_state(const MemConsoleState *state, MemConsoleUiPrefsV7 *out_prefs) {
    MemConsoleUiPrefsV6 v6_prefs = {0};

    if (!state || !out_prefs) {
        return;
    }

    memset(out_prefs, 0, sizeof(*out_prefs));
    prefs_build_v6_from_state(state, &v6_prefs);
    memcpy(out_prefs, &v6_prefs, sizeof(v6_prefs));
    out_prefs->version = MEM_CONSOLE_UI_PREFS_VERSION;
    out_prefs->graph_scope_full_mode_enabled = state->graph_scope_full_mode_enabled ? 1 : 0;
}

static void prefs_build_v8_from_state(const MemConsoleState *state, MemConsoleUiPrefsV8 *out_prefs) {
    MemConsoleUiPrefsV7 v7_prefs = {0};
    uint32_t all_mask = 0u;

    if (!state || !out_prefs) {
        return;
    }

    memset(out_prefs, 0, sizeof(*out_prefs));
    prefs_build_v7_from_state(state, &v7_prefs);
    memcpy(out_prefs, &v7_prefs, sizeof(v7_prefs));
    out_prefs->version = MEM_CONSOLE_UI_PREFS_VERSION;

    all_mask = mem_console_graph_kind_filter_all_mask();
    out_prefs->graph_kind_filter_all_override = state->graph_kind_filter_all_override ? 1 : 0;
    out_prefs->graph_kind_filter_mask = state->graph_kind_filter_mask & all_mask;
    if (out_prefs->graph_kind_filter_all_override) {
        out_prefs->graph_kind_filter_mask = all_mask;
    }
    out_prefs->graph_anchor_funnel_enabled = state->graph_anchor_funnel_enabled ? 1 : 0;
    out_prefs->graph_edge_labels_enabled = state->graph_edge_labels_enabled ? 1 : 0;
}

static void prefs_build_v9_from_state(const MemConsoleState *state, MemConsoleUiPrefsV9 *out_prefs) {
    MemConsoleUiPrefsV8 v8_prefs = {0};

    if (!state || !out_prefs) {
        return;
    }

    memset(out_prefs, 0, sizeof(*out_prefs));
    prefs_build_v8_from_state(state, &v8_prefs);
    memcpy(out_prefs, &v8_prefs, sizeof(v8_prefs));
    out_prefs->version = MEM_CONSOLE_UI_PREFS_VERSION;
    prefs_copy_hidden_anchors_from_state(state, out_prefs);
}

static void prefs_build_v10_from_state(const MemConsoleState *state, MemConsoleUiPrefsV10 *out_prefs) {
    MemConsoleUiPrefsV9 v9_prefs = {0};

    if (!state || !out_prefs) {
        return;
    }

    memset(out_prefs, 0, sizeof(*out_prefs));
    prefs_build_v9_from_state(state, &v9_prefs);
    memcpy(out_prefs, &v9_prefs, sizeof(v9_prefs));
    out_prefs->version = MEM_CONSOLE_UI_PREFS_VERSION;
    out_prefs->left_panel_top_ratio = prefs_ratio_or_default(state->left_panel_top_ratio);
}

static void prefs_build_v11_from_state(const MemConsoleState *state, MemConsoleUiPrefsV11 *out_prefs) {
    MemConsoleUiPrefsV10 v10_prefs = {0};

    if (!state || !out_prefs) {
        return;
    }

    memset(out_prefs, 0, sizeof(*out_prefs));
    prefs_build_v10_from_state(state, &v10_prefs);
    memcpy(out_prefs, &v10_prefs, sizeof(v10_prefs));
    out_prefs->version = MEM_CONSOLE_UI_PREFS_VERSION;
    out_prefs->text_zoom_step = (int32_t)state->text_zoom_step;
}

static int prefs_apply_v5_to_state(const MemConsoleUiPrefsV5 *prefs, MemConsoleState *state) {
    if (!prefs || !state) {
        return 0;
    }

    (void)state_set_theme_preset(state, (CoreThemePresetId)prefs->theme_preset_id);
    (void)state_set_font_preset(state, (CoreFontPresetId)prefs->font_preset_id);
    state->pane_left_ratio = prefs_ratio_or_default(prefs->pane_left_ratio);
    state->pane_right_split_ratio = prefs_ratio_or_default(prefs->pane_right_split_ratio);
    state->pane_detail_split_ratio = prefs_ratio_or_default(prefs->pane_detail_split_ratio);
    state->pane_detail_top_split_ratio = prefs_ratio_or_default(prefs->pane_detail_top_split_ratio);
    state->pane_left_collapsed = prefs->pane_left_collapsed ? 1 : 0;
    state->pane_right_detail_collapsed = prefs->pane_right_detail_collapsed ? 1 : 0;

    state->selected_item_id = prefs->selected_item_id > 0 ? prefs->selected_item_id : 0;
    state->list_query_offset = prefs->list_query_offset > 0 ? prefs->list_query_offset : 0;
    prefs_copy_project_filters_to_state(prefs, state);

    (void)snprintf(state->graph_kind_filter,
                   sizeof(state->graph_kind_filter),
                   "%s",
                   prefs->graph_kind_filter);
    mem_console_graph_kind_set_single(state, state->graph_kind_filter);
    state->graph_kind_filter_all_override = prefs->graph_kind_filter_all_override ? 1 : 0;
    mem_console_graph_kind_sync_text_filter(state);
    mem_console_graph_edge_limit_set(state,
                                     mem_console_graph_edge_limit_clamp((int)prefs->graph_edge_limit));
    state->graph_query_hops = mem_console_graph_hops_clamp((int)prefs->graph_hops);
    state->graph_mode_enabled = 1;

    state->graph_viewport.pan_x = prefs_viewport_component_or_default(prefs->graph_pan_x, 0.0f);
    state->graph_viewport.pan_y = prefs_viewport_component_or_default(prefs->graph_pan_y, 0.0f);
    state->graph_viewport.zoom = prefs_viewport_zoom_or_default(prefs->graph_zoom);
    (void)snprintf(state->search_text, sizeof(state->search_text), "%s", prefs->search_text);
    state->graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_DAG;
    state->graph_sort_mode = MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST;
    mem_console_graph_node_kind_select_all(state);

    return 1;
}

static int prefs_apply_v6_to_state(const MemConsoleUiPrefsV6 *prefs, MemConsoleState *state) {
    uint32_t all_mask = 0u;

    if (!prefs || !state) {
        return 0;
    }
    if (!prefs_apply_v5_to_state((const MemConsoleUiPrefsV5 *)prefs, state)) {
        return 0;
    }

    state->graph_layout_mode = mem_console_graph_layout_mode_clamp((int)prefs->graph_layout_mode);
    state->graph_sort_mode = mem_console_graph_sort_mode_clamp((int)prefs->graph_sort_mode);
    all_mask = mem_console_graph_node_kind_filter_all_mask();
    state->graph_node_kind_filter_all_override = prefs->graph_node_kind_filter_all_override ? 1 : 0;
    state->graph_node_kind_filter_mask = ((uint32_t)prefs->graph_node_kind_filter_mask) & all_mask;
    if (state->graph_node_kind_filter_all_override) {
        state->graph_node_kind_filter_mask = all_mask;
    }
    state->graph_scope_full_mode_enabled = 0;
    return 1;
}

static int prefs_apply_v7_to_state(const MemConsoleUiPrefsV7 *prefs, MemConsoleState *state) {
    if (!prefs || !state) {
        return 0;
    }
    if (!prefs_apply_v6_to_state((const MemConsoleUiPrefsV6 *)prefs, state)) {
        return 0;
    }
    state->graph_scope_full_mode_enabled = prefs->graph_scope_full_mode_enabled ? 1 : 0;
    return 1;
}

static int prefs_apply_v8_to_state(const MemConsoleUiPrefsV8 *prefs, MemConsoleState *state) {
    uint32_t all_mask = 0u;

    if (!prefs || !state) {
        return 0;
    }
    if (!prefs_apply_v7_to_state((const MemConsoleUiPrefsV7 *)prefs, state)) {
        return 0;
    }

    all_mask = mem_console_graph_kind_filter_all_mask();
    state->graph_kind_filter_all_override = prefs->graph_kind_filter_all_override ? 1 : 0;
    state->graph_kind_filter_mask = prefs->graph_kind_filter_mask & all_mask;
    if (state->graph_kind_filter_all_override) {
        state->graph_kind_filter_mask = all_mask;
    }
    mem_console_graph_kind_sync_text_filter(state);
    state->graph_anchor_funnel_enabled = prefs->graph_anchor_funnel_enabled ? 1 : 0;
    state->graph_edge_labels_enabled = prefs->graph_edge_labels_enabled ? 1 : 0;
    return 1;
}

static int prefs_apply_v9_to_state(const MemConsoleUiPrefsV9 *prefs, MemConsoleState *state) {
    if (!prefs || !state) {
        return 0;
    }
    if (!prefs_apply_v8_to_state((const MemConsoleUiPrefsV8 *)prefs, state)) {
        return 0;
    }
    prefs_copy_hidden_anchors_to_state(prefs, state);
    return 1;
}

static int prefs_apply_v10_to_state(const MemConsoleUiPrefsV10 *prefs, MemConsoleState *state) {
    float left_ratio = 0.0f;

    if (!prefs || !state) {
        return 0;
    }
    if (!prefs_apply_v9_to_state((const MemConsoleUiPrefsV9 *)prefs, state)) {
        return 0;
    }

    left_ratio = prefs_ratio_or_default(prefs->left_panel_top_ratio);
    if (left_ratio <= 0.0f) {
        left_ratio = 0.58f;
    }
    state->left_panel_top_ratio = left_ratio;
    return 1;
}

static int prefs_apply_v11_to_state(const MemConsoleUiPrefsV11 *prefs, MemConsoleState *state) {
    if (!prefs || !state) {
        return 0;
    }
    if (!prefs_apply_v10_to_state((const MemConsoleUiPrefsV10 *)prefs, state)) {
        return 0;
    }
    (void)state_set_text_zoom_step(state, (int)prefs->text_zoom_step);
    return 1;
}

static int prefs_apply_v3_to_state(const MemConsoleUiPrefsV3 *prefs, MemConsoleState *state) {
    if (!prefs || !state) {
        return 0;
    }

    (void)state_set_theme_preset(state, (CoreThemePresetId)prefs->theme_preset_id);
    (void)state_set_font_preset(state, (CoreFontPresetId)prefs->font_preset_id);
    state->pane_left_ratio = prefs_ratio_or_default(prefs->pane_left_ratio);
    state->pane_right_split_ratio = prefs_ratio_or_default(prefs->pane_right_split_ratio);
    state->pane_detail_split_ratio = 0.0f;
    state->pane_detail_top_split_ratio = 0.0f;
    state->pane_left_collapsed = prefs->pane_left_collapsed ? 1 : 0;
    state->pane_right_detail_collapsed = prefs->pane_right_detail_collapsed ? 1 : 0;

    state->selected_item_id = prefs->selected_item_id > 0 ? prefs->selected_item_id : 0;
    state->list_query_offset = prefs->list_query_offset > 0 ? prefs->list_query_offset : 0;
    state->selected_project_count = 0;
    {
        int i;
        int write_index = 0;
        int load_count = prefs->selected_project_count;
        if (load_count < 0) {
            load_count = 0;
        }
        if (load_count > MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
            load_count = MEM_CONSOLE_SCOPE_FILTER_LIMIT;
        }
        for (i = 0; i < load_count; ++i) {
            if (prefs->selected_project_keys[i][0] == '\0') {
                continue;
            }
            (void)snprintf(state->selected_project_keys[write_index],
                           sizeof(state->selected_project_keys[write_index]),
                           "%s",
                           prefs->selected_project_keys[i]);
            write_index += 1;
            if (write_index >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
                break;
            }
        }
        for (i = write_index; i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
            state->selected_project_keys[i][0] = '\0';
        }
        state->selected_project_count = write_index;
    }

    (void)snprintf(state->graph_kind_filter,
                   sizeof(state->graph_kind_filter),
                   "%s",
                   prefs->graph_kind_filter);
    mem_console_graph_kind_set_single(state, state->graph_kind_filter);
    mem_console_graph_edge_limit_set(state,
                                     mem_console_graph_edge_limit_clamp((int)prefs->graph_edge_limit));
    state->graph_query_hops = mem_console_graph_hops_clamp((int)prefs->graph_hops);
    state->graph_mode_enabled = 1;
    state->graph_viewport.pan_x = prefs_viewport_component_or_default(prefs->graph_pan_x, 0.0f);
    state->graph_viewport.pan_y = prefs_viewport_component_or_default(prefs->graph_pan_y, 0.0f);
    state->graph_viewport.zoom = prefs_viewport_zoom_or_default(prefs->graph_zoom);
    (void)snprintf(state->search_text, sizeof(state->search_text), "%s", prefs->search_text);
    return 1;
}

int mem_console_build_prefs_path(const char *db_path, char *out_path, size_t out_cap) {
    return mem_console_build_prefs_path_impl(db_path, out_path, out_cap);
}

int mem_console_build_app_prefs_path(char *out_path, size_t out_cap) {
    return mem_console_build_app_prefs_path_impl(out_path, out_cap);
}

int mem_console_build_app_prefs_path_for_output_root(const char *output_root,
                                                     char *out_path,
                                                     size_t out_cap) {
    return mem_console_build_app_prefs_path_for_output_root_impl(output_root, out_path, out_cap);
}

CoreResult mem_console_prefs_load(const char *prefs_path, MemConsoleState *state) {
    CorePackReader reader = {0};
    CorePackChunkInfo chunk = {0};
    CoreResult result;
    MemConsoleUiPrefsV11 prefs = {0};
    FILE *probe = 0;
    int loaded_any = 0;

    if (!prefs_path || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid prefs load request" };
    }

    probe = fopen(prefs_path, "rb");
    if (!probe) {
        return core_result_ok();
    }
    fclose(probe);

    result = core_pack_reader_open(prefs_path, &reader);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_pack_reader_find_chunk(&reader, "MCFG", 0, &chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return core_result_ok();
    }

    if (chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV1) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV2) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV3) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV4) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV5) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV6) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV7) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV8) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV9) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV10) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV11)) {
        (void)core_pack_reader_close(&reader);
        return (CoreResult){ CORE_ERR_FORMAT, "invalid mem_console prefs payload size" };
    }

    result = core_pack_reader_read_chunk_data(&reader, &chunk, &prefs, sizeof(prefs));
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return result;
    }

    if (prefs.version == 1u && chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV1)) {
        if (state_set_theme_preset(state, (CoreThemePresetId)prefs.theme_preset_id)) {
            loaded_any = 1;
        }
        if (state_set_font_preset(state, (CoreFontPresetId)prefs.font_preset_id)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 2u && chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV2)) {
        if (state_set_theme_preset(state, (CoreThemePresetId)prefs.theme_preset_id)) {
            loaded_any = 1;
        }
        if (state_set_font_preset(state, (CoreFontPresetId)prefs.font_preset_id)) {
            loaded_any = 1;
        }
        state->pane_left_ratio = prefs_ratio_or_default(prefs.pane_left_ratio);
        state->pane_right_split_ratio = prefs_ratio_or_default(prefs.pane_right_split_ratio);
        state->pane_left_collapsed = prefs.pane_left_collapsed ? 1 : 0;
        state->pane_right_detail_collapsed = prefs.pane_right_detail_collapsed ? 1 : 0;
        loaded_any = 1;
    } else if (prefs.version == 3u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV3)) {
        if (prefs_apply_v3_to_state((const MemConsoleUiPrefsV3 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 4u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV4)) {
        if (prefs_apply_v5_to_state((const MemConsoleUiPrefsV5 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 5u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV5)) {
        if (prefs_apply_v5_to_state((const MemConsoleUiPrefsV5 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 6u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV6)) {
        if (prefs_apply_v6_to_state((const MemConsoleUiPrefsV6 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 7u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV7)) {
        if (prefs_apply_v7_to_state((const MemConsoleUiPrefsV7 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 8u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV8)) {
        if (prefs_apply_v8_to_state((const MemConsoleUiPrefsV8 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 9u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV9)) {
        if (prefs_apply_v9_to_state((const MemConsoleUiPrefsV9 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 10u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV10)) {
        if (prefs_apply_v10_to_state((const MemConsoleUiPrefsV10 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == MEM_CONSOLE_UI_PREFS_VERSION &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV11)) {
        if (prefs_apply_v11_to_state(&prefs, state)) {
            loaded_any = 1;
        }
    }

    result = core_pack_reader_close(&reader);
    if (result.code != CORE_OK) {
        return result;
    }

    if (loaded_any) {
        state->graph_mode_enabled = 1;
        state->pane_prefs_dirty = 0;
        return (CoreResult){ CORE_OK, "prefs loaded" };
    }
    return core_result_ok();
}

CoreResult mem_console_prefs_save(const char *prefs_path, const MemConsoleState *state) {
    CorePackWriter writer = {0};
    CoreResult result;
    MemConsoleUiPrefsV11 prefs = {0};

    if (!prefs_path || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid prefs save request" };
    }
    if (!mem_console_ensure_parent_directory(prefs_path)) {
        return (CoreResult){ CORE_ERR_IO, "failed to create prefs directory" };
    }

    prefs_build_v11_from_state(state, &prefs);

    result = core_pack_writer_open(prefs_path, &writer);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_pack_writer_add_chunk(&writer, "MCFG", &prefs, (uint64_t)sizeof(prefs));
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

uint64_t mem_console_prefs_state_signature(const MemConsoleState *state) {
    MemConsoleUiPrefsV11 prefs = {0};

    if (!state) {
        return 0u;
    }

    prefs_build_v11_from_state(state, &prefs);
    return core_hash64_fnv1a(&prefs, sizeof(prefs));
}
