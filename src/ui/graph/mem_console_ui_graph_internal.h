#ifndef MEM_CONSOLE_UI_GRAPH_INTERNAL_H
#define MEM_CONSOLE_UI_GRAPH_INTERNAL_H

#include "mem_console_ui_graph.h"

#include "mem_console_ui_common.h"
#include "mem_console_ui_hud.h"

#define GRAPH_NODE_ZOOM_SIZE_CAP 1.18f
#define GRAPH_NODE_MIN_RENDER_WIDTH_PX 0.93f
#define GRAPH_NODE_MIN_RENDER_HEIGHT_PX 0.80f
#define GRAPH_NODE_TEXT_HIDE_WIDTH_PX 10.0f
#define GRAPH_NODE_TEXT_HIDE_HEIGHT_PX 8.0f
#define GRAPH_NODE_TEXT_MIN_ZOOM 1.20f

typedef struct GraphEdgeLegendEntry {
    const char *kind;
    const char *label;
    KitRenderColor color;
} GraphEdgeLegendEntry;

typedef enum GraphBucketRole {
    GRAPH_BUCKET_ROLE_NONE = 0,
    GRAPH_BUCKET_ROLE_SCOPE = 1,
    GRAPH_BUCKET_ROLE_PLANS = 2,
    GRAPH_BUCKET_ROLE_DECISIONS = 3,
    GRAPH_BUCKET_ROLE_ISSUES = 4,
    GRAPH_BUCKET_ROLE_MISC = 5
} GraphBucketRole;

typedef struct GraphProjectPod {
    char key[64];
    int node_indices[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int node_count;
    KitRenderRect bounds;
} GraphProjectPod;

const GraphEdgeLegendEntry *graph_edge_legend_entry_for_kind(const char *kind);
uint32_t graph_edge_legend_entry_count(void);
const GraphEdgeLegendEntry *graph_edge_legend_entry_at(uint32_t index);
KitRenderColor graph_edge_color_for_kind(const char *kind);
const char *graph_edge_display_label_for_kind(const char *kind);

void graph_format_project_display_name(const char *project_key,
                                       char *out_text,
                                       size_t out_cap);
GraphBucketRole graph_bucket_role_for_node(const MemConsoleGraphNode *node);
const char *graph_bucket_role_label(GraphBucketRole role);
KitRenderColor graph_bucket_border_color(GraphBucketRole role);
void graph_build_node_label_text(const MemConsoleGraphNode *node,
                                 char *out_text,
                                 size_t out_cap);
int graph_edge_is_hierarchy_kind(const char *kind);

int graph_collect_project_pods(const MemConsoleState *state,
                               const KitGraphStructNodeLayout *layouts,
                               uint32_t layout_count,
                               GraphProjectPod *out_pods,
                               int out_pod_cap);
void apply_project_pod_layout(KitRenderRect bounds,
                              const MemConsoleState *state,
                              KitGraphStructNodeLayout *layouts,
                              uint32_t layout_count);

float graph_clampf(float value, float min_v, float max_v);
void refine_edge_label_layouts_for_callouts(KitRenderRect bounds,
                                            const KitGraphStructEdgeRoute *routes,
                                            uint32_t route_count,
                                            KitGraphStructEdgeLabelLayout *label_layouts);
KitRenderVec2 compute_label_attach_point(KitRenderRect rect, KitRenderVec2 anchor);

void graph_camera_apply_to_layouts(KitGraphStructNodeLayout *layouts,
                                   uint32_t layout_count,
                                   const KitGraphStructViewport *viewport,
                                   KitRenderRect graph_bounds);
void graph_camera_pan_by_screen_delta(KitGraphStructViewport *viewport,
                                      float delta_x,
                                      float delta_y);

CoreResult draw_rect_outline(KitRenderFrame *frame,
                             KitRenderRect rect,
                             float thickness,
                             KitRenderColor color);
CoreResult draw_project_pod_overlays(const KitRenderContext *render_ctx,
                                     KitRenderFrame *frame,
                                     MemConsoleState *state,
                                     KitRenderRect bounds);
CoreResult draw_graph_edge_legend(KitUiContext *ui_ctx,
                                  const KitRenderContext *render_ctx,
                                  const KitUiInputState *input,
                                  KitRenderFrame *frame,
                                  KitRenderRect bounds,
                                  MemConsoleState *state,
                                  int *out_click_consumed,
                                  int *out_filter_changed);
CoreResult draw_graph_view_diagnostics(KitUiContext *ui_ctx,
                                       const KitRenderContext *render_ctx,
                                       KitRenderFrame *frame,
                                       MemConsoleState *state,
                                       KitRenderRect bounds,
                                       uint32_t node_count,
                                       uint32_t edge_count);

float graph_edge_route_hit_radius_for_zoom(const MemConsoleState *state);
int graph_find_edge_index_at_point(const MemConsoleState *state,
                                   float mouse_x,
                                   float mouse_y,
                                   float route_hit_radius_px,
                                   uint32_t *out_edge_index);
int graph_build_node_hud_spec(MemConsoleState *state,
                              int hovered_node_index,
                              MemConsoleUiHudCardSpec *out_spec);
int graph_build_edge_hud_spec(MemConsoleState *state,
                              int hovered_edge_index,
                              MemConsoleUiHudCardSpec *out_spec);

#endif
