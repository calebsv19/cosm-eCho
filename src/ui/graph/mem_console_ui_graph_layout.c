#include "mem_console_ui_graph_internal.h"

#include <math.h>
#include <string.h>

static void configure_graph_layout_style(KitGraphStructLayoutStyle *style) {
    if (!style) {
        return;
    }
    kit_graph_struct_layout_style_default(style);
    style->padding = 12.0f;
    style->level_gap = 92.0f;
    style->sibling_gap = 40.0f;
    style->node_width = 29.3f;
    style->node_height = 12.0f;
    style->node_min_width = 20.0f;
    style->node_max_width = 52.0f;
    style->node_padding_x = 2.0f;
    style->label_char_width = 3.5f;
    style->node_label_font_role = CORE_FONT_ROLE_UI_REGULAR;
    style->node_label_text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
    style->measure_text_fn = 0;
    style->measure_text_user = 0;
    style->edge_label_padding_x = 4.0f;
    style->edge_label_height = 13.0f;
    style->edge_label_lane_gap = 2.0f;
}

static void transpose_layouts_to_horizontal_flow(KitRenderRect bounds,
                                                 KitGraphStructNodeLayout *layouts,
                                                 uint32_t layout_count) {
    uint32_t i;

    if (!layouts) {
        return;
    }

    for (i = 0u; i < layout_count; ++i) {
        float old_x = layouts[i].rect.x;
        float old_y = layouts[i].rect.y;
        layouts[i].rect.x = bounds.x + (old_y - bounds.y);
        layouts[i].rect.y = bounds.y + (old_x - bounds.x);
    }
}

static uint32_t focus_hash_u32(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

static float focus_hash_signed(uint32_t seed) {
    uint32_t hash = focus_hash_u32(seed);
    float unit = (float)(hash & 0x00ffffffu) / 16777215.0f;
    return (unit * 2.0f) - 1.0f;
}

static float focus_layout_center_x(const KitGraphStructNodeLayout *layout) {
    if (!layout) {
        return 0.0f;
    }
    return layout->rect.x + (layout->rect.width * 0.5f);
}

static float focus_layout_center_y(const KitGraphStructNodeLayout *layout) {
    if (!layout) {
        return 0.0f;
    }
    return layout->rect.y + (layout->rect.height * 0.5f);
}

static void focus_layout_set_center(KitRenderRect bounds,
                                    KitGraphStructNodeLayout *layout,
                                    float center_x,
                                    float center_y,
                                    float min_y) {
    float min_center_x;
    float max_center_x;
    float min_center_y;
    float max_center_y;

    if (!layout) {
        return;
    }

    min_center_x = bounds.x + 2.0f + (layout->rect.width * 0.5f);
    max_center_x = bounds.x + bounds.width - 2.0f - (layout->rect.width * 0.5f);
    min_center_y = min_y + (layout->rect.height * 0.5f);
    max_center_y = bounds.y + bounds.height - 2.0f - (layout->rect.height * 0.5f);
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

static void focus_layout_sort_indices_by_score(int *indices,
                                               int count,
                                               const float *scores) {
    int i;

    if (!indices || !scores || count <= 1) {
        return;
    }

    for (i = 1; i < count; ++i) {
        int key = indices[i];
        float key_score = scores[key];
        int j = i - 1;
        while (j >= 0 && scores[indices[j]] > key_score) {
            indices[j + 1] = indices[j];
            j -= 1;
        }
        indices[j + 1] = key;
    }
}

static void apply_focus_anchor_priority_layout(KitRenderRect bounds,
                                               const MemConsoleState *state,
                                               const KitGraphStructEdge *edges,
                                               uint32_t edge_count,
                                               KitGraphStructNodeLayout *layouts,
                                               uint32_t layout_count) {
    enum {
        FOCUS_LEVEL_MAX = 16
    };
    int node_count = 0;
    int depth[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int degree[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float neighbor_sum_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int neighbor_count[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float score_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float target_center_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float target_center_y[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float center_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float center_y[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int level_members[FOCUS_LEVEL_MAX + 1][MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int level_counts[FOCUS_LEVEL_MAX + 1];
    int anchor_by_role[5] = { -1, -1, -1, -1, -1 };
    int is_anchor[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int selected_index = -1;
    float selected_center_x = 0.0f;
    float edge_density = 0.0f;
    float crowd = 0.0f;
    int i;
    int pass;

    if (!state || !edges || !layouts) {
        return;
    }

    node_count = state->graph_node_count;
    if (node_count < 0) {
        return;
    }
    if ((uint32_t)node_count > layout_count) {
        node_count = (int)layout_count;
    }
    if (node_count <= 0 || node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        return;
    }
    edge_density = (float)edge_count / (float)node_count;
    crowd = graph_clampf((edge_density - 1.1f) / 2.8f, 0.0f, 1.0f);

    memset(level_members, 0, sizeof(level_members));
    memset(level_counts, 0, sizeof(level_counts));
    memset(is_anchor, 0, sizeof(is_anchor));
    for (i = 0; i < node_count; ++i) {
        GraphBucketRole role = graph_bucket_role_for_node(&state->graph_nodes[i]);
        depth[i] = 9999;
        degree[i] = 0;
        neighbor_sum_x[i] = 0.0f;
        neighbor_count[i] = 0;
        score_x[i] = 0.0f;
        target_center_x[i] = focus_layout_center_x(&layouts[i]);
        target_center_y[i] = focus_layout_center_y(&layouts[i]);
        center_x[i] = target_center_x[i];
        center_y[i] = target_center_y[i];
        if (state->graph_nodes[i].item_id == state->selected_item_id) {
            selected_index = i;
        }
        if (role >= GRAPH_BUCKET_ROLE_SCOPE && role <= GRAPH_BUCKET_ROLE_MISC) {
            int slot = ((int)role) - 1;
            if (slot >= 0 && slot < 5 && anchor_by_role[slot] < 0) {
                anchor_by_role[slot] = i;
            }
            is_anchor[i] = 1;
            depth[i] = 0;
        }
    }

    for (i = 0; (uint32_t)i < edge_count; ++i) {
        int from_index = (int)edges[i].from_id - 1;
        int to_index = (int)edges[i].to_id - 1;
        if (from_index < 0 || to_index < 0 ||
            from_index >= node_count || to_index >= node_count ||
            from_index == to_index) {
            continue;
        }
        degree[from_index] += 1;
        degree[to_index] += 1;
        neighbor_sum_x[from_index] += center_x[to_index];
        neighbor_sum_x[to_index] += center_x[from_index];
        neighbor_count[from_index] += 1;
        neighbor_count[to_index] += 1;
    }

    for (pass = 0; pass < node_count; ++pass) {
        int changed = 0;
        for (i = 0; (uint32_t)i < edge_count; ++i) {
            int from_index = (int)edges[i].from_id - 1;
            int to_index = (int)edges[i].to_id - 1;
            if (from_index < 0 || to_index < 0 ||
                from_index >= node_count || to_index >= node_count) {
                continue;
            }
            if (depth[from_index] + 1 < depth[to_index]) {
                depth[to_index] = depth[from_index] + 1;
                changed = 1;
            }
            if (depth[to_index] + 1 < depth[from_index]) {
                depth[from_index] = depth[to_index] + 1;
                changed = 1;
            }
        }
        if (!changed) {
            break;
        }
    }

    for (i = 0; i < 5; ++i) {
        int node_index = anchor_by_role[i];
        float slot_center_x = 0.0f;
        float target_y = 0.0f;
        if (node_index < 0 || node_index >= node_count) {
            continue;
        }
        slot_center_x = bounds.x + ((((float)i) + 0.5f) / 5.0f) * bounds.width;
        target_y = bounds.y + (i == 0 ? 16.0f : 58.0f);
        target_y += focus_hash_signed((uint32_t)node_index * 101u) * 2.0f;
        target_center_x[node_index] = slot_center_x;
        target_center_y[node_index] = target_y + (layouts[node_index].rect.height * 0.5f);
        focus_layout_set_center(bounds,
                                &layouts[node_index],
                                target_center_x[node_index],
                                target_center_y[node_index],
                                bounds.y + 2.0f);
        center_x[node_index] = focus_layout_center_x(&layouts[node_index]);
        center_y[node_index] = focus_layout_center_y(&layouts[node_index]);
    }

    selected_center_x = bounds.x + (bounds.width * 0.5f);
    if (selected_index >= 0 && selected_index < node_count) {
        selected_center_x = center_x[selected_index];
    }

    for (i = 0; i < node_count; ++i) {
        if (is_anchor[i]) {
            continue;
        }

        {
            int safe_depth = depth[i];
            int level = 1;
            float base_x = center_x[i];
            if (safe_depth > FOCUS_LEVEL_MAX) {
                safe_depth = FOCUS_LEVEL_MAX;
            }
            if (safe_depth < 1) {
                safe_depth = 1;
            }
            level = safe_depth;
            if (level < 1) {
                level = 1;
            }
            if (level > FOCUS_LEVEL_MAX) {
                level = FOCUS_LEVEL_MAX;
            }
            if (neighbor_count[i] > 0) {
                base_x = neighbor_sum_x[i] / (float)neighbor_count[i];
            }
            base_x += focus_hash_signed((uint32_t)i * 2654435761u) *
                      (24.0f / (1.0f + (float)(degree[i] > 0 ? degree[i] : 1)));
            score_x[i] = base_x;
            if (level_counts[level] < MEM_CONSOLE_GRAPH_NODE_LIMIT) {
                int write_index = level_counts[level];
                level_members[level][write_index] = i;
                level_counts[level] += 1;
            }
        }
    }

    for (i = 1; i <= FOCUS_LEVEL_MAX; ++i) {
        int level_count = level_counts[i];
        int slot;
        float side_pad = 18.0f + (crowd * 8.0f);
        float usable_left = bounds.x + side_pad;
        float usable_right = bounds.x + bounds.width - side_pad;
        float usable_width = usable_right - usable_left;
        float level_start_y = bounds.y + 108.0f + (crowd * 8.0f);
        float level_step = (bounds.height - 230.0f) / (float)(FOCUS_LEVEL_MAX - 1);
        float level_y;
        int sorted_indices[MEM_CONSOLE_GRAPH_NODE_LIMIT];

        if (level_count <= 0) {
            continue;
        }
        if (level_step < (30.0f + (crowd * 8.0f))) {
            level_step = 30.0f + (crowd * 8.0f);
        }
        if (level_step > (56.0f + (crowd * 10.0f))) {
            level_step = 56.0f + (crowd * 10.0f);
        }
        level_y = level_start_y + ((float)(i - 1) * level_step);
        if (usable_width < 24.0f) {
            usable_width = 24.0f;
        }
        for (slot = 0; slot < level_count; ++slot) {
            sorted_indices[slot] = level_members[i][slot];
        }
        focus_layout_sort_indices_by_score(sorted_indices, level_count, score_x);

        for (slot = 0; slot < level_count; ++slot) {
            int node_index = sorted_indices[slot];
            int max_per_row = 4 + (i / 2);
            int row;
            int col;
            int row_count;
            int cols_this_row;
            float row_center_shift = 0.0f;
            float even_center_x;
            float bias_center_x = selected_center_x +
                                  (focus_hash_signed(((uint32_t)node_index + 17u) * 2246822519u) *
                                   (36.0f + ((float)i * 5.0f)));
            float jitter_y = focus_hash_signed((uint32_t)node_index * 3266489917u) * 8.0f;
            float degree_lift = degree[node_index] >= 4 ? -5.0f : 0.0f;

            if (max_per_row > 8) {
                max_per_row = 8;
            }
            if (max_per_row < 4) {
                max_per_row = 4;
            }
            row = slot / max_per_row;
            col = slot % max_per_row;
            row_count = (level_count + max_per_row - 1) / max_per_row;
            cols_this_row = level_count - (row * max_per_row);
            if (cols_this_row > max_per_row) {
                cols_this_row = max_per_row;
            }
            if (cols_this_row < 1) {
                cols_this_row = 1;
            }
            row_center_shift = ((float)row - ((float)(row_count - 1) * 0.5f)) * (34.0f + (crowd * 12.0f));
            even_center_x = usable_left +
                            (((float)(col + 1) / (float)(cols_this_row + 1)) * usable_width);

            target_center_x[node_index] = (even_center_x * (0.72f + (crowd * 0.16f))) +
                                          (bias_center_x * (0.28f - (crowd * 0.16f)));
            target_center_y[node_index] = level_y + row_center_shift + jitter_y + degree_lift;
        }
    }

    if (selected_index >= 0 &&
        selected_index < node_count &&
        !is_anchor[selected_index]) {
        target_center_x[selected_index] = bounds.x + (bounds.width * 0.5f);
        target_center_y[selected_index] = bounds.y + graph_clampf(bounds.height * 0.38f, 128.0f, 178.0f);
    }

    for (i = 0; i < node_count; ++i) {
        if (is_anchor[i]) {
            continue;
        }
        center_x[i] = (center_x[i] * 0.22f) + (target_center_x[i] * 0.78f);
        center_y[i] = (center_y[i] * 0.16f) + (target_center_y[i] * 0.84f);
        focus_layout_set_center(bounds,
                                &layouts[i],
                                center_x[i],
                                center_y[i],
                                bounds.y + 72.0f);
        center_x[i] = focus_layout_center_x(&layouts[i]);
        center_y[i] = focus_layout_center_y(&layouts[i]);
    }

    for (pass = 0; pass < 12; ++pass) {
        int a;
        int b;
        for (a = 0; a < node_count; ++a) {
            if (is_anchor[a]) {
                continue;
            }
            for (b = a + 1; b < node_count; ++b) {
                float dx;
                float dy;
                float min_dx;
                float min_dy;
                float overlap_x;
                float overlap_y;
                float dir;

                if (is_anchor[b]) {
                    continue;
                }

                dx = center_x[a] - center_x[b];
                dy = center_y[a] - center_y[b];
                min_dx = (layouts[a].rect.width + layouts[b].rect.width) * 0.5f + 18.0f + (crowd * 16.0f);
                min_dy = (layouts[a].rect.height + layouts[b].rect.height) * 0.5f + 24.0f + (crowd * 12.0f);
                if (fabsf(dx) >= min_dx || fabsf(dy) >= min_dy) {
                    continue;
                }

                overlap_x = min_dx - fabsf(dx);
                overlap_y = min_dy - fabsf(dy);
                if (overlap_x <= overlap_y) {
                    dir = dx >= 0.0f ? 1.0f : -1.0f;
                    if (fabsf(dx) < 0.1f) {
                        dir = focus_hash_signed((uint32_t)(a + 1) * 911382323u) >= 0.0f ? 1.0f : -1.0f;
                    }
                    center_x[a] += dir * overlap_x * 0.42f;
                    center_x[b] -= dir * overlap_x * 0.42f;
                } else {
                    dir = dy >= 0.0f ? 1.0f : -1.0f;
                    if (fabsf(dy) < 0.1f) {
                        dir = focus_hash_signed((uint32_t)(b + 3) * 972663749u) >= 0.0f ? 1.0f : -1.0f;
                    }
                    center_y[a] += dir * overlap_y * 0.52f;
                    center_y[b] -= dir * overlap_y * 0.52f;
                }
            }
        }

        for (a = 0; a < node_count; ++a) {
            if (is_anchor[a]) {
                continue;
            }
            for (b = a + 1; b < node_count; ++b) {
                float dx;
                float dy;
                float dist_sq;
                float min_dist_x = 72.0f + (crowd * 28.0f);
                float min_dist_y = 36.0f + (crowd * 16.0f);

                if (is_anchor[b]) {
                    continue;
                }
                dx = center_x[a] - center_x[b];
                dy = center_y[a] - center_y[b];
                if (fabsf(dx) >= min_dist_x || fabsf(dy) >= min_dist_y) {
                    continue;
                }
                dist_sq = (dx * dx) + (dy * dy);
                if (dist_sq < 0.01f) {
                    dx = focus_hash_signed((uint32_t)(a + 1) * 1640531513u);
                    dy = focus_hash_signed((uint32_t)(b + 7) * 2654435761u);
                    dist_sq = (dx * dx) + (dy * dy);
                }
                if (dist_sq > 0.01f) {
                    float dist = sqrtf(dist_sq);
                    float nx = dx / dist;
                    float ny = dy / dist;
                    float push = ((min_dist_x - fabsf(dx)) + (min_dist_y - fabsf(dy))) *
                                 (0.22f + (crowd * 0.08f));
                    center_x[a] += nx * push;
                    center_y[a] += ny * push * (1.12f + (crowd * 0.10f));
                    center_x[b] -= nx * push;
                    center_y[b] -= ny * push * (1.12f + (crowd * 0.10f));
                }
            }
        }

        for (i = 0; i < node_count; ++i) {
            if (is_anchor[i]) {
                continue;
            }
            center_x[i] += (target_center_x[i] - center_x[i]) * 0.12f;
            center_y[i] += (target_center_y[i] - center_y[i]) * 0.14f;
            focus_layout_set_center(bounds,
                                    &layouts[i],
                                    center_x[i],
                                    center_y[i],
                                    bounds.y + 72.0f);
            center_x[i] = focus_layout_center_x(&layouts[i]);
            center_y[i] = focus_layout_center_y(&layouts[i]);
        }
    }

    for (i = 0; i < node_count; ++i) {
        if (is_anchor[i]) {
            continue;
        }
        if (i == selected_index) {
            focus_layout_set_center(bounds,
                                    &layouts[i],
                                    target_center_x[i],
                                    target_center_y[i],
                                    bounds.y + 72.0f);
        }
    }
}

static int find_layout_index_by_node_id(const KitGraphStructNodeLayout *layouts,
                                        uint32_t layout_count,
                                        uint32_t node_id) {
    uint32_t i;

    if (!layouts) {
        return -1;
    }
    for (i = 0u; i < layout_count; ++i) {
        if (layouts[i].node_id == node_id) {
            return (int)i;
        }
    }
    return -1;
}

static int graph_rect_intersects_bounds(KitRenderRect rect, KitRenderRect bounds, float pad) {
    float rect_x0 = rect.x - pad;
    float rect_y0 = rect.y - pad;
    float rect_x1 = rect.x + rect.width + pad;
    float rect_y1 = rect.y + rect.height + pad;
    float bounds_x1 = bounds.x + bounds.width;
    float bounds_y1 = bounds.y + bounds.height;

    if (rect_x1 < bounds.x || rect_x0 > bounds_x1) {
        return 0;
    }
    if (rect_y1 < bounds.y || rect_y0 > bounds_y1) {
        return 0;
    }
    return 1;
}

static void filter_edges_for_visible_layout_nodes(KitRenderRect bounds,
                                                  const KitGraphStructNodeLayout *layouts,
                                                  uint32_t layout_count,
                                                  KitGraphStructEdge *edges,
                                                  int *edge_state_indices,
                                                  uint32_t *io_edge_count) {
    uint32_t read_index;
    uint32_t write_index = 0u;
    uint32_t edge_count;
    const float visibility_pad = 2.0f;

    if (!layouts || !edges || !edge_state_indices || !io_edge_count) {
        return;
    }

    edge_count = *io_edge_count;
    for (read_index = 0u; read_index < edge_count; ++read_index) {
        int from_index = find_layout_index_by_node_id(layouts, layout_count, edges[read_index].from_id);
        int to_index = find_layout_index_by_node_id(layouts, layout_count, edges[read_index].to_id);
        int from_visible = 0;
        int to_visible = 0;
        int keep = 1;

        if (from_index >= 0 && (uint32_t)from_index < layout_count) {
            from_visible = graph_rect_intersects_bounds(layouts[from_index].rect, bounds, visibility_pad);
        }
        if (to_index >= 0 && (uint32_t)to_index < layout_count) {
            to_visible = graph_rect_intersects_bounds(layouts[to_index].rect, bounds, visibility_pad);
        }

        /*
         * Keep edges only if at least one endpoint node is visible in current bounds.
         * This preserves visible->offscreen continuity while dropping fully offscreen clutter.
         */
        keep = from_visible || to_visible;

        if (!keep) {
            continue;
        }
        if (write_index != read_index) {
            edges[write_index] = edges[read_index];
            edge_state_indices[write_index] = edge_state_indices[read_index];
        }
        write_index += 1u;
    }

    *io_edge_count = write_index;
}

static float graph_lane_slot_for_index(uint32_t lane_index, uint32_t lane_count) {
    if (lane_count <= 1u) {
        return 0.0f;
    }
    /* Keep all lanes non-zero when we have multi-edges so nothing sits on top of a center line. */
    {
        float magnitude = (float)(lane_index / 2u) + 1.0f;
        float sign = (lane_index & 1u) ? 1.0f : -1.0f;
        return magnitude * sign;
    }
}

typedef enum GraphPortSide {
    GRAPH_PORT_TOP = 0,
    GRAPH_PORT_RIGHT = 1,
    GRAPH_PORT_BOTTOM = 2,
    GRAPH_PORT_LEFT = 3
} GraphPortSide;

static GraphPortSide graph_port_side_opposite(GraphPortSide side) {
    switch (side) {
        case GRAPH_PORT_TOP: return GRAPH_PORT_BOTTOM;
        case GRAPH_PORT_RIGHT: return GRAPH_PORT_LEFT;
        case GRAPH_PORT_BOTTOM: return GRAPH_PORT_TOP;
        case GRAPH_PORT_LEFT: return GRAPH_PORT_RIGHT;
    }
    return GRAPH_PORT_TOP;
}

static GraphPortSide graph_port_side_for_direction(float dx, float dy) {
    if (fabsf(dx) >= fabsf(dy)) {
        return dx >= 0.0f ? GRAPH_PORT_RIGHT : GRAPH_PORT_LEFT;
    }
    return dy >= 0.0f ? GRAPH_PORT_BOTTOM : GRAPH_PORT_TOP;
}

static void graph_port_side_normal(GraphPortSide side, float *out_nx, float *out_ny) {
    float nx = 0.0f;
    float ny = 0.0f;
    switch (side) {
        case GRAPH_PORT_TOP: ny = -1.0f; break;
        case GRAPH_PORT_RIGHT: nx = 1.0f; break;
        case GRAPH_PORT_BOTTOM: ny = 1.0f; break;
        case GRAPH_PORT_LEFT: nx = -1.0f; break;
    }
    if (out_nx) {
        *out_nx = nx;
    }
    if (out_ny) {
        *out_ny = ny;
    }
}

static float graph_port_slot_offset(float usable_span, int slot_index, int slot_count) {
    float step;
    if (slot_count <= 1 || usable_span <= 0.0f) {
        return 0.0f;
    }
    step = usable_span / (float)(slot_count + 1);
    return (-usable_span * 0.5f) + (step * (float)(slot_index + 1));
}

static KitRenderVec2 graph_compute_node_port_point(const KitRenderRect *rect,
                                                   GraphPortSide side,
                                                   int slot_index,
                                                   int slot_count) {
    KitRenderVec2 point = { 0.0f, 0.0f };
    float center_x;
    float center_y;
    float usable_w;
    float usable_h;

    if (!rect) {
        return point;
    }

    center_x = rect->x + (rect->width * 0.5f);
    center_y = rect->y + (rect->height * 0.5f);
    usable_w = rect->width - 8.0f;
    usable_h = rect->height - 6.0f;
    if (usable_w < 2.0f) {
        usable_w = 2.0f;
    }
    if (usable_h < 2.0f) {
        usable_h = 2.0f;
    }

    switch (side) {
        case GRAPH_PORT_TOP:
            point.x = center_x + graph_port_slot_offset(usable_w, slot_index, slot_count);
            point.y = rect->y;
            break;
        case GRAPH_PORT_RIGHT:
            point.x = rect->x + rect->width;
            point.y = center_y + graph_port_slot_offset(usable_h, slot_index, slot_count);
            break;
        case GRAPH_PORT_BOTTOM:
            point.x = center_x + graph_port_slot_offset(usable_w, slot_index, slot_count);
            point.y = rect->y + rect->height;
            break;
        case GRAPH_PORT_LEFT:
            point.x = rect->x;
            point.y = center_y + graph_port_slot_offset(usable_h, slot_index, slot_count);
            break;
    }

    point.x = graph_clampf(point.x, rect->x + 1.0f, rect->x + rect->width - 1.0f);
    point.y = graph_clampf(point.y, rect->y + 1.0f, rect->y + rect->height - 1.0f);
    return point;
}

static void apply_endpoint_port_slots(const KitGraphStructEdge *edges,
                                      uint32_t edge_count,
                                      const KitGraphStructNodeLayout *layouts,
                                      uint32_t layout_count,
                                      KitGraphStructEdgeRoute *routes) {
    int per_side_totals[MEM_CONSOLE_GRAPH_NODE_LIMIT][4];
    int per_side_seen[MEM_CONSOLE_GRAPH_NODE_LIMIT][4];
    int from_layout_index[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    int to_layout_index[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    uint8_t from_side[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    uint8_t to_side[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    uint32_t i;
    const float stem_len = 10.0f;

    if (!edges || !layouts || !routes || edge_count == 0u) {
        return;
    }

    memset(per_side_totals, 0, sizeof(per_side_totals));
    memset(per_side_seen, 0, sizeof(per_side_seen));
    memset(from_layout_index, -1, sizeof(from_layout_index));
    memset(to_layout_index, -1, sizeof(to_layout_index));
    memset(from_side, 0, sizeof(from_side));
    memset(to_side, 0, sizeof(to_side));

    for (i = 0u; i < edge_count && i < MEM_CONSOLE_GRAPH_EDGE_LIMIT; ++i) {
        int from_idx = find_layout_index_by_node_id(layouts, layout_count, edges[i].from_id);
        int to_idx = find_layout_index_by_node_id(layouts, layout_count, edges[i].to_id);
        float from_cx;
        float from_cy;
        float to_cx;
        float to_cy;
        GraphPortSide source_side;
        GraphPortSide target_side;

        if (from_idx < 0 || to_idx < 0 ||
            from_idx >= MEM_CONSOLE_GRAPH_NODE_LIMIT ||
            to_idx >= MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            continue;
        }
        from_layout_index[i] = from_idx;
        to_layout_index[i] = to_idx;

        from_cx = layouts[from_idx].rect.x + (layouts[from_idx].rect.width * 0.5f);
        from_cy = layouts[from_idx].rect.y + (layouts[from_idx].rect.height * 0.5f);
        to_cx = layouts[to_idx].rect.x + (layouts[to_idx].rect.width * 0.5f);
        to_cy = layouts[to_idx].rect.y + (layouts[to_idx].rect.height * 0.5f);

        source_side = graph_port_side_for_direction(to_cx - from_cx, to_cy - from_cy);
        target_side = graph_port_side_opposite(source_side);
        from_side[i] = (uint8_t)source_side;
        to_side[i] = (uint8_t)target_side;

        per_side_totals[from_idx][(int)source_side] += 1;
        per_side_totals[to_idx][(int)target_side] += 1;
    }

    for (i = 0u; i < edge_count && i < MEM_CONSOLE_GRAPH_EDGE_LIMIT; ++i) {
        int from_idx = from_layout_index[i];
        int to_idx = to_layout_index[i];
        GraphPortSide source_side;
        GraphPortSide target_side;
        int source_slot;
        int target_slot;
        int source_slot_count;
        int target_slot_count;
        KitRenderVec2 source_port;
        KitRenderVec2 target_port;
        KitRenderVec2 source_stem;
        KitRenderVec2 target_stem;
        float source_nx = 0.0f;
        float source_ny = 0.0f;
        float target_nx = 0.0f;
        float target_ny = 0.0f;
        KitGraphStructEdgeRoute *route = &routes[i];

        if (from_idx < 0 || to_idx < 0 ||
            from_idx >= MEM_CONSOLE_GRAPH_NODE_LIMIT ||
            to_idx >= MEM_CONSOLE_GRAPH_NODE_LIMIT ||
            route->point_count == 0u) {
            continue;
        }

        source_side = (GraphPortSide)from_side[i];
        target_side = (GraphPortSide)to_side[i];
        source_slot = per_side_seen[from_idx][(int)source_side];
        target_slot = per_side_seen[to_idx][(int)target_side];
        source_slot_count = per_side_totals[from_idx][(int)source_side];
        target_slot_count = per_side_totals[to_idx][(int)target_side];
        if (source_slot_count <= 0) {
            source_slot_count = 1;
        }
        if (target_slot_count <= 0) {
            target_slot_count = 1;
        }
        per_side_seen[from_idx][(int)source_side] += 1;
        per_side_seen[to_idx][(int)target_side] += 1;

        source_port = graph_compute_node_port_point(&layouts[from_idx].rect,
                                                    source_side,
                                                    source_slot,
                                                    source_slot_count);
        target_port = graph_compute_node_port_point(&layouts[to_idx].rect,
                                                    target_side,
                                                    target_slot,
                                                    target_slot_count);
        graph_port_side_normal(source_side, &source_nx, &source_ny);
        graph_port_side_normal(target_side, &target_nx, &target_ny);
        source_stem = (KitRenderVec2){
            source_port.x + (source_nx * stem_len),
            source_port.y + (source_ny * stem_len)
        };
        target_stem = (KitRenderVec2){
            target_port.x + (target_nx * stem_len),
            target_port.y + (target_ny * stem_len)
        };

        if (route->point_count < 5u) {
            KitRenderVec2 elbow;
            route->point_count = 5u;
            if (fabsf(source_stem.x - target_stem.x) >= fabsf(source_stem.y - target_stem.y)) {
                elbow.x = target_stem.x;
                elbow.y = source_stem.y;
            } else {
                elbow.x = source_stem.x;
                elbow.y = target_stem.y;
            }
            route->points[0] = source_port;
            route->points[1] = source_stem;
            route->points[2] = elbow;
            route->points[3] = target_stem;
            route->points[4] = target_port;
            continue;
        }

        route->points[0] = source_port;
        route->points[1] = source_stem;
        route->points[route->point_count - 2u] = target_stem;
        route->points[route->point_count - 1u] = target_port;
    }
}

static int graph_layout_node_is_anchor(const MemConsoleState *state, int layout_index) {
    if (!state || layout_index < 0 || layout_index >= MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        return 0;
    }
    if (layout_index >= state->graph_node_count) {
        return 0;
    }
    return graph_bucket_role_for_node(&state->graph_nodes[layout_index]) != GRAPH_BUCKET_ROLE_NONE ? 1 : 0;
}

static void apply_top_anchor_funnel_mode(const MemConsoleState *state,
                                         const KitGraphStructEdge *edges,
                                         uint32_t edge_count,
                                         const KitGraphStructNodeLayout *layouts,
                                         uint32_t layout_count,
                                         KitGraphStructEdgeRoute *routes) {
    uint32_t edge_index;
    const float stem_len = 10.0f;

    if (!state || !state->graph_anchor_funnel_enabled || !edges || !layouts || !routes) {
        return;
    }

    for (edge_index = 0u; edge_index < edge_count; ++edge_index) {
        KitGraphStructEdgeRoute *route = &routes[edge_index];
        int to_idx = find_layout_index_by_node_id(layouts, layout_count, edges[edge_index].to_id);
        uint32_t last_idx;
        uint32_t stem_idx;
        GraphPortSide target_side;
        KitRenderVec2 target_port;
        KitRenderVec2 target_stem;
        float nx = 0.0f;
        float ny = 0.0f;
        float center_x;
        float center_y;
        float dir_x;
        float dir_y;

        if (route->point_count < 4u || to_idx < 0) {
            continue;
        }
        if (!graph_layout_node_is_anchor(state, to_idx)) {
            continue;
        }

        last_idx = route->point_count - 1u;
        stem_idx = last_idx - 1u;
        center_x = layouts[to_idx].rect.x + (layouts[to_idx].rect.width * 0.5f);
        center_y = layouts[to_idx].rect.y + (layouts[to_idx].rect.height * 0.5f);
        dir_x = route->points[last_idx].x - center_x;
        dir_y = route->points[last_idx].y - center_y;
        target_side = graph_port_side_for_direction(dir_x, dir_y);
        target_port = graph_compute_node_port_point(&layouts[to_idx].rect, target_side, 0, 1);
        graph_port_side_normal(target_side, &nx, &ny);
        target_stem = (KitRenderVec2){
            target_port.x + (nx * stem_len),
            target_port.y + (ny * stem_len)
        };

        route->points[stem_idx] = target_stem;
        route->points[last_idx] = target_port;

        if (stem_idx >= 2u) {
            uint32_t pre_stem_idx = stem_idx - 1u;
            if (target_side == GRAPH_PORT_LEFT || target_side == GRAPH_PORT_RIGHT) {
                route->points[pre_stem_idx].x = target_stem.x;
            } else {
                route->points[pre_stem_idx].y = target_stem.y;
            }
        }
    }
}

static KitRenderRect graph_expand_rect(KitRenderRect rect, float margin) {
    rect.x -= margin;
    rect.y -= margin;
    rect.width += margin * 2.0f;
    rect.height += margin * 2.0f;
    return rect;
}

static int graph_segment_hits_rect(KitRenderVec2 a, KitRenderVec2 b, KitRenderRect rect) {
    float x0 = a.x < b.x ? a.x : b.x;
    float x1 = a.x > b.x ? a.x : b.x;
    float y0 = a.y < b.y ? a.y : b.y;
    float y1 = a.y > b.y ? a.y : b.y;
    float rect_x1 = rect.x + rect.width;
    float rect_y1 = rect.y + rect.height;

    if (fabsf(a.y - b.y) <= fabsf(a.x - b.x)) {
        float y = (a.y + b.y) * 0.5f;
        if (y < rect.y || y > rect_y1) {
            return 0;
        }
        return !(x1 < rect.x || x0 > rect_x1);
    }

    {
        float x = (a.x + b.x) * 0.5f;
        if (x < rect.x || x > rect_x1) {
            return 0;
        }
        return !(y1 < rect.y || y0 > rect_y1);
    }
}

static float graph_pick_clear_coordinate(float current,
                                         float option_a,
                                         float option_b,
                                         float min_value,
                                         float max_value) {
    float candidate_a = graph_clampf(option_a, min_value, max_value);
    float candidate_b = graph_clampf(option_b, min_value, max_value);
    float delta_a = fabsf(candidate_a - current);
    float delta_b = fabsf(candidate_b - current);
    return delta_a <= delta_b ? candidate_a : candidate_b;
}

static void apply_node_obstacle_avoidance(const KitGraphStructEdge *edges,
                                          uint32_t edge_count,
                                          const KitGraphStructNodeLayout *layouts,
                                          uint32_t layout_count,
                                          KitRenderRect bounds,
                                          KitGraphStructEdgeRoute *routes,
                                          float zoom,
                                          int scope_full_mode_enabled) {
    uint32_t edge_index;
    float node_margin = 4.0f;
    float nudge_gap = 12.0f;
    float zoom_strength = 1.0f;
    const float min_x = bounds.x + 6.0f;
    const float max_x = bounds.x + bounds.width - 6.0f;
    const float min_y = bounds.y + 6.0f;
    const float max_y = bounds.y + bounds.height - 6.0f;
    int max_attempts = 10;

    if (!edges || !layouts || !routes || edge_count == 0u) {
        return;
    }
    {
        float min_zoom_for_avoid = scope_full_mode_enabled ? 1.85f : 1.15f;
        if (zoom < min_zoom_for_avoid) {
            return;
        }
        zoom_strength = graph_clampf((zoom - min_zoom_for_avoid) /
                                     (scope_full_mode_enabled ? 0.95f : 0.85f),
                                     0.0f,
                                     1.0f);
    }
    if (zoom_strength <= 0.01f) {
        return;
    }
    if (zoom_strength < 0.40f) {
        max_attempts = 4;
    } else if (zoom_strength < 0.75f) {
        max_attempts = 7;
    }
    if (layout_count > 64u || edge_count > 220u) {
        node_margin = 6.0f;
        nudge_gap = 15.0f;
    } else if (layout_count > 36u || edge_count > 140u) {
        node_margin = 5.0f;
        nudge_gap = 14.0f;
    }
    node_margin = (node_margin * 0.25f) + (node_margin * 0.75f * zoom_strength);
    nudge_gap = (nudge_gap * 0.30f) + (nudge_gap * 0.70f * zoom_strength);

    for (edge_index = 0u; edge_index < edge_count; ++edge_index) {
        KitGraphStructEdgeRoute *route = &routes[edge_index];
        int from_idx = find_layout_index_by_node_id(layouts, layout_count, edges[edge_index].from_id);
        int to_idx = find_layout_index_by_node_id(layouts, layout_count, edges[edge_index].to_id);
        int attempt;

        if (route->point_count < 4u || from_idx < 0 || to_idx < 0) {
            continue;
        }

        for (attempt = 0; attempt < max_attempts; ++attempt) {
            int moved = 0;
            uint32_t seg;

            for (seg = 1u; seg + 1u < route->point_count - 1u; ++seg) {
                KitRenderVec2 a = route->points[seg];
                KitRenderVec2 b = route->points[seg + 1u];
                int is_horizontal = fabsf(a.y - b.y) <= fabsf(a.x - b.x);
                uint32_t node_idx;

                for (node_idx = 0u; node_idx < layout_count; ++node_idx) {
                    KitRenderRect obstacle;

                    if ((int)node_idx == from_idx || (int)node_idx == to_idx) {
                        continue;
                    }
                    obstacle = graph_expand_rect(layouts[node_idx].rect, node_margin);
                    if (!graph_segment_hits_rect(a, b, obstacle)) {
                        continue;
                    }

                    if (is_horizontal) {
                        float y_up = obstacle.y - nudge_gap;
                        float y_down = obstacle.y + obstacle.height + nudge_gap;
                        float target_y = graph_pick_clear_coordinate(a.y, y_up, y_down, min_y, max_y);
                        route->points[seg].y = target_y;
                        route->points[seg + 1u].y = target_y;
                    } else {
                        float x_left = obstacle.x - nudge_gap;
                        float x_right = obstacle.x + obstacle.width + nudge_gap;
                        float target_x = graph_pick_clear_coordinate(a.x, x_left, x_right, min_x, max_x);
                        route->points[seg].x = target_x;
                        route->points[seg + 1u].x = target_x;
                    }
                    moved = 1;
                    break;
                }
                if (moved) {
                    break;
                }
            }

            if (!moved) {
                break;
            }
        }
    }
}

static void apply_multi_edge_route_lanes(const KitGraphStructEdge *edges,
                                         uint32_t edge_count,
                                         const KitGraphStructNodeLayout *layouts,
                                         uint32_t layout_count,
                                         KitGraphStructEdgeRoute *routes) {
    float lane_spacing = 8.0f;
    uint32_t i;

    if (!edges || !layouts || !routes || edge_count == 0u) {
        return;
    }
    if (layout_count > 64u || edge_count > 220u) {
        lane_spacing = 11.0f;
    } else if (layout_count > 36u || edge_count > 140u) {
        lane_spacing = 9.5f;
    }

    for (i = 0u; i < edge_count; ++i) {
        uint32_t from_id = edges[i].from_id;
        uint32_t to_id = edges[i].to_id;
        uint32_t pair_min = from_id < to_id ? from_id : to_id;
        uint32_t pair_max = from_id < to_id ? to_id : from_id;
        uint32_t lane_index = 0u;
        uint32_t lane_count = 0u;
        uint32_t j;
        float lane_slot = 0.0f;
        float lane_offset = 0.0f;
        float offset_x = 0.0f;
        float offset_y = 0.0f;
        int from_layout_index;
        int to_layout_index;

        if (routes[i].point_count < 2u) {
            continue;
        }

        for (j = 0u; j < edge_count; ++j) {
            uint32_t other_from = edges[j].from_id;
            uint32_t other_to = edges[j].to_id;
            uint32_t other_min = other_from < other_to ? other_from : other_to;
            uint32_t other_max = other_from < other_to ? other_to : other_from;
            if (other_min == pair_min && other_max == pair_max) {
                if (j < i) {
                    lane_index += 1u;
                }
                lane_count += 1u;
            }
        }
        if (lane_count <= 1u) {
            continue;
        }

        from_layout_index = find_layout_index_by_node_id(layouts, layout_count, from_id);
        to_layout_index = find_layout_index_by_node_id(layouts, layout_count, to_id);
        if (from_layout_index < 0 || to_layout_index < 0) {
            continue;
        }

        lane_slot = graph_lane_slot_for_index(lane_index, lane_count);
        lane_offset = lane_slot * lane_spacing;
        if (fabsf(lane_offset) < 0.01f) {
            continue;
        }

        {
            float from_center_x = layouts[from_layout_index].rect.x + (layouts[from_layout_index].rect.width * 0.5f);
            float from_center_y = layouts[from_layout_index].rect.y + (layouts[from_layout_index].rect.height * 0.5f);
            float to_center_x = layouts[to_layout_index].rect.x + (layouts[to_layout_index].rect.width * 0.5f);
            float to_center_y = layouts[to_layout_index].rect.y + (layouts[to_layout_index].rect.height * 0.5f);
            float dx = to_center_x - from_center_x;
            float dy = to_center_y - from_center_y;

            if (fabsf(dx) >= fabsf(dy)) {
                offset_y = lane_offset;
            } else {
                offset_x = lane_offset;
            }
        }

        for (j = 0u; j < routes[i].point_count; ++j) {
            routes[i].points[j].x += offset_x;
            routes[i].points[j].y += offset_y;
        }
    }
}

static CoreResult compute_graph_preview_layout(const MemConsoleState *state,
                                               const KitRenderContext *render_ctx,
                                               KitRenderRect bounds,
                                               KitGraphStructNode *out_nodes,
                                               uint32_t *out_node_count,
                                               KitGraphStructEdge *out_edges,
                                               uint32_t *out_edge_count,
                                               int *out_edge_state_indices,
                                               KitGraphStructNodeLayout *out_layouts,
                                               int *out_has_graph_data) {
    KitGraphStructLayoutStyle style;
    KitGraphStructViewport viewport;
    KitGraphStructViewport layout_viewport;
    char node_labels[MEM_CONSOLE_GRAPH_NODE_LIMIT][32];
    uint32_t node_count = 0u;
    uint32_t edge_count = 0u;
    uint32_t i;
    int has_graph_data = 0;

    if (!state || !out_nodes || !out_node_count || !out_edges || !out_edge_count ||
        !out_edge_state_indices || !out_layouts || !out_has_graph_data) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph preview layout request" };
    }

    has_graph_data = state->graph_node_count > 0 ? 1 : 0;
    if (has_graph_data) {
        node_count = (uint32_t)state->graph_node_count;
        if (node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
        }

        for (i = 0u; i < node_count; ++i) {
            out_nodes[i].id = i + 1u;
            graph_build_node_label_text(&state->graph_nodes[i],
                                        node_labels[i],
                                        sizeof(node_labels[i]));
            out_nodes[i].label = node_labels[i];
        }

        for (i = 0u; i < (uint32_t)state->graph_edge_count && edge_count < MEM_CONSOLE_GRAPH_EDGE_LIMIT; ++i) {
            int from_index = state->graph_edges[i].from_index;
            int to_index = state->graph_edges[i].to_index;
            const char *edge_kind = state->graph_edges[i].kind[0] ? state->graph_edges[i].kind : "related";
            GraphBucketRole from_role = GRAPH_BUCKET_ROLE_NONE;
            GraphBucketRole to_role = GRAPH_BUCKET_ROLE_NONE;

            if (from_index < 0 || to_index < 0) {
                continue;
            }
            if ((uint32_t)from_index >= node_count || (uint32_t)to_index >= node_count) {
                continue;
            }
            if (!mem_console_graph_node_kind_is_enabled(state, state->graph_nodes[from_index].kind) ||
                !mem_console_graph_node_kind_is_enabled(state, state->graph_nodes[to_index].kind)) {
                continue;
            }
            if (!mem_console_graph_kind_is_enabled(state, edge_kind)) {
                continue;
            }
            from_role = graph_bucket_role_for_node(&state->graph_nodes[from_index]);
            to_role = graph_bucket_role_for_node(&state->graph_nodes[to_index]);
            if ((from_role != GRAPH_BUCKET_ROLE_NONE &&
                 mem_console_graph_anchor_hidden_is_set(state, state->graph_nodes[from_index].item_id)) ||
                (to_role != GRAPH_BUCKET_ROLE_NONE &&
                 mem_console_graph_anchor_hidden_is_set(state, state->graph_nodes[to_index].item_id))) {
                continue;
            }

            out_edges[edge_count].from_id = (uint32_t)from_index + 1u;
            out_edges[edge_count].to_id = (uint32_t)to_index + 1u;
            out_edge_state_indices[edge_count] = (int)i;
            edge_count += 1u;
        }
    } else {
        node_count = 1u;
        out_nodes[0].id = 1u;
        out_nodes[0].label = "No Memory";
    }

    configure_graph_layout_style(&style);
    viewport = state->graph_viewport;
    kit_graph_struct_viewport_default(&layout_viewport);

    (void)render_ctx;

    {
        KitRenderRect world_bounds = {
            0.0f,
            0.0f,
            bounds.width,
            bounds.height
        };
        KitRenderRect virtual_bounds = {
            0.0f,
            0.0f,
            bounds.height,
            bounds.width
        };
        CoreResult result;
        if (state->graph_layout_mode == MEM_CONSOLE_GRAPH_LAYOUT_TREE) {
            result = kit_graph_struct_compute_layered_tree_layout(out_nodes,
                                                                  node_count,
                                                                  out_edges,
                                                                  edge_count,
                                                                  virtual_bounds,
                                                                  &layout_viewport,
                                                                  &style,
                                                                  out_layouts);
        } else {
            result = kit_graph_struct_compute_layered_dag_layout(out_nodes,
                                                                 node_count,
                                                                 out_edges,
                                                                 edge_count,
                                                                 virtual_bounds,
                                                                 &layout_viewport,
                                                                 &style,
                                                                 out_layouts);
        }
        if (result.code != CORE_OK) {
            return result;
        }
        transpose_layouts_to_horizontal_flow(world_bounds, out_layouts, node_count);
        if (state->graph_scope_full_mode_enabled && has_graph_data) {
            apply_project_pod_layout(world_bounds, state, out_layouts, node_count);
        } else if (has_graph_data) {
            apply_focus_anchor_priority_layout(world_bounds,
                                               state,
                                               out_edges,
                                               edge_count,
                                               out_layouts,
                                               node_count);
        }
        graph_camera_apply_to_layouts(out_layouts, node_count, &viewport, bounds);
    }

    *out_node_count = node_count;
    *out_edge_count = edge_count;
    *out_has_graph_data = has_graph_data;
    return core_result_ok();
}

static uint64_t graph_hash_bytes(uint64_t seed, const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint64_t hash = seed;
    size_t i;

    for (i = 0u; i < len; ++i) {
        hash ^= (uint64_t)bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint32_t float_bits(float value) {
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static uint64_t graph_preview_layout_signature(const MemConsoleState *state, KitRenderRect bounds) {
    uint64_t hash = 1469598103934665603ull;
    uint32_t bounds_bits[4];
    uint32_t zoom_bits[1];
    int node_count = 0;
    int edge_count = 0;
    int hidden_anchor_count = 0;
    int i;

    if (!state) {
        return hash;
    }

    bounds_bits[0] = float_bits(bounds.x);
    bounds_bits[1] = float_bits(bounds.y);
    bounds_bits[2] = float_bits(bounds.width);
    bounds_bits[3] = float_bits(bounds.height);
    zoom_bits[0] = float_bits(state->graph_viewport.zoom);

    hash = graph_hash_bytes(hash, &state->graph_node_count, sizeof(state->graph_node_count));
    hash = graph_hash_bytes(hash, &state->graph_edge_count, sizeof(state->graph_edge_count));
    hash = graph_hash_bytes(hash, bounds_bits, sizeof(bounds_bits));
    hash = graph_hash_bytes(hash, zoom_bits, sizeof(zoom_bits));
    hash = graph_hash_bytes(hash, state->graph_kind_filter, strlen(state->graph_kind_filter));
    hash = graph_hash_bytes(hash, &state->graph_kind_filter_mask, sizeof(state->graph_kind_filter_mask));
    hash = graph_hash_bytes(hash,
                            &state->graph_kind_filter_all_override,
                            sizeof(state->graph_kind_filter_all_override));
    hash = graph_hash_bytes(hash,
                            &state->graph_node_kind_filter_mask,
                            sizeof(state->graph_node_kind_filter_mask));
    hash = graph_hash_bytes(hash,
                            &state->graph_node_kind_filter_all_override,
                            sizeof(state->graph_node_kind_filter_all_override));
    hash = graph_hash_bytes(hash, &state->graph_layout_mode, sizeof(state->graph_layout_mode));
    hash = graph_hash_bytes(hash, &state->graph_sort_mode, sizeof(state->graph_sort_mode));
    hash = graph_hash_bytes(hash,
                            &state->graph_scope_full_mode_enabled,
                            sizeof(state->graph_scope_full_mode_enabled));
    hash = graph_hash_bytes(hash,
                            &state->graph_anchor_funnel_enabled,
                            sizeof(state->graph_anchor_funnel_enabled));
    hidden_anchor_count = state->graph_hidden_anchor_count;
    if (hidden_anchor_count < 0) hidden_anchor_count = 0;
    if (hidden_anchor_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) hidden_anchor_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    hash = graph_hash_bytes(hash, &hidden_anchor_count, sizeof(hidden_anchor_count));
    if (hidden_anchor_count > 0) {
        hash = graph_hash_bytes(hash,
                                state->graph_hidden_anchor_item_ids,
                                (size_t)hidden_anchor_count * sizeof(state->graph_hidden_anchor_item_ids[0]));
    }
    hash = graph_hash_bytes(hash, &state->font_preset_id, sizeof(state->font_preset_id));

    node_count = state->graph_node_count;
    if (node_count < 0) node_count = 0;
    if (node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    edge_count = state->graph_edge_count;
    if (edge_count < 0) edge_count = 0;
    if (edge_count > MEM_CONSOLE_GRAPH_EDGE_LIMIT) edge_count = MEM_CONSOLE_GRAPH_EDGE_LIMIT;

    for (i = 0; i < node_count; ++i) {
        hash = graph_hash_bytes(hash, &state->graph_nodes[i].item_id, sizeof(state->graph_nodes[i].item_id));
        hash = graph_hash_bytes(hash,
                                state->graph_nodes[i].title,
                                strlen(state->graph_nodes[i].title));
        hash = graph_hash_bytes(hash,
                                state->graph_nodes[i].project_key,
                                strlen(state->graph_nodes[i].project_key));
        hash = graph_hash_bytes(hash,
                                state->graph_nodes[i].kind,
                                strlen(state->graph_nodes[i].kind));
        hash = graph_hash_bytes(hash,
                                state->graph_nodes[i].stable_id,
                                strlen(state->graph_nodes[i].stable_id));
        hash = graph_hash_bytes(hash, &state->graph_nodes[i].pinned, sizeof(state->graph_nodes[i].pinned));
        hash = graph_hash_bytes(hash, &state->graph_nodes[i].canonical, sizeof(state->graph_nodes[i].canonical));
    }
    for (i = 0; i < edge_count; ++i) {
        hash = graph_hash_bytes(hash, &state->graph_edges[i].from_index, sizeof(state->graph_edges[i].from_index));
        hash = graph_hash_bytes(hash, &state->graph_edges[i].to_index, sizeof(state->graph_edges[i].to_index));
        hash = graph_hash_bytes(hash,
                                state->graph_edges[i].kind,
                                strlen(state->graph_edges[i].kind));
    }

    return hash;
}

CoreResult mem_console_ui_graph_ensure_layout_cache(const KitRenderContext *render_ctx,
                                                    MemConsoleState *state,
                                                    KitRenderRect bounds) {
    KitGraphStructLayoutStyle style;
    KitGraphStructEdgeLabel edge_labels[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    KitGraphStructEdgeLabelOptions label_options;
    CoreResult result;
    uint64_t signature;
    uint32_t i;

    if (!state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph layout cache request" };
    }

    signature = graph_preview_layout_signature(state, bounds);
    if (state->graph_layout_valid && state->graph_layout_signature == signature) {
        return core_result_ok();
    }

    result = compute_graph_preview_layout(state,
                                          render_ctx,
                                          bounds,
                                          state->graph_layout_nodes,
                                          &state->graph_layout_node_count,
                                          state->graph_layout_edges,
                                          &state->graph_layout_edge_count,
                                          state->graph_layout_edge_state_indices,
                                          state->graph_layout_node_layouts,
                                          &state->graph_layout_has_graph_data);
    if (result.code != CORE_OK) {
        return result;
    }

    filter_edges_for_visible_layout_nodes(bounds,
                                          state->graph_layout_node_layouts,
                                          state->graph_layout_node_count,
                                          state->graph_layout_edges,
                                          state->graph_layout_edge_state_indices,
                                          &state->graph_layout_edge_count);

    configure_graph_layout_style(&style);
    for (i = 0u; i < state->graph_layout_edge_count; ++i) {
        int state_edge_index = state->graph_layout_edge_state_indices[i];
        const char *edge_kind_label = "related";
        if (state_edge_index >= 0 &&
            state_edge_index < state->graph_edge_count &&
            state->graph_edges[state_edge_index].kind[0] != '\0') {
            edge_kind_label = state->graph_edges[state_edge_index].kind;
        }
        edge_labels[i].text = edge_kind_label;
    }
    result = kit_graph_struct_compute_edge_routes(state->graph_layout_edges,
                                                  state->graph_layout_edge_count,
                                                  state->graph_layout_node_layouts,
                                                  state->graph_layout_node_count,
                                                  KIT_GRAPH_STRUCT_ROUTE_ORTHOGONAL,
                                                  state->graph_layout_edge_routes);
    if (result.code != CORE_OK) {
        return result;
    }
    apply_multi_edge_route_lanes(state->graph_layout_edges,
                                 state->graph_layout_edge_count,
                                 state->graph_layout_node_layouts,
                                 state->graph_layout_node_count,
                                 state->graph_layout_edge_routes);
    apply_endpoint_port_slots(state->graph_layout_edges,
                              state->graph_layout_edge_count,
                              state->graph_layout_node_layouts,
                              state->graph_layout_node_count,
                              state->graph_layout_edge_routes);
    apply_top_anchor_funnel_mode(state,
                                 state->graph_layout_edges,
                                 state->graph_layout_edge_count,
                                 state->graph_layout_node_layouts,
                                 state->graph_layout_node_count,
                                 state->graph_layout_edge_routes);
    apply_node_obstacle_avoidance(state->graph_layout_edges,
                                  state->graph_layout_edge_count,
                                  state->graph_layout_node_layouts,
                                  state->graph_layout_node_count,
                                  bounds,
                                  state->graph_layout_edge_routes,
                                  state->graph_viewport.zoom,
                                  state->graph_scope_full_mode_enabled);
    kit_graph_struct_edge_label_options_default(&label_options);
    label_options.current_zoom = state->graph_viewport.zoom;
    label_options.min_zoom_for_labels = 0.82f;
    label_options.density_mode = KIT_GRAPH_STRUCT_EDGE_LABEL_DENSITY_CULL_OVERLAP;
    result = kit_graph_struct_compute_edge_label_layouts_routed(state->graph_layout_node_layouts,
                                                                state->graph_layout_node_count,
                                                                state->graph_layout_edge_routes,
                                                                state->graph_layout_edge_count,
                                                                &style,
                                                                edge_labels,
                                                                state->graph_layout_edge_count,
                                                                &label_options,
                                                                state->graph_layout_edge_label_layouts);
    if (result.code != CORE_OK) {
        return result;
    }
    refine_edge_label_layouts_for_callouts(bounds,
                                           state->graph_layout_edge_routes,
                                           state->graph_layout_edge_count,
                                           state->graph_layout_edge_label_layouts);

    state->graph_layout_signature = signature;
    state->graph_layout_bounds = bounds;
    state->graph_layout_valid = 1;
    return core_result_ok();
}

CoreResult mem_console_ui_graph_center_layout_view(MemConsoleState *state) {
    float min_x;
    float min_y;
    float max_x;
    float max_y;
    float current_center_x;
    float current_center_y;
    float target_center_x;
    float target_center_y;
    float delta_x;
    float delta_y;
    uint32_t i;

    if (!state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph center request" };
    }
    if (!state->graph_layout_valid || state->graph_layout_node_count == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "graph layout unavailable" };
    }

    min_x = state->graph_layout_node_layouts[0].rect.x;
    min_y = state->graph_layout_node_layouts[0].rect.y;
    max_x = state->graph_layout_node_layouts[0].rect.x + state->graph_layout_node_layouts[0].rect.width;
    max_y = state->graph_layout_node_layouts[0].rect.y + state->graph_layout_node_layouts[0].rect.height;
    for (i = 1u; i < state->graph_layout_node_count; ++i) {
        float x0 = state->graph_layout_node_layouts[i].rect.x;
        float y0 = state->graph_layout_node_layouts[i].rect.y;
        float x1 = x0 + state->graph_layout_node_layouts[i].rect.width;
        float y1 = y0 + state->graph_layout_node_layouts[i].rect.height;

        if (x0 < min_x) min_x = x0;
        if (y0 < min_y) min_y = y0;
        if (x1 > max_x) max_x = x1;
        if (y1 > max_y) max_y = y1;
    }

    current_center_x = (min_x + max_x) * 0.5f;
    current_center_y = (min_y + max_y) * 0.5f;
    target_center_x = state->graph_layout_bounds.x + (state->graph_layout_bounds.width * 0.5f);
    target_center_y = state->graph_layout_bounds.y + (state->graph_layout_bounds.height * 0.5f);
    delta_x = target_center_x - current_center_x;
    delta_y = target_center_y - current_center_y;
    graph_camera_pan_by_screen_delta(&state->graph_viewport, delta_x, delta_y);

    state->graph_layout_valid = 0;
    return core_result_ok();
}

CoreResult mem_console_ui_graph_center_selected_view(MemConsoleState *state) {
    uint32_t selected_node_id = 0u;
    float current_center_x;
    float current_center_y;
    float target_center_x;
    float target_center_y;
    float delta_x;
    float delta_y;
    int i;
    int selected_layout_index = -1;

    if (!state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph focus request" };
    }
    if (state->selected_item_id == 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "no selected node" };
    }
    if (!state->graph_layout_valid || state->graph_layout_node_count == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "graph layout unavailable" };
    }

    for (i = 0; i < state->graph_node_count; ++i) {
        if (state->graph_nodes[i].item_id == state->selected_item_id) {
            selected_node_id = (uint32_t)i + 1u;
            break;
        }
    }
    if (selected_node_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "selected node not present in graph" };
    }

    for (i = 0; i < (int)state->graph_layout_node_count; ++i) {
        if (state->graph_layout_node_layouts[i].node_id == selected_node_id) {
            selected_layout_index = i;
            break;
        }
    }
    if (selected_layout_index < 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "selected node layout not present" };
    }

    current_center_x = state->graph_layout_node_layouts[selected_layout_index].rect.x +
                       (state->graph_layout_node_layouts[selected_layout_index].rect.width * 0.5f);
    current_center_y = state->graph_layout_node_layouts[selected_layout_index].rect.y +
                       (state->graph_layout_node_layouts[selected_layout_index].rect.height * 0.5f);
    target_center_x = state->graph_layout_bounds.x + (state->graph_layout_bounds.width * 0.5f);
    target_center_y = state->graph_layout_bounds.y + (state->graph_layout_bounds.height * 0.5f);
    delta_x = target_center_x - current_center_x;
    delta_y = target_center_y - current_center_y;
    graph_camera_pan_by_screen_delta(&state->graph_viewport, delta_x, delta_y);

    state->graph_layout_valid = 0;
    return core_result_ok();
}
