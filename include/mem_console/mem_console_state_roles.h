#ifndef MEM_CONSOLE_STATE_ROLES_H
#define MEM_CONSOLE_STATE_ROLES_H

#include <stddef.h>

#include "mem_console_state.h"

typedef struct MemConsoleLeftPanelRenderState {
    const char *db_path;
    const char *input_root;
    const char *schema_version;
    const char *theme_name;
    const char *status_line;
    const char *runtime_summary_line;
    int64_t active_count;
    int64_t matching_count;
    int visible_count;
    const MemConsoleListItem *visible_items;
} MemConsoleLeftPanelRenderState;

typedef struct MemConsoleLeftPanelRenderStorage {
    char *db_summary_line;
    size_t db_summary_line_cap;
    char *db_summary_draw_line;
    size_t db_summary_draw_line_cap;
    char *input_root_summary_line;
    size_t input_root_summary_line_cap;
    char *input_root_summary_draw_line;
    size_t input_root_summary_draw_line_cap;
    char *schema_summary_line;
    size_t schema_summary_line_cap;
    char *visible_summary_line;
    size_t visible_summary_line_cap;
    char *status_draw_line;
    size_t status_draw_line_cap;
    char *runtime_summary_draw_line;
    size_t runtime_summary_draw_line_cap;
    char (*list_item_labels)[MEM_CONSOLE_LIST_ITEM_LABEL_CAP];
    int list_item_label_count;
} MemConsoleLeftPanelRenderStorage;

typedef struct MemConsoleDetailRenderState {
    int64_t selected_item_id;
    int64_t selected_created_ns;
    const char *selected_title;
    const MemConsoleRelationshipItem *relationships;
    int relationship_count;
    int relationship_out_count;
    int relationship_in_count;
    const char *relationship_summary_line;
} MemConsoleDetailRenderState;

typedef struct MemConsoleDetailRenderStorage {
    char (*title_lines)[MEM_CONSOLE_DETAIL_TEXT_LINE_CAP];
    int title_line_capacity;
    int *title_line_count;
    char *meta_line;
    size_t meta_line_cap;
    char (*connection_summary_lines)[MEM_CONSOLE_DETAIL_CONNECTION_LINE_CAP];
    int connection_summary_line_capacity;
    char (*relationship_group_labels)[MEM_CONSOLE_DETAIL_RELATIONSHIP_GROUP_LABEL_CAP];
    char (*relationship_row_labels)[MEM_CONSOLE_DETAIL_RELATIONSHIP_ROW_LABEL_CAP];
    int relationship_label_capacity;
} MemConsoleDetailRenderStorage;

typedef struct MemConsoleGraphRenderState {
    const MemConsoleGraphNode *nodes;
    int node_count;
    const MemConsoleGraphEdge *edges;
    int edge_count;
} MemConsoleGraphRenderState;

typedef struct MemConsoleGraphRenderStorage {
    char (*edge_labels)[MEM_CONSOLE_GRAPH_EDGE_LABEL_CAP];
    int edge_label_capacity;
    char (*node_labels)[MEM_CONSOLE_GRAPH_NODE_LABEL_CAP];
    int node_label_capacity;
} MemConsoleGraphRenderStorage;

int mem_console_left_panel_render_state_from_state(const MemConsoleState *state,
                                                   MemConsoleLeftPanelRenderState *out_view);
int mem_console_left_panel_render_storage_from_state(MemConsoleState *state,
                                                     MemConsoleLeftPanelRenderStorage *out_storage);
void mem_console_left_panel_derive_db_summary(const MemConsoleLeftPanelRenderState *view,
                                              MemConsoleLeftPanelRenderStorage *storage,
                                              float width_px);
void mem_console_left_panel_derive_input_root_summary(const MemConsoleLeftPanelRenderState *view,
                                                      MemConsoleLeftPanelRenderStorage *storage,
                                                      float width_px);
void mem_console_left_panel_derive_schema_summary(const MemConsoleLeftPanelRenderState *view,
                                                  MemConsoleLeftPanelRenderStorage *storage);
void mem_console_left_panel_derive_visible_summary(const MemConsoleLeftPanelRenderState *view,
                                                   MemConsoleLeftPanelRenderStorage *storage);
void mem_console_left_panel_derive_status_summary(const MemConsoleLeftPanelRenderState *view,
                                                  MemConsoleLeftPanelRenderStorage *storage,
                                                  float width_px);
void mem_console_left_panel_derive_runtime_summary(const MemConsoleLeftPanelRenderState *view,
                                                   MemConsoleLeftPanelRenderStorage *storage,
                                                   float width_px);
int mem_console_left_panel_derive_item_label(const MemConsoleLeftPanelRenderState *view,
                                             MemConsoleLeftPanelRenderStorage *storage,
                                             int visible_index);

int mem_console_detail_render_state_from_state(const MemConsoleState *state,
                                               MemConsoleDetailRenderState *out_view);
int mem_console_detail_render_storage_from_state(MemConsoleState *state,
                                                 MemConsoleDetailRenderStorage *out_storage);
float mem_console_detail_wrapped_text_line_step(CoreFontTextSizeTier text_tier);
int mem_console_detail_wrap_text_lines(const char *text,
                                       char line_storage[][MEM_CONSOLE_DETAIL_TEXT_LINE_CAP],
                                       int line_storage_count,
                                       int max_chars);
void mem_console_detail_clear_title_lines(MemConsoleDetailRenderStorage *storage);
int mem_console_detail_derive_title_lines(const MemConsoleDetailRenderState *view,
                                          MemConsoleDetailRenderStorage *storage,
                                          int line_limit,
                                          int max_chars);
void mem_console_detail_derive_meta_line(const MemConsoleDetailRenderState *view,
                                         MemConsoleDetailRenderStorage *storage);
const char *mem_console_detail_relationship_header_label(const MemConsoleDetailRenderState *view);
int mem_console_detail_relationship_group_changed(const MemConsoleRelationshipItem *prev,
                                                  const MemConsoleRelationshipItem *item);
int mem_console_detail_relationship_group_count(const MemConsoleDetailRenderState *view);
void mem_console_detail_derive_empty_relationship_line(const MemConsoleDetailRenderState *view,
                                                       MemConsoleDetailRenderStorage *storage);
int mem_console_detail_derive_relationship_group_label(const MemConsoleDetailRenderState *view,
                                                       MemConsoleDetailRenderStorage *storage,
                                                       int group_index,
                                                       const MemConsoleRelationshipItem *item);
int mem_console_detail_derive_relationship_row_label(const MemConsoleDetailRenderState *view,
                                                     MemConsoleDetailRenderStorage *storage,
                                                     int row_index,
                                                     const MemConsoleRelationshipItem *item);

int mem_console_graph_render_state_from_state(const MemConsoleState *state,
                                              MemConsoleGraphRenderState *out_view);
int mem_console_graph_render_storage_from_state(MemConsoleState *state,
                                                MemConsoleGraphRenderStorage *out_storage);
int mem_console_graph_derive_edge_label(const MemConsoleGraphRenderState *view,
                                        MemConsoleGraphRenderStorage *storage,
                                        int edge_index,
                                        const char *edge_kind_label);
int mem_console_graph_derive_node_label(const MemConsoleGraphRenderState *view,
                                        MemConsoleGraphRenderStorage *storage,
                                        int node_index);

#endif
