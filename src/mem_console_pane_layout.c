#include "mem_console_pane_layout.h"

#include "core_pane.h"

enum {
    MEM_CONSOLE_PANE_TREE_NODE_ROOT = 0,
    MEM_CONSOLE_PANE_TREE_NODE_LEFT = 1,
    MEM_CONSOLE_PANE_TREE_NODE_RIGHT_SPLIT = 2,
    MEM_CONSOLE_PANE_TREE_NODE_DETAIL = 3,
    MEM_CONSOLE_PANE_TREE_NODE_GRAPH = 4,
    MEM_CONSOLE_PANE_TREE_NODE_COUNT = 5
};

static float pane_clampf(float value, float min_value, float max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static float pane_non_negative(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    return value;
}

static int pane_point_in_rect(KitRenderRect rect, float x, float y) {
    if (x < rect.x || y < rect.y) return 0;
    if (x > rect.x + rect.width) return 0;
    if (y > rect.y + rect.height) return 0;
    return 1;
}

static void pane_normalize_frame(const MemConsoleLayoutConfig *layout_cfg,
                                 int *io_frame_width,
                                 int *io_frame_height) {
    if (!layout_cfg || !io_frame_width || !io_frame_height) {
        return;
    }
    if (*io_frame_width < layout_cfg->min_frame_width) {
        *io_frame_width = layout_cfg->min_frame_width;
    }
    if (*io_frame_height < layout_cfg->min_frame_height) {
        *io_frame_height = layout_cfg->min_frame_height;
    }
}

static void pane_ratio_limits_for_span(float span,
                                       float min_a,
                                       float min_b,
                                       float *out_min_ratio,
                                       float *out_max_ratio) {
    float local_min_a;
    float local_min_b;
    float min_ratio = 0.0f;
    float max_ratio = 1.0f;

    if (span <= 0.0f) {
        if (out_min_ratio) *out_min_ratio = 0.0f;
        if (out_max_ratio) *out_max_ratio = 1.0f;
        return;
    }

    local_min_a = pane_non_negative(min_a);
    local_min_b = pane_non_negative(min_b);
    if ((local_min_a + local_min_b) > span && (local_min_a + local_min_b) > 0.0f) {
        float scale = span / (local_min_a + local_min_b);
        local_min_a *= scale;
        local_min_b *= scale;
    }

    min_ratio = local_min_a / span;
    max_ratio = 1.0f - (local_min_b / span);
    if (max_ratio < min_ratio) {
        float t = min_ratio;
        min_ratio = max_ratio;
        max_ratio = t;
    }
    min_ratio = pane_clampf(min_ratio, 0.0f, 1.0f);
    max_ratio = pane_clampf(max_ratio, 0.0f, 1.0f);

    if (out_min_ratio) *out_min_ratio = min_ratio;
    if (out_max_ratio) *out_max_ratio = max_ratio;
}

static float estimate_right_detail_height(const MemConsoleLayoutConfig *layout_cfg,
                                          int graph_mode_enabled) {
    float gap = 8.0f;
    float action_block_h;
    float height;

    if (!layout_cfg) {
        return 0.0f;
    }

    action_block_h = (layout_cfg->action_button_h * 2.0f) +
                     layout_cfg->action_button_gap +
                     (layout_cfg->action_block_pad * 2.0f);

    height = layout_cfg->right_header_h + gap +
             layout_cfg->right_meta_h + gap +
             layout_cfg->right_section_h + gap +
             layout_cfg->right_body_h + gap +
             layout_cfg->right_section_h + gap;

    if (graph_mode_enabled) {
        height += layout_cfg->graph_filter_h + gap +
                  layout_cfg->graph_settings_h + gap;
    } else {
        height += layout_cfg->graph_collapsed_hint_h + gap;
    }

    height += action_block_h + gap;
    return height + (layout_cfg->panel_inner_padding * 2.0f);
}

static void pane_build_nodes(const MemConsoleState *state,
                             const MemConsoleLayoutConfig *layout_cfg,
                             float pane_height,
                             float split_span,
                             CorePaneNode out_nodes[MEM_CONSOLE_PANE_TREE_NODE_COUNT]) {
    float root_min_ratio = 0.0f;
    float root_max_ratio = 1.0f;
    float right_min_ratio = 0.0f;
    float right_max_ratio = 1.0f;
    float left_min = 220.0f;
    float right_min = 420.0f;
    float detail_min = estimate_right_detail_height(layout_cfg, state ? state->graph_mode_enabled : 0);
    float graph_min = layout_cfg ? (layout_cfg->graph_panel_min_h + 24.0f) : 220.0f;
    float root_ratio = 0.5f;
    float right_ratio = 0.5f;

    if (!out_nodes || !layout_cfg || !state) {
        return;
    }

    pane_ratio_limits_for_span(split_span, left_min, right_min, &root_min_ratio, &root_max_ratio);
    pane_ratio_limits_for_span(pane_height, detail_min, graph_min, &right_min_ratio, &right_max_ratio);

    if (state->pane_left_ratio > 0.0f && state->pane_left_ratio < 1.0f) {
        root_ratio = state->pane_left_ratio;
    } else if (split_span > 1.0f) {
        root_ratio = layout_cfg->left_pane_width / split_span;
    }
    root_ratio = core_pane_clamp_ratio(root_ratio, root_min_ratio, root_max_ratio);

    if (state->pane_right_split_ratio > 0.0f && state->pane_right_split_ratio < 1.0f) {
        right_ratio = state->pane_right_split_ratio;
    } else if (pane_height > 1.0f) {
        right_ratio = estimate_right_detail_height(layout_cfg, state->graph_mode_enabled) / pane_height;
    }
    right_ratio = core_pane_clamp_ratio(right_ratio, right_min_ratio, right_max_ratio);

    out_nodes[MEM_CONSOLE_PANE_TREE_NODE_ROOT] = (CorePaneNode){
        CORE_PANE_NODE_SPLIT,
        100u,
        CORE_PANE_AXIS_HORIZONTAL,
        root_ratio,
        MEM_CONSOLE_PANE_TREE_NODE_LEFT,
        MEM_CONSOLE_PANE_TREE_NODE_RIGHT_SPLIT,
        { left_min, right_min }
    };
    out_nodes[MEM_CONSOLE_PANE_TREE_NODE_LEFT] = (CorePaneNode){
        CORE_PANE_NODE_LEAF,
        MEM_CONSOLE_PANE_LEFT_NAV,
        CORE_PANE_AXIS_HORIZONTAL,
        0.0f,
        0u,
        0u,
        { 0.0f, 0.0f }
    };
    out_nodes[MEM_CONSOLE_PANE_TREE_NODE_RIGHT_SPLIT] = (CorePaneNode){
        CORE_PANE_NODE_SPLIT,
        200u,
        CORE_PANE_AXIS_VERTICAL,
        right_ratio,
        MEM_CONSOLE_PANE_TREE_NODE_DETAIL,
        MEM_CONSOLE_PANE_TREE_NODE_GRAPH,
        { detail_min, graph_min }
    };
    out_nodes[MEM_CONSOLE_PANE_TREE_NODE_DETAIL] = (CorePaneNode){
        CORE_PANE_NODE_LEAF,
        MEM_CONSOLE_PANE_RIGHT_DETAIL,
        CORE_PANE_AXIS_HORIZONTAL,
        0.0f,
        0u,
        0u,
        { 0.0f, 0.0f }
    };
    out_nodes[MEM_CONSOLE_PANE_TREE_NODE_GRAPH] = (CorePaneNode){
        CORE_PANE_NODE_LEAF,
        MEM_CONSOLE_PANE_RIGHT_GRAPH,
        CORE_PANE_AXIS_HORIZONTAL,
        0.0f,
        0u,
        0u,
        { 0.0f, 0.0f }
    };
}

static CoreResult pane_compute_internal(MemConsoleState *state,
                                        const MemConsoleLayoutConfig *layout_cfg,
                                        int frame_width,
                                        int frame_height,
                                        CorePaneNode out_nodes[MEM_CONSOLE_PANE_TREE_NODE_COUNT],
                                        CorePaneRect *out_root_bounds) {
    float outer_margin;
    float pane_gap;
    float pane_height;
    float frame_inner_width;
    float split_span;
    CorePaneLeafRect leaves[3];
    uint32_t leaf_count = 0u;
    CorePaneNode nodes[MEM_CONSOLE_PANE_TREE_NODE_COUNT];
    CorePaneRect root_bounds;
    uint32_t i;
    int have_left = 0;
    int have_detail = 0;
    int have_graph = 0;
    CorePaneRect right_union = {0};

    if (!state || !layout_cfg) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid pane layout compute request" };
    }

    pane_normalize_frame(layout_cfg, &frame_width, &frame_height);

    outer_margin = layout_cfg->outer_margin;
    pane_gap = layout_cfg->pane_gap;
    pane_height = (float)frame_height - (outer_margin * 2.0f);
    frame_inner_width = (float)frame_width - (outer_margin * 2.0f);
    if (frame_inner_width < 320.0f) {
        frame_inner_width = 320.0f;
    }
    split_span = frame_inner_width - pane_gap;
    if (split_span < 280.0f) {
        split_span = 280.0f;
    }

    pane_build_nodes(state, layout_cfg, pane_height, split_span, nodes);

    root_bounds = (CorePaneRect){
        outer_margin,
        outer_margin,
        split_span,
        pane_height
    };

    if (!core_pane_solve(nodes,
                         MEM_CONSOLE_PANE_TREE_NODE_COUNT,
                         MEM_CONSOLE_PANE_TREE_NODE_ROOT,
                         root_bounds,
                         leaves,
                         3u,
                         &leaf_count)) {
        return (CoreResult){ CORE_ERR_FORMAT, "core_pane solve failed" };
    }

    state->pane_left_ratio = nodes[MEM_CONSOLE_PANE_TREE_NODE_ROOT].ratio_01;
    state->pane_right_split_ratio = nodes[MEM_CONSOLE_PANE_TREE_NODE_RIGHT_SPLIT].ratio_01;

    for (i = 0u; i < leaf_count; ++i) {
        KitRenderRect mapped = {
            leaves[i].rect.x,
            leaves[i].rect.y,
            leaves[i].rect.width,
            leaves[i].rect.height
        };

        if (leaves[i].id == MEM_CONSOLE_PANE_RIGHT_DETAIL || leaves[i].id == MEM_CONSOLE_PANE_RIGHT_GRAPH) {
            mapped.x += pane_gap;
        }

        if (leaves[i].id == MEM_CONSOLE_PANE_LEFT_NAV) {
            state->left_pane = mapped;
            have_left = 1;
        } else if (leaves[i].id == MEM_CONSOLE_PANE_RIGHT_DETAIL) {
            state->pane_right_detail = mapped;
            have_detail = 1;
        } else if (leaves[i].id == MEM_CONSOLE_PANE_RIGHT_GRAPH) {
            state->pane_right_graph = mapped;
            have_graph = 1;
        }
    }

    if (!have_left || !have_detail || !have_graph) {
        return (CoreResult){ CORE_ERR_FORMAT, "missing pane leaf from solve" };
    }

    right_union.x = state->pane_right_detail.x;
    right_union.y = state->pane_right_detail.y;
    right_union.width = state->pane_right_detail.width;
    right_union.height = state->pane_right_detail.height;

    if (state->pane_right_graph.y < right_union.y) {
        right_union.y = state->pane_right_graph.y;
    }
    if (state->pane_right_graph.x < right_union.x) {
        right_union.x = state->pane_right_graph.x;
    }
    {
        float x1 = state->pane_right_detail.x + state->pane_right_detail.width;
        float y1 = state->pane_right_detail.y + state->pane_right_detail.height;
        float x2 = state->pane_right_graph.x + state->pane_right_graph.width;
        float y2 = state->pane_right_graph.y + state->pane_right_graph.height;
        float max_x = x1 > x2 ? x1 : x2;
        float max_y = y1 > y2 ? y1 : y2;
        right_union.width = max_x - right_union.x;
        right_union.height = max_y - right_union.y;
    }
    state->right_pane = (KitRenderRect){
        right_union.x,
        right_union.y,
        right_union.width,
        right_union.height
    };

    if (out_nodes) {
        for (i = 0u; i < MEM_CONSOLE_PANE_TREE_NODE_COUNT; ++i) {
            out_nodes[i] = nodes[i];
        }
    }
    if (out_root_bounds) {
        *out_root_bounds = root_bounds;
    }

    return core_result_ok();
}

CoreResult mem_console_pane_layout_compute(MemConsoleState *state,
                                           const MemConsoleLayoutConfig *layout_cfg,
                                           int frame_width,
                                           int frame_height) {
    return pane_compute_internal(state, layout_cfg, frame_width, frame_height, 0, 0);
}

CoreResult mem_console_pane_layout_get(const MemConsoleState *state,
                                       MemConsolePaneId pane_id,
                                       KitRenderRect *out_bounds) {
    if (!state || !out_bounds) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid pane layout get request" };
    }

    switch (pane_id) {
        case MEM_CONSOLE_PANE_LEFT_NAV:
            *out_bounds = state->left_pane;
            break;
        case MEM_CONSOLE_PANE_RIGHT_DETAIL:
            *out_bounds = state->pane_right_detail;
            break;
        case MEM_CONSOLE_PANE_RIGHT_GRAPH:
            *out_bounds = state->pane_right_graph;
            break;
        default:
            return (CoreResult){ CORE_ERR_INVALID_ARG, "unknown pane id" };
    }

    return core_result_ok();
}

CoreResult mem_console_pane_layout_get_splitter_bounds(const MemConsoleState *state,
                                                       const MemConsoleLayoutConfig *layout_cfg,
                                                       MemConsolePaneSplitterId splitter_id,
                                                       KitRenderRect *out_bounds) {
    float horizontal_thickness = 8.0f;

    if (!state || !layout_cfg || !out_bounds) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid splitter bounds request" };
    }

    switch (splitter_id) {
        case MEM_CONSOLE_PANE_SPLITTER_LEFT_RIGHT:
            *out_bounds = (KitRenderRect){
                state->left_pane.x + state->left_pane.width,
                state->left_pane.y,
                layout_cfg->pane_gap,
                state->left_pane.height
            };
            break;
        case MEM_CONSOLE_PANE_SPLITTER_RIGHT_STACK:
            {
                float seam_x = state->left_pane.x + state->left_pane.width;
                float right_edge = state->right_pane.x + state->right_pane.width;
                float seam_w = right_edge - seam_x;
                if (seam_w < state->right_pane.width) {
                    seam_x = state->right_pane.x;
                    seam_w = state->right_pane.width;
                }
            *out_bounds = (KitRenderRect){
                seam_x,
                state->pane_right_detail.y + state->pane_right_detail.height - (horizontal_thickness * 0.5f),
                seam_w,
                horizontal_thickness
            };
            }
            break;
        default:
            return (CoreResult){ CORE_ERR_INVALID_ARG, "unknown splitter id" };
    }

    return core_result_ok();
}

int mem_console_pane_layout_begin_drag(MemConsoleState *state,
                                       const MemConsoleLayoutConfig *layout_cfg,
                                       int frame_width,
                                       int frame_height,
                                       float mouse_x,
                                       float mouse_y) {
    CoreResult result;
    KitRenderRect splitter_bounds;
    MemConsolePaneSplitterId hit_splitter = MEM_CONSOLE_PANE_SPLITTER_NONE;

    if (!state || !layout_cfg) {
        return 0;
    }

    result = mem_console_pane_layout_compute(state, layout_cfg, frame_width, frame_height);
    if (result.code != CORE_OK) {
        return 0;
    }

    result = mem_console_pane_layout_get_splitter_bounds(state,
                                                         layout_cfg,
                                                         MEM_CONSOLE_PANE_SPLITTER_LEFT_RIGHT,
                                                         &splitter_bounds);
    if (result.code == CORE_OK && pane_point_in_rect(splitter_bounds, mouse_x, mouse_y)) {
        hit_splitter = MEM_CONSOLE_PANE_SPLITTER_LEFT_RIGHT;
    } else {
        result = mem_console_pane_layout_get_splitter_bounds(state,
                                                             layout_cfg,
                                                             MEM_CONSOLE_PANE_SPLITTER_RIGHT_STACK,
                                                             &splitter_bounds);
        if (result.code == CORE_OK && pane_point_in_rect(splitter_bounds, mouse_x, mouse_y)) {
            hit_splitter = MEM_CONSOLE_PANE_SPLITTER_RIGHT_STACK;
        }
    }

    if (hit_splitter == MEM_CONSOLE_PANE_SPLITTER_NONE) {
        return 0;
    }

    state->pane_drag_active = 1;
    state->pane_drag_splitter_id = hit_splitter;
    state->pane_drag_anchor_x = mouse_x;
    state->pane_drag_anchor_y = mouse_y;
    state->pane_drag_start_left_ratio = state->pane_left_ratio;
    state->pane_drag_start_right_ratio = state->pane_right_split_ratio;
    return 1;
}

int mem_console_pane_layout_update_drag(MemConsoleState *state,
                                        const MemConsoleLayoutConfig *layout_cfg,
                                        int frame_width,
                                        int frame_height,
                                        float mouse_x,
                                        float mouse_y) {
    CorePaneNode nodes[MEM_CONSOLE_PANE_TREE_NODE_COUNT];
    CorePaneRect root_bounds;
    CorePaneSplitterHit hit = {0};
    float dx;
    float dy;
    float split_span;
    float pane_height;
    float min_ratio;
    float max_ratio;
    int changed = 0;

    if (!state || !layout_cfg || !state->pane_drag_active) {
        return 0;
    }

    pane_normalize_frame(layout_cfg, &frame_width, &frame_height);
    pane_height = (float)frame_height - (layout_cfg->outer_margin * 2.0f);
    split_span = ((float)frame_width - (layout_cfg->outer_margin * 2.0f)) - layout_cfg->pane_gap;
    if (pane_height <= 0.0f || split_span <= 0.0f) {
        return 0;
    }

    (void)pane_compute_internal(state, layout_cfg, frame_width, frame_height, nodes, &root_bounds);

    dx = mouse_x - state->pane_drag_anchor_x;
    dy = mouse_y - state->pane_drag_anchor_y;

    if (state->pane_drag_splitter_id == MEM_CONSOLE_PANE_SPLITTER_LEFT_RIGHT) {
        pane_ratio_limits_for_span(split_span,
                                   nodes[MEM_CONSOLE_PANE_TREE_NODE_ROOT].constraints.min_size_a,
                                   nodes[MEM_CONSOLE_PANE_TREE_NODE_ROOT].constraints.min_size_b,
                                   &min_ratio,
                                   &max_ratio);
        hit.active = true;
        hit.node_index = MEM_CONSOLE_PANE_TREE_NODE_ROOT;
        hit.axis = CORE_PANE_AXIS_HORIZONTAL;
        hit.ratio_01 = state->pane_drag_start_left_ratio;
        hit.parent_span = split_span;
        hit.min_ratio_01 = min_ratio;
        hit.max_ratio_01 = max_ratio;
        nodes[MEM_CONSOLE_PANE_TREE_NODE_ROOT].ratio_01 = state->pane_drag_start_left_ratio;
        changed = core_pane_apply_splitter_drag(nodes,
                                                MEM_CONSOLE_PANE_TREE_NODE_COUNT,
                                                &hit,
                                                dx,
                                                0.0f) ? 1 : 0;
        if (changed) {
            state->pane_left_ratio = nodes[MEM_CONSOLE_PANE_TREE_NODE_ROOT].ratio_01;
        }
    } else if (state->pane_drag_splitter_id == MEM_CONSOLE_PANE_SPLITTER_RIGHT_STACK) {
        pane_ratio_limits_for_span(pane_height,
                                   nodes[MEM_CONSOLE_PANE_TREE_NODE_RIGHT_SPLIT].constraints.min_size_a,
                                   nodes[MEM_CONSOLE_PANE_TREE_NODE_RIGHT_SPLIT].constraints.min_size_b,
                                   &min_ratio,
                                   &max_ratio);
        hit.active = true;
        hit.node_index = MEM_CONSOLE_PANE_TREE_NODE_RIGHT_SPLIT;
        hit.axis = CORE_PANE_AXIS_VERTICAL;
        hit.ratio_01 = state->pane_drag_start_right_ratio;
        hit.parent_span = pane_height;
        hit.min_ratio_01 = min_ratio;
        hit.max_ratio_01 = max_ratio;
        nodes[MEM_CONSOLE_PANE_TREE_NODE_RIGHT_SPLIT].ratio_01 = state->pane_drag_start_right_ratio;
        changed = core_pane_apply_splitter_drag(nodes,
                                                MEM_CONSOLE_PANE_TREE_NODE_COUNT,
                                                &hit,
                                                0.0f,
                                                dy) ? 1 : 0;
        if (changed) {
            state->pane_right_split_ratio = nodes[MEM_CONSOLE_PANE_TREE_NODE_RIGHT_SPLIT].ratio_01;
        }
    }

    (void)root_bounds;
    if (changed) {
        state->pane_prefs_dirty = 1;
        (void)mem_console_pane_layout_compute(state, layout_cfg, frame_width, frame_height);
    }
    return changed;
}

void mem_console_pane_layout_end_drag(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->pane_drag_active = 0;
    state->pane_drag_splitter_id = MEM_CONSOLE_PANE_SPLITTER_NONE;
}
