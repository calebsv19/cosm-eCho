#include "mem_console_ui_graph_internal.h"

#include <math.h>

static int graph_find_root_index(const MemConsoleState *state, uint32_t layout_count) {
    uint32_t i;

    if (!state) {
        return -1;
    }
    for (i = 0u; i < layout_count && i < (uint32_t)state->graph_node_count; ++i) {
        if (state->graph_nodes[i].item_id == state->selected_item_id) {
            return (int)i;
        }
    }
    for (i = 0u; i < layout_count && i < (uint32_t)state->graph_node_count; ++i) {
        if (state->graph_nodes[i].item_id == state->graph_center_item_id) {
            return (int)i;
        }
    }
    return layout_count > 0u ? 0 : -1;
}

static int graph_node_is_emphasized(const MemConsoleState *state, uint32_t node_index) {
    int root_index;
    int edge_index;

    if (!state || node_index >= (uint32_t)state->graph_node_count) {
        return 0;
    }
    if (state->graph_nodes[node_index].item_id == state->selected_item_id ||
        state->graph_nodes[node_index].item_id == state->graph_center_item_id) {
        return 1;
    }

    root_index = graph_find_root_index(state, (uint32_t)state->graph_node_count);
    if (root_index < 0) {
        return 0;
    }

    for (edge_index = 0; edge_index < state->graph_edge_count; ++edge_index) {
        int from_index = state->graph_edges[edge_index].from_index;
        int to_index = state->graph_edges[edge_index].to_index;

        if ((from_index == root_index && to_index == (int)node_index) ||
            (to_index == root_index && from_index == (int)node_index)) {
            return 1;
        }
    }
    return 0;
}

float graph_mode_node_zoom_scale(const MemConsoleState *state, float zoom) {
    float t = zoom;
    float q = 1.0f;
    int view_mode = mem_console_graph_view_mode_get(state);

    if (t <= 0.0f) {
        return 0.0f;
    }

    t = t / 1.6f;
    if (zoom < 1.75f) {
        q = zoom / 1.75f;
        if (q < 0.0f) {
            q = 0.0f;
        }
        if (q > 1.0f) {
            q = 1.0f;
        }
        t *= 0.58f + (0.42f * q * q);
    }

    if (zoom < 1.10f) {
        t *= 0.90f;
    }
    if (zoom < 0.82f) {
        t *= 0.82f;
    }
    if (zoom < 0.58f) {
        t *= 0.74f;
    }
    if (zoom < 0.40f) {
        t *= 0.68f;
    }

    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_PODS) {
        t *= 1.06f;
    } else if (view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        t *= 1.52f;
        if (zoom < 1.05f) {
            t *= 1.10f;
        }
    } else {
        t *= 1.72f;
        if (zoom < 1.08f) {
            t *= 1.12f;
        }
    }

    return t;
}

float graph_mode_min_render_width_px(const MemConsoleState *state) {
    int view_mode = mem_console_graph_view_mode_get(state);

    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        return 1.26f;
    }
    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_FOCUS) {
        return 2.20f;
    }
    return GRAPH_NODE_MIN_RENDER_WIDTH_PX;
}

float graph_mode_min_render_height_px(const MemConsoleState *state) {
    int view_mode = mem_console_graph_view_mode_get(state);

    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        return 1.08f;
    }
    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_FOCUS) {
        return 1.72f;
    }
    return GRAPH_NODE_MIN_RENDER_HEIGHT_PX;
}

float graph_mode_text_min_zoom(const MemConsoleState *state, int emphasized_node) {
    int view_mode = mem_console_graph_view_mode_get(state);

    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_PODS) {
        return emphasized_node ? 1.06f : 1.20f;
    }
    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        return emphasized_node ? 0.90f : 1.00f;
    }
    return emphasized_node ? 0.68f : 0.90f;
}

int graph_node_should_render_text(const MemConsoleState *state,
                                  uint32_t node_index,
                                  float node_w,
                                  float node_h) {
    float min_w = GRAPH_NODE_TEXT_HIDE_WIDTH_PX;
    float min_h = GRAPH_NODE_TEXT_HIDE_HEIGHT_PX;
    int emphasized = graph_node_is_emphasized(state, node_index);
    int view_mode = mem_console_graph_view_mode_get(state);

    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        min_w = emphasized ? 7.0f : 8.4f;
        min_h = emphasized ? 5.8f : 6.6f;
    } else if (view_mode == MEM_CONSOLE_GRAPH_VIEW_FOCUS) {
        min_w = emphasized ? 5.6f : 7.4f;
        min_h = emphasized ? 4.8f : 6.0f;
    }

    if (node_w <= min_w || node_h <= min_h) {
        return 0;
    }
    if (!state) {
        return 0;
    }
    return state->graph_viewport.zoom >= graph_mode_text_min_zoom(state, emphasized) ? 1 : 0;
}
