#include "mem_console_ui_graph_internal.h"

#include <math.h>
#include <string.h>

static float graph_layout_center_x(const KitGraphStructNodeLayout *layout) {
    if (!layout) {
        return 0.0f;
    }
    return layout->rect.x + (layout->rect.width * 0.5f);
}

static float graph_layout_center_y(const KitGraphStructNodeLayout *layout) {
    if (!layout) {
        return 0.0f;
    }
    return layout->rect.y + (layout->rect.height * 0.5f);
}

static void graph_layout_set_center(KitRenderRect bounds,
                                    KitGraphStructNodeLayout *layout,
                                    float center_x,
                                    float center_y) {
    float min_center_x;
    float max_center_x;
    float min_center_y;
    float max_center_y;

    if (!layout) {
        return;
    }

    min_center_x = bounds.x + 6.0f + (layout->rect.width * 0.5f);
    max_center_x = bounds.x + bounds.width - 6.0f - (layout->rect.width * 0.5f);
    min_center_y = bounds.y + 6.0f + (layout->rect.height * 0.5f);
    max_center_y = bounds.y + bounds.height - 6.0f - (layout->rect.height * 0.5f);
    if (max_center_x < min_center_x) {
        max_center_x = min_center_x;
    }
    if (max_center_y < min_center_y) {
        max_center_y = min_center_y;
    }

    center_x = graph_clampf(center_x, min_center_x, max_center_x);
    center_y = graph_clampf(center_y, min_center_y, max_center_y);
    layout->rect.x = center_x - (layout->rect.width * 0.5f);
    layout->rect.y = center_y - (layout->rect.height * 0.5f);
}

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
        return 1.48f;
    }
    return GRAPH_NODE_MIN_RENDER_WIDTH_PX;
}

float graph_mode_min_render_height_px(const MemConsoleState *state) {
    int view_mode = mem_console_graph_view_mode_get(state);

    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        return 1.08f;
    }
    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_FOCUS) {
        return 1.24f;
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
    return emphasized_node ? 0.82f : 0.92f;
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
        min_w = emphasized ? 6.4f : 7.4f;
        min_h = emphasized ? 5.4f : 6.0f;
    }

    if (node_w <= min_w || node_h <= min_h) {
        return 0;
    }
    if (!state) {
        return 0;
    }
    return state->graph_viewport.zoom >= graph_mode_text_min_zoom(state, emphasized) ? 1 : 0;
}

static void graph_sort_indices_by_center_y(const KitGraphStructNodeLayout *layouts,
                                           const int *depth,
                                           int *indices,
                                           int count) {
    int i;

    if (!layouts || !indices || count <= 1) {
        return;
    }

    for (i = 1; i < count; ++i) {
        int key = indices[i];
        int j = i - 1;
        while (j >= 0) {
            int probe = indices[j];
            float probe_y = graph_layout_center_y(&layouts[probe]);
            float key_y = graph_layout_center_y(&layouts[key]);
            int should_shift = 0;

            if (probe_y > key_y) {
                should_shift = 1;
            } else if (probe_y == key_y && depth && depth[probe] > depth[key]) {
                should_shift = 1;
            } else if (probe_y == key_y && (!depth || depth[probe] == depth[key]) && probe > key) {
                should_shift = 1;
            }
            if (!should_shift) {
                break;
            }
            indices[j + 1] = probe;
            j -= 1;
        }
        indices[j + 1] = key;
    }
}

static void graph_sort_component_indices_by_size(const int *component_sizes,
                                                 const int *component_has_root,
                                                 int *component_indices,
                                                 int count) {
    int i;

    if (!component_indices || count <= 1) {
        return;
    }

    for (i = 1; i < count; ++i) {
        int key = component_indices[i];
        int j = i - 1;

        while (j >= 0) {
            int probe = component_indices[j];
            int should_shift = 0;

            if (component_has_root && component_has_root[probe] < component_has_root[key]) {
                should_shift = 1;
            } else if (component_has_root &&
                       component_has_root[probe] == component_has_root[key] &&
                       component_sizes[probe] < component_sizes[key]) {
                should_shift = 1;
            } else if ((!component_has_root || component_has_root[probe] == component_has_root[key]) &&
                       component_sizes[probe] == component_sizes[key] &&
                       probe > key) {
                should_shift = 1;
            }
            if (!should_shift) {
                break;
            }

            component_indices[j + 1] = probe;
            j -= 1;
        }
        component_indices[j + 1] = key;
    }
}

void apply_free_web_layout(KitRenderRect bounds,
                           const MemConsoleState *state,
                           const KitGraphStructEdge *edges,
                           uint32_t edge_count,
                           KitGraphStructNodeLayout *layouts,
                           uint32_t layout_count) {
    int node_count;
    int depth[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int component_of[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int component_sizes[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int component_has_root[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int component_initialized[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int component_indices[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float component_min_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float component_min_y[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float component_max_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float component_max_y[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int queue[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float target_center_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float target_center_y[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int root_index;
    int root_component = 0;
    int component_count = 0;
    float min_x;
    float min_y;
    float max_x;
    float max_y;
    float root_x;
    float root_y;
    float aspect = 1.0f;
    float x_scale = 1.0f;
    float y_scale = 1.0f;
    float density = 0.0f;
    const float full_turn = 6.28318530718f;
    const float quarter_turn = full_turn * 0.25f;
    int max_depth = 0;
    int i;
    int pass;

    if (!state || !edges || !layouts || layout_count == 0u) {
        return;
    }

    node_count = state->graph_node_count;
    if (node_count <= 0) {
        return;
    }
    if ((uint32_t)node_count > layout_count) {
        node_count = (int)layout_count;
    }
    if (node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }

    root_index = graph_find_root_index(state, (uint32_t)node_count);
    if (root_index < 0) {
        root_index = 0;
    }

    min_x = layouts[0].rect.x;
    min_y = layouts[0].rect.y;
    max_x = layouts[0].rect.x + layouts[0].rect.width;
    max_y = layouts[0].rect.y + layouts[0].rect.height;
    for (i = 0; i < node_count; ++i) {
        float x0 = layouts[i].rect.x;
        float y0 = layouts[i].rect.y;
        float x1 = x0 + layouts[i].rect.width;
        float y1 = y0 + layouts[i].rect.height;

        if (x0 < min_x) min_x = x0;
        if (y0 < min_y) min_y = y0;
        if (x1 > max_x) max_x = x1;
        if (y1 > max_y) max_y = y1;
    }
    if ((max_y - min_y) > 1.0f) {
        aspect = (max_x - min_x) / (max_y - min_y);
    }
    if (aspect > 1.65f) {
        float compress = graph_clampf((aspect - 1.65f) / 3.4f, 0.0f, 1.0f);
        x_scale = 1.0f - (0.14f * compress);
        y_scale = 1.0f + (0.10f * compress);
    }
    density = (float)edge_count / (float)node_count;

    for (i = 0; i < node_count; ++i) {
        depth[i] = -1;
        component_of[i] = -1;
        target_center_x[i] = graph_layout_center_x(&layouts[i]);
        target_center_y[i] = graph_layout_center_y(&layouts[i]);
    }

    for (i = 0; i < node_count; ++i) {
        int queue_read;
        int queue_write;
        int component_id;

        if (component_of[i] >= 0) {
            continue;
        }

        component_id = component_count;
        component_count += 1;
        component_of[i] = component_id;
        component_sizes[component_id] = 0;
        component_has_root[component_id] = 0;
        queue[0] = i;
        queue_read = 0;
        queue_write = 1;

        while (queue_read < queue_write) {
            int current_index = queue[queue_read++];

            component_sizes[component_id] += 1;
            if (current_index == root_index) {
                component_has_root[component_id] = 1;
                root_component = component_id;
            }

            for (pass = 0; (uint32_t)pass < edge_count; ++pass) {
                int from_index = (int)edges[pass].from_id - 1;
                int to_index = (int)edges[pass].to_id - 1;
                int next_index = -1;

                if (from_index < 0 || to_index < 0 ||
                    from_index >= node_count || to_index >= node_count) {
                    continue;
                }
                if (from_index == current_index) {
                    next_index = to_index;
                } else if (to_index == current_index) {
                    next_index = from_index;
                }
                if (next_index < 0 || component_of[next_index] >= 0) {
                    continue;
                }
                component_of[next_index] = component_id;
                if (queue_write < MEM_CONSOLE_GRAPH_NODE_LIMIT) {
                    queue[queue_write++] = next_index;
                }
            }
        }
    }

    root_x = target_center_x[root_index];
    root_y = target_center_y[root_index];
    depth[root_index] = 0;
    queue[0] = root_index;
    {
        int queue_read = 0;
        int queue_write = 1;
        while (queue_read < queue_write) {
            int current_index = queue[queue_read++];
            int current_depth = depth[current_index];
            for (i = 0; (uint32_t)i < edge_count; ++i) {
                int from_index = (int)edges[i].from_id - 1;
                int to_index = (int)edges[i].to_id - 1;
                int next_index = -1;

                if (from_index < 0 || to_index < 0 ||
                    from_index >= node_count || to_index >= node_count) {
                    continue;
                }
                if (from_index == current_index) {
                    next_index = to_index;
                } else if (to_index == current_index) {
                    next_index = from_index;
                }
                if (next_index < 0 || depth[next_index] >= 0) {
                    continue;
                }
                depth[next_index] = current_depth + 1;
                if (depth[next_index] > max_depth) {
                    max_depth = depth[next_index];
                }
                if (queue_write < MEM_CONSOLE_GRAPH_NODE_LIMIT) {
                    queue[queue_write++] = next_index;
                }
            }
        }
    }

    for (i = 0; i < node_count; ++i) {
        float dx = target_center_x[i] - root_x;
        float dy = target_center_y[i] - root_y;

        target_center_x[i] = root_x + (dx * x_scale);
        target_center_y[i] = root_y + (dy * y_scale);
    }

    for (pass = 0; pass <= max_depth + 1; ++pass) {
        int level = pass <= max_depth ? pass : -1;
        int indices[MEM_CONSOLE_GRAPH_NODE_LIMIT];
        int count = 0;
        int slot;
        float band_span;

        for (i = 0; i < node_count; ++i) {
            if (depth[i] == level) {
                indices[count++] = i;
            }
        }
        if (count <= 0) {
            continue;
        }

        graph_sort_indices_by_center_y(layouts, depth, indices, count);
        band_span = 32.0f + ((float)(count - 1) * (20.0f - (graph_clampf(density, 0.0f, 4.0f) * 1.0f)));
        if (band_span > bounds.height * 0.72f) {
            band_span = bounds.height * 0.72f;
        }

        for (slot = 0; slot < count; ++slot) {
            int node_index = indices[slot];
            float slot_t = count <= 1 ? 0.0f : ((float)slot / (float)(count - 1)) - 0.5f;
            float depth_blend = level < 0 ? 0.22f : (level == 0 ? 0.0f : 0.44f);
            float target_y = root_y + (slot_t * band_span);

            if (level == 1) {
                target_center_x[node_index] = root_x + ((target_center_x[node_index] - root_x) * 0.92f);
            } else if (level == 2) {
                target_center_x[node_index] = root_x + ((target_center_x[node_index] - root_x) * 0.96f);
            }
            if (level >= 0) {
                target_center_y[node_index] = (target_center_y[node_index] * (1.0f - depth_blend)) +
                                              (target_y * depth_blend);
            } else {
                target_center_y[node_index] = (target_center_y[node_index] * 0.72f) +
                                              (target_y * 0.28f);
            }
        }
    }

    for (i = 0; i < component_count; ++i) {
        component_indices[i] = i;
        component_initialized[i] = 0;
        component_min_x[i] = 0.0f;
        component_min_y[i] = 0.0f;
        component_max_x[i] = 0.0f;
        component_max_y[i] = 0.0f;
    }
    graph_sort_component_indices_by_size(component_sizes,
                                         component_has_root,
                                         component_indices,
                                         component_count);

    for (i = 0; i < node_count; ++i) {
        int component_id = component_of[i];
        float x0 = target_center_x[i] - (layouts[i].rect.width * 0.5f);
        float y0 = target_center_y[i] - (layouts[i].rect.height * 0.5f);
        float x1 = target_center_x[i] + (layouts[i].rect.width * 0.5f);
        float y1 = target_center_y[i] + (layouts[i].rect.height * 0.5f);

        if (!component_initialized[component_id]) {
            component_min_x[component_id] = x0;
            component_min_y[component_id] = y0;
            component_max_x[component_id] = x1;
            component_max_y[component_id] = y1;
            component_initialized[component_id] = 1;
            continue;
        }
        if (x0 < component_min_x[component_id]) component_min_x[component_id] = x0;
        if (y0 < component_min_y[component_id]) component_min_y[component_id] = y0;
        if (x1 > component_max_x[component_id]) component_max_x[component_id] = x1;
        if (y1 > component_max_y[component_id]) component_max_y[component_id] = y1;
    }

    for (i = 0; i < component_count; ++i) {
        int component_id = component_indices[i];

        if (component_id == root_component) {
            continue;
        }

        {
            int rank = i - 1;
            int secondary_count = component_count - 1;
            float component_center_x = (component_min_x[component_id] + component_max_x[component_id]) * 0.5f;
            float component_center_y = (component_min_y[component_id] + component_max_y[component_id]) * 0.5f;
            float component_width = component_max_x[component_id] - component_min_x[component_id];
            float component_height = component_max_y[component_id] - component_min_y[component_id];
            float angle = secondary_count <= 1
                              ? -quarter_turn
                              : (-quarter_turn + (((float)rank / (float)secondary_count) * full_turn));
            float orbit_x = (bounds.width * 0.30f) + (component_width * 0.10f);
            float orbit_y = (bounds.height * 0.24f) + (component_height * 0.12f);
            float anchor_x = (bounds.x + (bounds.width * 0.5f)) + (cosf(angle) * orbit_x);
            float anchor_y = (bounds.y + (bounds.height * 0.5f)) + (sinf(angle) * orbit_y);
            float shift_x = anchor_x - component_center_x;
            float shift_y = anchor_y - component_center_y;
            int node_index;

            for (node_index = 0; node_index < node_count; ++node_index) {
                if (component_of[node_index] != component_id) {
                    continue;
                }
                target_center_x[node_index] += shift_x;
                target_center_y[node_index] += shift_y;
            }
        }
    }

    target_center_x[root_index] = bounds.x + (bounds.width * 0.5f);
    target_center_y[root_index] = bounds.y + (bounds.height * 0.5f);

    for (i = 0; i < node_count; ++i) {
        graph_layout_set_center(bounds,
                                &layouts[i],
                                target_center_x[i],
                                target_center_y[i]);
        target_center_x[i] = graph_layout_center_x(&layouts[i]);
        target_center_y[i] = graph_layout_center_y(&layouts[i]);
    }

    for (pass = 0; pass < 4; ++pass) {
        int a;
        int b;

        for (a = 0; a < node_count; ++a) {
            for (b = a + 1; b < node_count; ++b) {
                float dx = target_center_x[a] - target_center_x[b];
                float dy = target_center_y[a] - target_center_y[b];
                float min_dx = (layouts[a].rect.width + layouts[b].rect.width) * 0.5f + 20.0f;
                float min_dy = (layouts[a].rect.height + layouts[b].rect.height) * 0.5f + 16.0f;

                if (fabsf(dx) >= min_dx || fabsf(dy) >= min_dy) {
                    continue;
                }

                if (fabsf(dy) < min_dy) {
                    float shift_y = (min_dy - fabsf(dy)) * 0.28f;
                    float dir_y = dy >= 0.0f ? 1.0f : -1.0f;
                    if (a != root_index) {
                        target_center_y[a] += shift_y * dir_y;
                    }
                    if (b != root_index) {
                        target_center_y[b] -= shift_y * dir_y;
                    }
                }
                if (fabsf(dx) < min_dx) {
                    float shift_x = (min_dx - fabsf(dx)) * 0.12f;
                    float dir_x = dx >= 0.0f ? 1.0f : -1.0f;
                    if (a != root_index) {
                        target_center_x[a] += shift_x * dir_x;
                    }
                    if (b != root_index) {
                        target_center_x[b] -= shift_x * dir_x;
                    }
                }
            }
        }

        for (i = 0; i < node_count; ++i) {
            graph_layout_set_center(bounds,
                                    &layouts[i],
                                    target_center_x[i],
                                    target_center_y[i]);
            target_center_x[i] = graph_layout_center_x(&layouts[i]);
            target_center_y[i] = graph_layout_center_y(&layouts[i]);
        }
        target_center_x[root_index] = bounds.x + (bounds.width * 0.5f);
        target_center_y[root_index] = bounds.y + (bounds.height * 0.5f);
        graph_layout_set_center(bounds,
                                &layouts[root_index],
                                target_center_x[root_index],
                                target_center_y[root_index]);
        target_center_x[root_index] = graph_layout_center_x(&layouts[root_index]);
        target_center_y[root_index] = graph_layout_center_y(&layouts[root_index]);
    }
}
