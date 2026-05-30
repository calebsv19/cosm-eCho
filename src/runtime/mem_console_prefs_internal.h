#ifndef MEM_CONSOLE_PREFS_INTERNAL_H
#define MEM_CONSOLE_PREFS_INTERNAL_H

#include "mem_console_prefs.h"

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

float prefs_ratio_or_default(float ratio);
float prefs_viewport_component_or_default(float value, float fallback);
float prefs_viewport_zoom_or_default(float zoom);
void prefs_build_v11_from_state(const MemConsoleState *state, MemConsoleUiPrefsV11 *out_prefs);
int prefs_apply_v3_to_state(const MemConsoleUiPrefsV3 *prefs, MemConsoleState *state);
int prefs_apply_v5_to_state(const MemConsoleUiPrefsV5 *prefs, MemConsoleState *state);
int prefs_apply_v6_to_state(const MemConsoleUiPrefsV6 *prefs, MemConsoleState *state);
int prefs_apply_v7_to_state(const MemConsoleUiPrefsV7 *prefs, MemConsoleState *state);
int prefs_apply_v8_to_state(const MemConsoleUiPrefsV8 *prefs, MemConsoleState *state);
int prefs_apply_v9_to_state(const MemConsoleUiPrefsV9 *prefs, MemConsoleState *state);
int prefs_apply_v10_to_state(const MemConsoleUiPrefsV10 *prefs, MemConsoleState *state);
int prefs_apply_v11_to_state(const MemConsoleUiPrefsV11 *prefs, MemConsoleState *state);

#endif
