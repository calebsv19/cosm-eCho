#ifndef MEM_CONSOLE_TYPES_H
#define MEM_CONSOLE_TYPES_H

#include <stdint.h>

#include "core_memdb.h"
#include "kit_graph_struct.h"

enum {
    MEM_CONSOLE_LIST_FETCH_LIMIT = 96,
    MEM_CONSOLE_LIST_ROW_PITCH_PX = 36,
    MEM_CONSOLE_GRAPH_NODE_LIMIT = 128,
    MEM_CONSOLE_GRAPH_EDGE_LIMIT = 1024,
    MEM_CONSOLE_GRAPH_PROJECT_POD_LIMIT = 48,
    MEM_CONSOLE_GRAPH_EDGE_LIMIT_DEFAULT = 128,
    MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN = 4,
    MEM_CONSOLE_GRAPH_HOPS_MIN = 1,
    MEM_CONSOLE_GRAPH_HOPS_MAX = 10,
    MEM_CONSOLE_SCOPE_FILTER_LIMIT = 16,
    MEM_CONSOLE_DETAIL_BODY_WRAP_LINE_LIMIT = 96,
    MEM_CONSOLE_DETAIL_CONNECTION_WRAP_LINE_LIMIT = 48,
    MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT = 12,
    MEM_CONSOLE_GRAPH_HUD_LINE_LIMIT = 64,
    MEM_CONSOLE_DB_PICKER_LIST_LIMIT = 64
};

typedef enum MemConsoleAction {
    MEM_CONSOLE_ACTION_NONE = 0,
    MEM_CONSOLE_ACTION_REFRESH = 1,
    MEM_CONSOLE_ACTION_CREATE_FROM_SEARCH = 2,
    MEM_CONSOLE_ACTION_BEGIN_TITLE_EDIT = 3,
    MEM_CONSOLE_ACTION_CANCEL_TITLE_EDIT = 4,
    MEM_CONSOLE_ACTION_SAVE_TITLE_EDIT = 5,
    MEM_CONSOLE_ACTION_BEGIN_BODY_EDIT = 6,
    MEM_CONSOLE_ACTION_CANCEL_BODY_EDIT = 7,
    MEM_CONSOLE_ACTION_SAVE_BODY_EDIT = 8,
    MEM_CONSOLE_ACTION_TOGGLE_PINNED = 9,
    MEM_CONSOLE_ACTION_TOGGLE_CANONICAL = 10,
    MEM_CONSOLE_ACTION_TOGGLE_GRAPH_MODE = 11,
    MEM_CONSOLE_ACTION_REFRESH_GRAPH = 12,
    MEM_CONSOLE_ACTION_CENTER_GRAPH = 13,
    MEM_CONSOLE_ACTION_CENTER_SELECTED = 14,
    MEM_CONSOLE_ACTION_BEGIN_DB_PICKER = 15,
    MEM_CONSOLE_ACTION_BEGIN_DB_CREATE = 16,
    MEM_CONSOLE_ACTION_CANCEL_DB_PICKER = 17,
    MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER = 18,
    MEM_CONSOLE_ACTION_BEGIN_INPUT_ROOT_PICKER = 19,
    MEM_CONSOLE_ACTION_PICK_INPUT_ROOT_FOLDER = 20
} MemConsoleAction;

typedef enum MemConsoleInputTarget {
    MEM_CONSOLE_INPUT_SEARCH = 0,
    MEM_CONSOLE_INPUT_TITLE_EDIT = 1,
    MEM_CONSOLE_INPUT_BODY_EDIT = 2,
    MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT = 3,
    MEM_CONSOLE_INPUT_DB_PATH = 4
} MemConsoleInputTarget;

typedef enum MemConsoleRedrawReason {
    MEM_CONSOLE_REDRAW_REASON_NONE = 0u,
    MEM_CONSOLE_REDRAW_REASON_INPUT = 1u << 0,
    MEM_CONSOLE_REDRAW_REASON_LAYOUT = 1u << 1,
    MEM_CONSOLE_REDRAW_REASON_THEME = 1u << 2,
    MEM_CONSOLE_REDRAW_REASON_CONTENT = 1u << 3,
    MEM_CONSOLE_REDRAW_REASON_BACKGROUND = 1u << 4
} MemConsoleRedrawReason;

typedef enum MemConsoleGraphLayoutMode {
    MEM_CONSOLE_GRAPH_LAYOUT_DAG = 0,
    MEM_CONSOLE_GRAPH_LAYOUT_TREE = 1
} MemConsoleGraphLayoutMode;

typedef enum MemConsoleGraphSortMode {
    MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST = 0,
    MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST = 1
} MemConsoleGraphSortMode;

typedef struct MemConsoleListItem {
    int64_t id;
    int pinned;
    int canonical;
    char title[160];
    char workspace_key[64];
    char project_key[64];
    char kind[64];
} MemConsoleListItem;

typedef struct MemConsoleGraphNode {
    int64_t item_id;
    int64_t created_ns;
    int pinned;
    int canonical;
    char title[160];
    char body_preview[256];
    char project_key[64];
    char kind[32];
    char stable_id[96];
} MemConsoleGraphNode;

typedef struct MemConsoleGraphEdge {
    int from_index;
    int to_index;
    char kind[32];
} MemConsoleGraphEdge;

typedef struct MemConsoleState {
    const char *db_path;
    char db_path_storage[1024];
    char input_root[1024];
    char output_root[1024];
    char active_db_path[1024];
    char pending_db_path[1024];
    char search_text[256];
    char db_modal_text[896];
    char db_modal_resolved_path[1024];
    char db_modal_visible_text[896];
    char db_modal_resolved_line[1100];
    char db_modal_active_line[1100];
    char db_picker_entry_names[MEM_CONSOLE_DB_PICKER_LIST_LIMIT][128];
    char db_picker_entry_paths[MEM_CONSOLE_DB_PICKER_LIST_LIMIT][1024];
    char db_summary_line[384];
    char db_summary_draw_line[384];
    char schema_version[32];
    char status_line[160];
    char status_draw_line[160];
    char theme_name[64];
    char font_name[64];
    char selected_title[160];
    char detail_title_draw_line[192];
    char selected_body[256];
    char title_edit_text[160];
    char body_edit_text[1024];
    char schema_summary_line[96];
    char runtime_summary_line[128];
    char runtime_summary_draw_line[128];
    char kernel_summary_line[96];
    char kernel_summary_draw_line[96];
    char redraw_summary_line[96];
    char redraw_summary_draw_line[96];
    char theme_summary_line[96];
    char font_summary_line[96];
    char visible_summary_line[96];
    char detail_meta_line[96];
    char pinned_button_label[32];
    char canonical_button_label[32];
    char graph_mode_button_label[32];
    char graph_status_line[96];
    char graph_kind_filter[32];
    char graph_edge_limit_text[16];
    char project_filter_summary_line[128];
    char list_item_labels[MEM_CONSOLE_LIST_FETCH_LIMIT][220];
    char project_filter_labels[MEM_CONSOLE_SCOPE_FILTER_LIMIT][96];
    char project_filter_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    int64_t project_filter_counts[MEM_CONSOLE_SCOPE_FILTER_LIMIT];
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char wrapped_body_lines[MEM_CONSOLE_DETAIL_BODY_WRAP_LINE_LIMIT][256];
    CoreThemePresetId theme_preset_id;
    CoreFontPresetId font_preset_id;
    int text_zoom_step;
    int64_t selected_item_id;
    int64_t graph_center_item_id;
    int64_t selected_created_ns;
    int64_t active_count;
    int64_t matching_count;
    int selected_pinned;
    int selected_canonical;
    int title_edit_mode;
    int body_edit_mode;
    int db_modal_open;
    int db_modal_create_mode;
    int db_modal_input_root_mode;
    int search_cursor;
    int title_edit_cursor;
    int body_edit_cursor;
    int db_modal_cursor;
    int db_modal_selection_anchor;
    int db_modal_selection_start;
    int db_modal_selection_end;
    int db_modal_drag_select_active;
    int db_picker_entry_count;
    int db_picker_selected_index;
    int graph_edge_limit_cursor;
    int graph_mode_enabled;
    int graph_query_edge_limit;
    int graph_query_hops;
    int graph_layout_mode;
    int graph_sort_mode;
    int graph_scope_full_mode_enabled;
    int graph_anchor_funnel_enabled;
    int graph_edge_labels_enabled;
    uint32_t graph_kind_filter_mask;
    int graph_kind_filter_all_override;
    int graph_hidden_anchor_count;
    int64_t graph_hidden_anchor_item_ids[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    uint32_t graph_node_kind_filter_mask;
    int graph_node_kind_filter_all_override;
    int list_query_offset;
    int visible_start_index;
    int visible_count;
    int search_refresh_pending;
    MemConsoleInputTarget input_target;
    uint64_t search_last_input_ms;
    uint64_t runtime_refresh_submitted;
    uint64_t runtime_refresh_applied;
    uint64_t runtime_refresh_dropped;
    uint64_t runtime_refresh_errors;
    uint64_t runtime_refresh_coalesced;
    uint64_t redraw_frame_count;
    uint64_t redraw_last_frame_ms;
    uint32_t redraw_pending_reasons;
    uint32_t redraw_last_reasons;
    int runtime_refresh_in_flight;
    int runtime_pending_intent;
    int project_filter_option_count;
    int selected_project_count;
    int kernel_bridge_enabled;
    uint64_t kernel_tick_count;
    uint64_t kernel_last_work_units;
    int kernel_last_render_requested;
    float list_scroll;
    float project_filter_scroll;
    float detail_connection_scroll;
    float detail_body_scroll;
    MemConsoleListItem visible_items[MEM_CONSOLE_LIST_FETCH_LIMIT];
    int graph_node_count;
    int graph_edge_count;
    MemConsoleGraphNode graph_nodes[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    MemConsoleGraphEdge graph_edges[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    int graph_layout_valid;
    int graph_layout_has_graph_data;
    uint64_t graph_layout_signature;
    KitRenderRect graph_layout_bounds;
    uint32_t graph_layout_node_count;
    uint32_t graph_layout_edge_count;
    KitGraphStructNode graph_layout_nodes[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    KitGraphStructEdge graph_layout_edges[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    int graph_layout_edge_state_indices[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    KitGraphStructNodeLayout graph_layout_node_layouts[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    KitGraphStructEdgeRoute graph_layout_edge_routes[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    KitGraphStructEdgeLabelLayout graph_layout_edge_label_layouts[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    char graph_draw_edge_labels[MEM_CONSOLE_GRAPH_EDGE_LIMIT][96];
    char graph_draw_pod_labels[MEM_CONSOLE_GRAPH_PROJECT_POD_LIMIT][96];
    char graph_draw_node_labels[MEM_CONSOLE_GRAPH_NODE_LIMIT][192];
    char graph_hud_id_line[48];
    char graph_hud_title_raw[176];
    char graph_hud_title[176];
    char graph_hud_flags[96];
    char graph_hud_body[256];
    char graph_hud_wrapped_lines[16][256];
    int graph_hud_cache_valid;
    uint64_t graph_hud_cache_signature;
    KitRenderRect graph_hud_cache_outer;
    KitRenderRect graph_hud_cache_inner;
    int graph_hud_cache_row_count;
    int graph_hud_cache_line_count;
    int graph_hud_cache_row_first_line[MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT];
    int graph_hud_cache_row_line_count[MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT];
    CoreThemeColorToken graph_hud_cache_row_tokens[MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT];
    CoreFontRoleId graph_hud_cache_row_roles[MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT];
    CoreFontTextSizeTier graph_hud_cache_row_tiers[MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT];
    float graph_hud_cache_row_line_steps[MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT];
    char graph_hud_cache_lines[MEM_CONSOLE_GRAPH_HUD_LINE_LIMIT][256];
    char detail_connection_summary_text[640];
    char detail_connection_summary_lines[MEM_CONSOLE_DETAIL_CONNECTION_WRAP_LINE_LIMIT][256];
    int graph_drag_active;
    int graph_drag_moved;
    int graph_click_armed;
    uint64_t list_last_click_ms;
    int64_t list_last_click_item_id;
    uint64_t graph_last_click_ms;
    int64_t graph_last_click_item_id;
    float graph_drag_last_x;
    float graph_drag_last_y;
    float pane_left_ratio;
    float pane_right_split_ratio;
    float pane_detail_split_ratio;
    float pane_detail_top_split_ratio;
    float left_panel_top_ratio;
    int pane_drag_active;
    int pane_drag_splitter_id;
    float pane_drag_anchor_x;
    float pane_drag_anchor_y;
    float pane_drag_start_left_ratio;
    float pane_drag_start_right_ratio;
    float pane_drag_start_detail_ratio;
    float pane_drag_start_detail_top_ratio;
    int left_panel_drag_active;
    float left_panel_drag_anchor_y;
    float left_panel_drag_start_ratio;
    int pane_left_collapsed;
    int pane_right_detail_collapsed;
    int pane_prefs_dirty;
    KitRenderRect left_pane;
    KitRenderRect right_pane;
    KitRenderRect pane_right_detail;
    KitRenderRect pane_right_detail_meta;
    KitRenderRect pane_right_detail_connections;
    KitRenderRect pane_right_detail_body;
    KitRenderRect pane_right_graph;
    KitGraphStructViewport graph_viewport;
} MemConsoleState;

#endif
