#include "mem_console_ui_graph_internal.h"

#include <math.h>

typedef struct GraphCameraWorldPoint {
    float x;
    float y;
} GraphCameraWorldPoint;

static float graph_camera_node_zoom_scale(float zoom) {
    float t = zoom;
    float q = 1.0f;

    if (t <= 0.0f) {
        return 0.0f;
    }

    /* Global sizing baseline: roughly 1/1.6 of raw zoom scale. */
    t = t / 1.6f;

    /*
     * Heavy collapse starts around 1.75x and increases as zoom moves away from
     * close-in program view.
     */
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
    return t;
}

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
                                   const KitGraphStructViewport *viewport,
                                   KitRenderRect graph_bounds) {
    uint32_t i;
    float safe_zoom = 1.0f;
    float node_zoom = 1.0f;

    if (!layouts || !viewport) {
        return;
    }
    if (viewport->zoom > 0.0001f) {
        safe_zoom = viewport->zoom;
    }
    node_zoom = graph_camera_node_zoom_scale(safe_zoom);
    if (node_zoom > GRAPH_NODE_ZOOM_SIZE_CAP) {
        node_zoom = GRAPH_NODE_ZOOM_SIZE_CAP;
    }

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

        if (capped_scaled_w < GRAPH_NODE_MIN_RENDER_WIDTH_PX) {
            capped_scaled_w = GRAPH_NODE_MIN_RENDER_WIDTH_PX;
            center_compensate_x = (full_scaled_w - capped_scaled_w) * 0.5f;
        }
        if (capped_scaled_h < GRAPH_NODE_MIN_RENDER_HEIGHT_PX) {
            capped_scaled_h = GRAPH_NODE_MIN_RENDER_HEIGHT_PX;
            center_compensate_y = (full_scaled_h - capped_scaled_h) * 0.5f;
        }

        layouts[i].rect.x = graph_bounds.x + viewport->pan_y + (world_x * safe_zoom) + center_compensate_x;
        layouts[i].rect.y = graph_bounds.y + viewport->pan_x + (world_y * safe_zoom) + center_compensate_y;
        layouts[i].rect.width = capped_scaled_w;
        layouts[i].rect.height = capped_scaled_h;
    }
}

void graph_camera_pan_by_screen_delta(KitGraphStructViewport *viewport,
                                      float delta_x,
                                      float delta_y) {
    if (!viewport) {
        return;
    }
    viewport->pan_x += delta_y;
    viewport->pan_y += delta_x;
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
    graph_camera_pan_by_screen_delta(viewport,
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
            graph_camera_pan_by_screen_delta(&state->graph_viewport, delta_x, delta_y);
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
