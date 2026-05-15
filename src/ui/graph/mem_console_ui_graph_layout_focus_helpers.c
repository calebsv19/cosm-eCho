#include "mem_console_ui_graph_layout_focus_helpers.h"

#include <math.h>
#include <string.h>

void mem_console_ui_graph_configure_layout_style(KitGraphStructLayoutStyle *style) {
    if (!style) {
        return;
    }
    kit_graph_struct_layout_style_default(style);
    style->padding = 12.0f;
    style->level_gap = 92.0f;
    style->sibling_gap = 40.0f;
    style->node_width = 34.0f;
    style->node_height = 14.0f;
    style->node_min_width = 26.0f;
    style->node_max_width = 56.0f;
    style->node_padding_x = 2.0f;
    style->label_char_width = 5.2f;
    style->node_label_font_role = CORE_FONT_ROLE_UI_REGULAR;
    style->node_label_text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
    style->measure_text_fn = 0;
    style->measure_text_user = 0;
    style->edge_label_padding_x = 4.0f;
    style->edge_label_height = 13.0f;
    style->edge_label_lane_gap = 2.0f;
}

void mem_console_ui_graph_transpose_layouts_to_horizontal_flow(KitRenderRect bounds,
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

void mem_console_ui_graph_apply_focus_anchor_priority_layout(KitRenderRect bounds,
                                               const MemConsoleState *state,
                                               const KitGraphStructEdge *edges,
                                               uint32_t edge_count,
                                               KitGraphStructNodeLayout *layouts,
                                               uint32_t layout_count) {
    enum {
        FOCUS_LEVEL_MAX = 16
    };
    const float full_turn = 6.28318530718f;
    const float quarter_turn = 1.57079632679f;
    int node_count = 0;
    int depth[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int degree[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int queue[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float score_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float angle_by_node[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float target_center_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float target_center_y[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float center_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float center_y[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int level_members[FOCUS_LEVEL_MAX + 1][MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int level_counts[FOCUS_LEVEL_MAX + 1];
    int root_index = -1;
    int max_depth = 0;
    float edge_density = 0.0f;
    float crowd = 0.0f;
    float root_center_x = 0.0f;
    float root_center_y = 0.0f;
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
    for (i = 0; i < node_count; ++i) {
        depth[i] = -1;
        degree[i] = 0;
        score_x[i] = 0.0f;
        angle_by_node[i] = -quarter_turn;
        target_center_x[i] = focus_layout_center_x(&layouts[i]);
        target_center_y[i] = focus_layout_center_y(&layouts[i]);
        center_x[i] = target_center_x[i];
        center_y[i] = target_center_y[i];
        if (state->graph_nodes[i].item_id == state->selected_item_id) {
            root_index = i;
        } else if (root_index < 0 && state->graph_nodes[i].item_id == state->graph_center_item_id) {
            root_index = i;
        }
    }
    if (root_index < 0) {
        root_index = 0;
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
    }

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

    if (max_depth < 1) {
        max_depth = 1;
    }
    for (i = 0; i < node_count; ++i) {
        int level = depth[i];
        if (level < 0) {
            level = max_depth + 1;
            depth[i] = level;
        }
        if (level > FOCUS_LEVEL_MAX) {
            level = FOCUS_LEVEL_MAX;
            depth[i] = level;
        }
        if (level > max_depth) {
            max_depth = level;
        }
        if (level_counts[level] < MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            int write_index = level_counts[level];
            level_members[level][write_index] = i;
            level_counts[level] += 1;
        }
    }

    root_center_x = bounds.x + (bounds.width * 0.5f);
    root_center_y = bounds.y + (bounds.height * 0.5f);
    target_center_x[root_index] = root_center_x;
    target_center_y[root_index] = root_center_y;
    focus_layout_set_center(bounds,
                            &layouts[root_index],
                            root_center_x,
                            root_center_y,
                            bounds.y + 4.0f);
    center_x[root_index] = focus_layout_center_x(&layouts[root_index]);
    center_y[root_index] = focus_layout_center_y(&layouts[root_index]);

    for (i = 0; i < node_count; ++i) {
        if (i == root_index) {
            continue;
        }
        score_x[i] = ((float)depth[i] * full_turn) +
                     focus_hash_signed((uint32_t)(i + 1) * 2654435761u);
    }

    for (i = 1; i <= max_depth && i <= FOCUS_LEVEL_MAX; ++i) {
        int level_count = level_counts[i];
        int slot;
        int sorted_indices[MEM_CONSOLE_GRAPH_NODE_LIMIT];
        int ring_capacity = 8 + (i * 5);
        int ring_count = 1;
        float radius_x = 60.0f + ((float)(i - 1) * (42.0f + (crowd * 8.0f)));
        float radius_y = 46.0f + ((float)(i - 1) * (34.0f + (crowd * 6.0f)));

        if (level_count <= 0) {
            continue;
        }
        if (ring_capacity > 24) {
            ring_capacity = 24;
        }
        if (ring_capacity < 8) {
            ring_capacity = 8;
        }
        if (level_count > ring_capacity) {
            ring_count = (level_count + ring_capacity - 1) / ring_capacity;
        }
        for (slot = 0; slot < level_count; ++slot) {
            int node_index = level_members[i][slot];
            float parent_bias = 0.0f;
            uint32_t edge_index;

            for (edge_index = 0u; edge_index < edge_count; ++edge_index) {
                int from_index = (int)edges[edge_index].from_id - 1;
                int to_index = (int)edges[edge_index].to_id - 1;
                int parent_index = -1;

                if (from_index < 0 || to_index < 0 ||
                    from_index >= node_count || to_index >= node_count) {
                    continue;
                }
                if (from_index == node_index && depth[to_index] == depth[node_index] - 1) {
                    parent_index = to_index;
                } else if (to_index == node_index && depth[from_index] == depth[node_index] - 1) {
                    parent_index = from_index;
                }
                if (parent_index >= 0) {
                    parent_bias = angle_by_node[parent_index];
                    break;
                }
            }
            score_x[node_index] = parent_bias +
                                  (focus_hash_signed((uint32_t)(node_index + 13) * 2246822519u) * 0.32f);
            sorted_indices[slot] = node_index;
        }
        focus_layout_sort_indices_by_score(sorted_indices, level_count, score_x);

        for (slot = 0; slot < level_count; ++slot) {
            int node_index = sorted_indices[slot];
            int ring_index = slot / ring_capacity;
            int ring_slot = slot % ring_capacity;
            int slots_in_ring = level_count - (ring_index * ring_capacity);
            float ring_radius_x;
            float ring_radius_y;
            float angle;

            if (slots_in_ring > ring_capacity) {
                slots_in_ring = ring_capacity;
            }
            if (slots_in_ring < 1) {
                slots_in_ring = 1;
            }

            ring_radius_x = radius_x + ((float)ring_index * (24.0f + (crowd * 6.0f)));
            ring_radius_y = radius_y + ((float)ring_index * (20.0f + (crowd * 5.0f)));
            if (ring_count > 1) {
                ring_radius_x += ((float)ring_count - 1.0f) * 3.0f;
                ring_radius_y += ((float)ring_count - 1.0f) * 2.0f;
            }

            if (slots_in_ring == 1) {
                angle = -quarter_turn;
            } else {
                angle = -quarter_turn + ((((float)ring_slot) / (float)slots_in_ring) * full_turn);
            }

            angle += focus_hash_signed((uint32_t)(node_index + 3) * 3266489917u) *
                     (0.05f + (((float)i - 1.0f) * 0.01f));
            angle_by_node[node_index] = angle;
            target_center_x[node_index] = root_center_x + (cosf(angle) * ring_radius_x);
            target_center_y[node_index] = root_center_y + (sinf(angle) * ring_radius_y);
        }
    }

    for (i = 0; i < node_count; ++i) {
        if (i == root_index) {
            continue;
        }
        center_x[i] = target_center_x[i];
        center_y[i] = target_center_y[i];
        focus_layout_set_center(bounds,
                                &layouts[i],
                                center_x[i],
                                center_y[i],
                                bounds.y + 8.0f);
        center_x[i] = focus_layout_center_x(&layouts[i]);
        center_y[i] = focus_layout_center_y(&layouts[i]);
    }

    for (pass = 0; pass < 8; ++pass) {
        int a;
        int b;
        for (a = 0; a < node_count; ++a) {
            if (a == root_index) {
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

                if (b == root_index) {
                    continue;
                }

                dx = center_x[a] - center_x[b];
                dy = center_y[a] - center_y[b];
                min_dx = (layouts[a].rect.width + layouts[b].rect.width) * 0.5f + 12.0f + (crowd * 9.0f);
                min_dy = (layouts[a].rect.height + layouts[b].rect.height) * 0.5f + 10.0f + (crowd * 6.0f);
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
            float dx_root;
            float dy_root;
            float root_dist_sq;
            float min_root_radius;

            if (a == root_index) {
                continue;
            }
            dx_root = center_x[a] - root_center_x;
            dy_root = center_y[a] - root_center_y;
            root_dist_sq = (dx_root * dx_root) + (dy_root * dy_root);
            min_root_radius = 34.0f + (layouts[a].rect.width * 0.5f) + (crowd * 8.0f);
            if (root_dist_sq < (min_root_radius * min_root_radius)) {
                float root_dist = sqrtf(root_dist_sq);
                float nx = 0.0f;
                float ny = -1.0f;
                float push = min_root_radius;

                if (root_dist > 0.01f) {
                    nx = dx_root / root_dist;
                    ny = dy_root / root_dist;
                    push = min_root_radius - root_dist;
                }
                center_x[a] += nx * push;
                center_y[a] += ny * push;
            }
        }

        for (i = 0; i < node_count; ++i) {
            if (i == root_index) {
                continue;
            }
            center_x[i] += (target_center_x[i] - center_x[i]) * 0.26f;
            center_y[i] += (target_center_y[i] - center_y[i]) * 0.26f;
            focus_layout_set_center(bounds,
                                    &layouts[i],
                                    center_x[i],
                                    center_y[i],
                                    bounds.y + 8.0f);
            center_x[i] = focus_layout_center_x(&layouts[i]);
            center_y[i] = focus_layout_center_y(&layouts[i]);
        }
    }

    focus_layout_set_center(bounds,
                            &layouts[root_index],
                            root_center_x,
                            root_center_y,
                            bounds.y + 4.0f);
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

void mem_console_ui_graph_filter_edges_for_visible_layout_nodes(KitRenderRect bounds,
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

uint64_t mem_console_ui_graph_preview_layout_signature(const MemConsoleState *state, KitRenderRect bounds) {
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
