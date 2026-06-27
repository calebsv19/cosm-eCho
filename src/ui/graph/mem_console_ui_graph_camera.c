#include "mem_console_ui_graph_internal.h"

#include <math.h>

typedef struct GraphCameraWorldPoint {
    float x;
    float y;
} GraphCameraWorldPoint;

/*
 * Graph drawing is transposed for horizontal flow:
 * - screen X maps to viewport pan_y axis
 * - screen Y maps to viewport pan_x axis
 * Keep this mapping centralized so drag/zoom use identical camera math.
 */
static GraphCameraWorldPoint graph_camera_screen_to_world(const KitGraphStructViewport *viewport,
                                                          KitRenderRect graph_bounds,
                                                          float screen_x,
                                                          float screen_y) {
    GraphCameraWorldPoint world = {0.0f, 0.0f};
    float origin_x;
    float origin_y;
    float safe_zoom = 1.0f;

    if (!viewport) {
        return world;
    }

    if (viewport->zoom > 0.0001f) {
        safe_zoom = viewport->zoom;
    }

    origin_x = graph_bounds.x;
    origin_y = graph_bounds.y;

    world.x = (screen_x - origin_x - viewport->pan_y) / safe_zoom;
    world.y = (screen_y - origin_y - viewport->pan_x) / safe_zoom;
    return world;
}

static void graph_camera_world_to_screen(const KitGraphStructViewport *viewport,
                                         KitRenderRect graph_bounds,
                                         GraphCameraWorldPoint world,
                                         float *out_screen_x,
                                         float *out_screen_y) {
    float origin_x;
    float origin_y;
    float safe_zoom = 1.0f;

    if (!viewport || !out_screen_x || !out_screen_y) {
        return;
    }

    if (viewport->zoom > 0.0001f) {
        safe_zoom = viewport->zoom;
    }

    origin_x = graph_bounds.x;
    origin_y = graph_bounds.y;

    *out_screen_x = origin_x + viewport->pan_y + (world.x * safe_zoom);
    *out_screen_y = origin_y + viewport->pan_x + (world.y * safe_zoom);
}

void graph_camera_apply_to_layouts(KitGraphStructNodeLayout *layouts,
                                   uint32_t layout_count,
                                   const MemConsoleState *state,
                                   KitRenderRect graph_bounds) {
    uint32_t i;
    const KitGraphStructViewport *viewport = 0;
    float safe_zoom = 1.0f;
    float node_zoom = 1.0f;
    float min_render_w = GRAPH_NODE_MIN_RENDER_WIDTH_PX;
    float min_render_h = GRAPH_NODE_MIN_RENDER_HEIGHT_PX;

    if (!layouts || !state) {
        return;
    }
    viewport = &state->graph_viewport;
    if (viewport->zoom > 0.0001f) {
        safe_zoom = viewport->zoom;
    }
    node_zoom = graph_mode_node_zoom_scale(state, safe_zoom);
    if (node_zoom > GRAPH_NODE_ZOOM_SIZE_CAP) {
        node_zoom = GRAPH_NODE_ZOOM_SIZE_CAP;
    }
    min_render_w = graph_mode_min_render_width_px(state);
    min_render_h = graph_mode_min_render_height_px(state);

    for (i = 0u; i < layout_count; ++i) {
        float world_x = layouts[i].rect.x;
        float world_y = layouts[i].rect.y;
        float world_w = layouts[i].rect.width;
        float world_h = layouts[i].rect.height;
        float full_scaled_w = world_w * safe_zoom;
        float full_scaled_h = world_h * safe_zoom;
        float capped_scaled_w = world_w * node_zoom;
        float capped_scaled_h = world_h * node_zoom;
        float center_compensate_x = (full_scaled_w - capped_scaled_w) * 0.5f;
        float center_compensate_y = (full_scaled_h - capped_scaled_h) * 0.5f;

        if (capped_scaled_w < min_render_w) {
            capped_scaled_w = min_render_w;
            center_compensate_x = (full_scaled_w - capped_scaled_w) * 0.5f;
        }
        if (capped_scaled_h < min_render_h) {
            capped_scaled_h = min_render_h;
            center_compensate_y = (full_scaled_h - capped_scaled_h) * 0.5f;
        }

        layouts[i].rect.x = graph_bounds.x + viewport->pan_y + (world_x * safe_zoom) + center_compensate_x;
        layouts[i].rect.y = graph_bounds.y + viewport->pan_x + (world_y * safe_zoom) + center_compensate_y;
        layouts[i].rect.width = capped_scaled_w;
        layouts[i].rect.height = capped_scaled_h;
    }
}

static float graph_camera_absf(float value) {
    return value < 0.0f ? -value : value;
}

static void graph_camera_store_live_viewport(KitGraphStructViewport *viewport,
                                             float pan_x,
                                             float pan_y,
                                             float zoom) {
    if (!viewport) {
        return;
    }
    viewport->pan_x = pan_x;
    viewport->pan_y = pan_y;
    viewport->zoom = zoom;
}

static int graph_camera_viewport_is_default_focus_reset(const KitGraphStructViewport *viewport) {
    if (!viewport) {
        return 0;
    }
    return graph_camera_absf(viewport->pan_x) <= 0.01f &&
           graph_camera_absf(viewport->pan_y) <= 0.01f &&
           graph_camera_absf(viewport->zoom - 1.14f) <= 0.015f;
}

static int graph_camera_layout_index_for_state_node(const KitGraphStructNodeLayout *layouts,
                                                    uint32_t layout_count,
                                                    int state_node_index) {
    uint32_t target_node_id;
    uint32_t i;

    if (!layouts || state_node_index < 0) {
        return -1;
    }
    target_node_id = (uint32_t)state_node_index + 1u;
    for (i = 0u; i < layout_count; ++i) {
        if (layouts[i].node_id == target_node_id) {
            return (int)i;
        }
    }
    return -1;
}

static int graph_camera_focus_root_state_index(const MemConsoleState *state,
                                               uint32_t layout_count) {
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

static int graph_camera_edge_touches_state_index(const KitGraphStructEdge *edges,
                                                 uint32_t edge_count,
                                                 int root_state_index,
                                                 int probe_state_index) {
    uint32_t i;

    if (!edges || root_state_index < 0 || probe_state_index < 0) {
        return 0;
    }
    for (i = 0u; i < edge_count; ++i) {
        int from_index = (int)edges[i].from_id - 1;
        int to_index = (int)edges[i].to_id - 1;
        if ((from_index == root_state_index && to_index == probe_state_index) ||
            (from_index == probe_state_index && to_index == root_state_index)) {
            return 1;
        }
    }
    return 0;
}

void graph_camera_apply_focus_initial_fit(MemConsoleState *state,
                                          KitRenderRect graph_bounds,
                                          const KitGraphStructEdge *edges,
                                          uint32_t edge_count,
                                          const KitGraphStructNodeLayout *layouts,
                                          uint32_t layout_count) {
    int root_state_index;
    int root_layout_index;
    float min_x;
    float min_y;
    float max_x;
    float max_y;
    float fit_w;
    float fit_h;
    float margin_x;
    float margin_y;
    float zoom_x;
    float zoom_y;
    float next_zoom;
    float world_center_x;
    float world_center_y;
    float target_screen_x;
    float target_screen_y;
    uint32_t i;
    int included_count = 0;

    if (!state || !edges || !layouts || layout_count == 0u) {
        return;
    }
    if (mem_console_graph_view_mode_get(state) != MEM_CONSOLE_GRAPH_VIEW_FOCUS) {
        return;
    }
    if (!graph_camera_viewport_is_default_focus_reset(&state->graph_viewport)) {
        return;
    }

    root_state_index = graph_camera_focus_root_state_index(state, layout_count);
    root_layout_index = graph_camera_layout_index_for_state_node(layouts, layout_count, root_state_index);
    if (root_layout_index < 0) {
        return;
    }

    min_x = layouts[root_layout_index].rect.x;
    min_y = layouts[root_layout_index].rect.y;
    max_x = layouts[root_layout_index].rect.x + layouts[root_layout_index].rect.width;
    max_y = layouts[root_layout_index].rect.y + layouts[root_layout_index].rect.height;
    included_count = 1;

    for (i = 0u; i < layout_count && i < (uint32_t)state->graph_node_count; ++i) {
        int layout_index;
        KitRenderRect rect;

        if ((int)i == root_state_index) {
            continue;
        }
        if (!graph_camera_edge_touches_state_index(edges, edge_count, root_state_index, (int)i)) {
            continue;
        }

        layout_index = graph_camera_layout_index_for_state_node(layouts, layout_count, (int)i);
        if (layout_index < 0) {
            continue;
        }
        rect = layouts[layout_index].rect;
        if (rect.x < min_x) min_x = rect.x;
        if (rect.y < min_y) min_y = rect.y;
        if (rect.x + rect.width > max_x) max_x = rect.x + rect.width;
        if (rect.y + rect.height > max_y) max_y = rect.y + rect.height;
        included_count += 1;
    }

    if (included_count <= 1) {
        return;
    }

    margin_x = graph_bounds.width * 0.12f;
    margin_y = graph_bounds.height * 0.16f;
    if (margin_x < 72.0f) {
        margin_x = 72.0f;
    }
    if (margin_y < 64.0f) {
        margin_y = 64.0f;
    }
    fit_w = max_x - min_x;
    fit_h = max_y - min_y;
    if (fit_w < 1.0f || fit_h < 1.0f ||
        graph_bounds.width <= margin_x * 2.0f ||
        graph_bounds.height <= margin_y * 2.0f) {
        return;
    }

    zoom_x = (graph_bounds.width - (margin_x * 2.0f)) / fit_w;
    zoom_y = (graph_bounds.height - (margin_y * 2.0f)) / fit_h;
    next_zoom = zoom_x < zoom_y ? zoom_x : zoom_y;
    next_zoom = graph_clampf(next_zoom, 1.24f, 2.20f);
    world_center_x = (min_x + max_x) * 0.5f;
    world_center_y = (min_y + max_y) * 0.5f;
    target_screen_x = graph_bounds.x + (graph_bounds.width * 0.50f);
    target_screen_y = graph_bounds.y + (graph_bounds.height * 0.46f);

    graph_camera_store_live_viewport(&state->graph_viewport,
                                     target_screen_y - graph_bounds.y - (world_center_y * next_zoom),
                                     target_screen_x - graph_bounds.x - (world_center_x * next_zoom),
                                     next_zoom);
}

static void graph_camera_pan_viewport_by_screen_delta(KitGraphStructViewport *viewport,
                                                      float delta_x,
                                                      float delta_y) {
    if (!viewport) {
        return;
    }
    viewport->pan_x += delta_y;
    viewport->pan_y += delta_x;
}

void graph_camera_pan_live_viewport_by_screen_delta(MemConsoleState *state,
                                                    float delta_x,
                                                    float delta_y) {
    if (!state) {
        return;
    }
    graph_camera_pan_viewport_by_screen_delta(&state->graph_viewport, delta_x, delta_y);
}

static void graph_camera_zoom_anchor_at_screen_point(KitGraphStructViewport *viewport,
                                                     KitRenderRect graph_bounds,
                                                     float screen_x,
                                                     float screen_y,
                                                     float zoom_factor,
                                                     int steps) {
    GraphCameraWorldPoint anchor_world;
    float anchored_screen_x = 0.0f;
    float anchored_screen_y = 0.0f;
    int i;

    if (!viewport || steps <= 0) {
        return;
    }

    anchor_world = graph_camera_screen_to_world(viewport, graph_bounds, screen_x, screen_y);

    for (i = 0; i < steps; ++i) {
        (void)kit_graph_struct_viewport_zoom_by(viewport,
                                                zoom_factor,
                                                0.02f,
                                                48.0f);
    }

    /*
     * Reposition pan so the same world point remains under the same cursor pixel.
     */
    graph_camera_world_to_screen(viewport,
                                 graph_bounds,
                                 anchor_world,
                                 &anchored_screen_x,
                                 &anchored_screen_y);
    graph_camera_pan_viewport_by_screen_delta(viewport,
                                              screen_x - anchored_screen_x,
                                              screen_y - anchored_screen_y);
}

static void graph_shift_cached_layout_for_pan(MemConsoleState *state,
                                              float delta_x,
                                              float delta_y) {
    uint32_t i;

    if (!state || !state->graph_layout_valid) {
        return;
    }

    for (i = 0u; i < state->graph_layout_node_count; ++i) {
        state->graph_layout_node_layouts[i].rect.x += delta_x;
        state->graph_layout_node_layouts[i].rect.y += delta_y;
    }

    for (i = 0u; i < state->graph_layout_edge_count; ++i) {
        KitGraphStructEdgeRoute *route = &state->graph_layout_edge_routes[i];
        uint32_t p;
        for (p = 0u; p < route->point_count; ++p) {
            route->points[p].x += delta_x;
            route->points[p].y += delta_y;
        }
        state->graph_layout_edge_label_layouts[i].rect.x += delta_x;
        state->graph_layout_edge_label_layouts[i].rect.y += delta_y;
        state->graph_layout_edge_label_layouts[i].anchor.x += delta_x;
        state->graph_layout_edge_label_layouts[i].anchor.y += delta_y;
    }
}

int mem_console_ui_graph_handle_viewport_interaction(MemConsoleState *state,
                                                     const KitUiInputState *input,
                                                     int wheel_y,
                                                     KitRenderRect graph_bounds) {
    int hovered;
    int suppress_release_click = 0;

    if (!state || !input) {
        return 0;
    }

    hovered = kit_ui_point_in_rect(graph_bounds, input->mouse_x, input->mouse_y);

    if (wheel_y != 0 && hovered) {
        int steps = wheel_y > 0 ? wheel_y : -wheel_y;
        float zoom_factor = wheel_y > 0 ? 1.12f : (1.0f / 1.12f);

        graph_camera_zoom_anchor_at_screen_point(&state->graph_viewport,
                                                 graph_bounds,
                                                 input->mouse_x,
                                                 input->mouse_y,
                                                 zoom_factor,
                                                 steps);
        state->graph_layout_valid = 0;
    }

    if (input->mouse_pressed) {
        if (hovered) {
            state->graph_click_armed = 1;
            state->graph_drag_active = 1;
            state->graph_drag_moved = 0;
            state->graph_drag_last_x = input->mouse_x;
            state->graph_drag_last_y = input->mouse_y;
        } else {
            state->graph_click_armed = 0;
            state->graph_drag_active = 0;
            state->graph_drag_moved = 0;
        }
    }

    if (state->graph_drag_active && input->mouse_down) {
        float delta_x = input->mouse_x - state->graph_drag_last_x;
        float delta_y = input->mouse_y - state->graph_drag_last_y;
        if (delta_x != 0.0f || delta_y != 0.0f) {
            float abs_dx = delta_x < 0.0f ? -delta_x : delta_x;
            float abs_dy = delta_y < 0.0f ? -delta_y : delta_y;
            graph_shift_cached_layout_for_pan(state, delta_x, delta_y);
            graph_camera_pan_live_viewport_by_screen_delta(state, delta_x, delta_y);
            state->graph_drag_last_x = input->mouse_x;
            state->graph_drag_last_y = input->mouse_y;
            if (!state->graph_drag_moved && (abs_dx >= 0.5f || abs_dy >= 0.5f)) {
                state->graph_drag_moved = 1;
            }
        }
    }

    if (state->graph_drag_active && input->mouse_released) {
        suppress_release_click = state->graph_drag_moved ? 1 : 0;
        state->graph_drag_active = 0;
        state->graph_drag_moved = 0;
    }

    if (!input->mouse_down && !input->mouse_pressed && !input->mouse_released && !hovered) {
        state->graph_click_armed = 0;
        state->graph_drag_active = 0;
        state->graph_drag_moved = 0;
    }

    return suppress_release_click;
}
