#include "mem_console_ui_graph_internal.h"

#include <math.h>
#include <string.h>

static float web_layout_center_x(const KitGraphStructNodeLayout *layout) {
    if (!layout) {
        return 0.0f;
    }
    return layout->rect.x + (layout->rect.width * 0.5f);
}

static float web_layout_center_y(const KitGraphStructNodeLayout *layout) {
    if (!layout) {
        return 0.0f;
    }
    return layout->rect.y + (layout->rect.height * 0.5f);
}

static void web_layout_set_center(KitRenderRect bounds,
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

    min_center_x = bounds.x + 8.0f + (layout->rect.width * 0.5f);
    max_center_x = bounds.x + bounds.width - 8.0f - (layout->rect.width * 0.5f);
    min_center_y = bounds.y + 8.0f + (layout->rect.height * 0.5f);
    max_center_y = bounds.y + bounds.height - 8.0f - (layout->rect.height * 0.5f);
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

static int web_find_root_index(const MemConsoleState *state, int node_count) {
    int i;

    if (!state || node_count <= 0) {
        return -1;
    }
    for (i = 0; i < node_count && i < state->graph_node_count; ++i) {
        if (state->graph_nodes[i].item_id == state->selected_item_id) {
            return i;
        }
    }
    for (i = 0; i < node_count && i < state->graph_node_count; ++i) {
        if (state->graph_nodes[i].item_id == state->graph_center_item_id) {
            return i;
        }
    }
    return 0;
}

static int web_nodes_cross_project(const MemConsoleState *state, int a, int b) {
    const char *project_a;
    const char *project_b;

    if (!state ||
        a < 0 || b < 0 ||
        a >= state->graph_node_count ||
        b >= state->graph_node_count) {
        return 0;
    }
    project_a = state->graph_nodes[a].project_key;
    project_b = state->graph_nodes[b].project_key;
    if (!project_a[0] || !project_b[0]) {
        return 0;
    }
    return strcmp(project_a, project_b) != 0 ? 1 : 0;
}

static void web_compute_degree_and_bridge_score(const MemConsoleState *state,
                                                const KitGraphStructEdge *edges,
                                                uint32_t edge_count,
                                                int node_count,
                                                int root_index,
                                                int *degree,
                                                int *cross_project_degree,
                                                int *bridge_score) {
    uint32_t edge_index;
    int i;

    if (!state || !edges || !degree || !cross_project_degree || !bridge_score) {
        return;
    }
    for (i = 0; i < node_count; ++i) {
        degree[i] = 0;
        cross_project_degree[i] = 0;
        bridge_score[i] = 0;
    }

    for (edge_index = 0u; edge_index < edge_count; ++edge_index) {
        int from_index = (int)edges[edge_index].from_id - 1;
        int to_index = (int)edges[edge_index].to_id - 1;
        int cross_project;

        if (from_index < 0 || to_index < 0 ||
            from_index >= node_count || to_index >= node_count ||
            from_index == to_index) {
            continue;
        }
        degree[from_index] += 1;
        degree[to_index] += 1;
        cross_project = web_nodes_cross_project(state, from_index, to_index);
        if (cross_project) {
            cross_project_degree[from_index] += 1;
            cross_project_degree[to_index] += 1;
        }
    }

    for (i = 0; i < node_count; ++i) {
        bridge_score[i] = (degree[i] * 3) + (cross_project_degree[i] * 6);
        if (i == root_index) {
            bridge_score[i] += 1000;
        }
        if (state->graph_nodes[i].pinned) {
            bridge_score[i] += 4;
        }
        if (state->graph_nodes[i].canonical) {
            bridge_score[i] += 4;
        }
    }
}

static void web_discover_components(const KitGraphStructEdge *edges,
                                    uint32_t edge_count,
                                    int node_count,
                                    int root_index,
                                    int *component_of,
                                    int *component_sizes,
                                    int *component_has_root,
                                    int *out_component_count) {
    int queue[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int component_count = 0;
    int i;

    if (!edges || !component_of || !component_sizes || !component_has_root || !out_component_count) {
        return;
    }
    for (i = 0; i < node_count; ++i) {
        component_of[i] = -1;
    }

    for (i = 0; i < node_count; ++i) {
        int queue_read = 0;
        int queue_write = 1;
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

        while (queue_read < queue_write) {
            int current_index = queue[queue_read++];
            uint32_t edge_index;

            component_sizes[component_id] += 1;
            if (current_index == root_index) {
                component_has_root[component_id] = 1;
            }

            for (edge_index = 0u; edge_index < edge_count; ++edge_index) {
                int from_index = (int)edges[edge_index].from_id - 1;
                int to_index = (int)edges[edge_index].to_id - 1;
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

    *out_component_count = component_count;
}

static void web_sort_components_by_root_size(const int *component_sizes,
                                             const int *component_has_root,
                                             int *component_indices,
                                             int component_count) {
    int i;

    if (!component_sizes || !component_indices || component_count <= 1) {
        return;
    }

    for (i = 1; i < component_count; ++i) {
        int key = component_indices[i];
        int j = i - 1;

        while (j >= 0) {
            int probe = component_indices[j];
            int shift = 0;

            if (component_has_root && component_has_root[probe] < component_has_root[key]) {
                shift = 1;
            } else if (component_has_root &&
                       component_has_root[probe] == component_has_root[key] &&
                       component_sizes[probe] < component_sizes[key]) {
                shift = 1;
            } else if ((!component_has_root || component_has_root[probe] == component_has_root[key]) &&
                       component_sizes[probe] == component_sizes[key] &&
                       probe > key) {
                shift = 1;
            }
            if (!shift) {
                break;
            }
            component_indices[j + 1] = probe;
            j -= 1;
        }
        component_indices[j + 1] = key;
    }
}

static void web_sort_nodes_by_bridge_score(const int *bridge_score,
                                           const int *degree,
                                           int *node_indices,
                                           int count) {
    int i;

    if (!bridge_score || !degree || !node_indices || count <= 1) {
        return;
    }

    for (i = 1; i < count; ++i) {
        int key = node_indices[i];
        int j = i - 1;

        while (j >= 0) {
            int probe = node_indices[j];
            int shift = 0;

            if (bridge_score[probe] < bridge_score[key]) {
                shift = 1;
            } else if (bridge_score[probe] == bridge_score[key] && degree[probe] < degree[key]) {
                shift = 1;
            } else if (bridge_score[probe] == bridge_score[key] &&
                       degree[probe] == degree[key] &&
                       probe > key) {
                shift = 1;
            }
            if (!shift) {
                break;
            }
            node_indices[j + 1] = probe;
            j -= 1;
        }
        node_indices[j + 1] = key;
    }
}

static void web_component_anchor_for_rank(KitRenderRect bounds,
                                          int rank,
                                          int secondary_count,
                                          float *out_x,
                                          float *out_y) {
    static const float k_angles[] = {
        -1.57079632679f,
        0.0f,
        1.57079632679f,
        3.14159265359f,
        -0.78539816339f,
        0.78539816339f,
        2.35619449019f,
        -2.35619449019f
    };
    const float full_turn = 6.28318530718f;
    int angle_count = (int)(sizeof(k_angles) / sizeof(k_angles[0]));
    float center_x = bounds.x + (bounds.width * 0.5f);
    float center_y = bounds.y + (bounds.height * 0.5f);
    float angle;
    float ring = 1.0f;
    float radius_x;
    float radius_y;

    if (!out_x || !out_y) {
        return;
    }
    if (rank < 0 || secondary_count <= 0) {
        *out_x = center_x;
        *out_y = center_y;
        return;
    }

    if (rank < angle_count) {
        angle = k_angles[rank];
    } else {
        ring += (float)(rank / angle_count);
        angle = -1.57079632679f + (((float)(rank % angle_count) / (float)angle_count) * full_turn);
    }
    radius_x = (bounds.width * (secondary_count <= 3 ? 0.29f : 0.33f)) * ring;
    radius_y = (bounds.height * (secondary_count <= 3 ? 0.24f : 0.27f)) * ring;

    *out_x = center_x + (cosf(angle) * radius_x);
    *out_y = center_y + (sinf(angle) * radius_y);
}

static void web_place_component_nodes(KitRenderRect bounds,
                                      const int *component_of,
                                      const int *bridge_score,
                                      const int *degree,
                                      int component_id,
                                      float anchor_x,
                                      float anchor_y,
                                      KitGraphStructNodeLayout *layouts,
                                      int node_count,
                                      float *target_center_x,
                                      float *target_center_y) {
    int node_indices[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int count = 0;
    int slot;
    const float full_turn = 6.28318530718f;

    if (!component_of || !bridge_score || !degree || !layouts ||
        !target_center_x || !target_center_y) {
        return;
    }

    for (slot = 0; slot < node_count; ++slot) {
        if (component_of[slot] == component_id) {
            node_indices[count++] = slot;
        }
    }
    if (count <= 0) {
        return;
    }
    web_sort_nodes_by_bridge_score(bridge_score, degree, node_indices, count);

    for (slot = 0; slot < count; ++slot) {
        int node_index = node_indices[slot];
        float x = anchor_x;
        float y = anchor_y;

        if (slot > 0) {
            int ring_slot = slot - 1;
            int ring = ring_slot / 8 + 1;
            int pos = ring_slot % 8;
            float angle = -1.57079632679f + (((float)pos / 8.0f) * full_turn);
            float radius_x = 66.0f + ((float)(ring - 1) * 48.0f);
            float radius_y = 40.0f + ((float)(ring - 1) * 34.0f);
            float bridge_bias = bridge_score[node_index] > 8 ? 0.72f : 1.0f;

            x += cosf(angle) * radius_x * bridge_bias;
            y += sinf(angle) * radius_y * bridge_bias;
            if ((slot % 2) == 0) {
                y += 9.0f;
            }
        }

        web_layout_set_center(bounds, &layouts[node_index], x, y);
        target_center_x[node_index] = web_layout_center_x(&layouts[node_index]);
        target_center_y[node_index] = web_layout_center_y(&layouts[node_index]);
    }
}

static void web_relax_node_overlap(KitRenderRect bounds,
                                   const int *component_of,
                                   int root_index,
                                   KitGraphStructNodeLayout *layouts,
                                   int node_count,
                                   float *center_x,
                                   float *center_y) {
    int pass;

    if (!component_of || !layouts || !center_x || !center_y) {
        return;
    }

    for (pass = 0; pass < 7; ++pass) {
        int a;
        int b;

        for (a = 0; a < node_count; ++a) {
            for (b = a + 1; b < node_count; ++b) {
                float dx = center_x[a] - center_x[b];
                float dy = center_y[a] - center_y[b];
                float same_component = component_of[a] == component_of[b] ? 1.0f : 0.0f;
                float min_dx = (layouts[a].rect.width + layouts[b].rect.width) * 0.5f +
                               (same_component ? 16.0f : 34.0f);
                float min_dy = (layouts[a].rect.height + layouts[b].rect.height) * 0.5f +
                               (same_component ? 13.0f : 26.0f);

                if (fabsf(dx) >= min_dx || fabsf(dy) >= min_dy) {
                    continue;
                }

                if (fabsf(dx) < min_dx) {
                    float dir_x = dx >= 0.0f ? 1.0f : -1.0f;
                    float shift_x = (min_dx - fabsf(dx)) * (same_component ? 0.18f : 0.34f);
                    if (a != root_index) {
                        center_x[a] += shift_x * dir_x;
                    }
                    if (b != root_index) {
                        center_x[b] -= shift_x * dir_x;
                    }
                }
                if (fabsf(dy) < min_dy) {
                    float dir_y = dy >= 0.0f ? 1.0f : -1.0f;
                    float shift_y = (min_dy - fabsf(dy)) * (same_component ? 0.26f : 0.38f);
                    if (a != root_index) {
                        center_y[a] += shift_y * dir_y;
                    }
                    if (b != root_index) {
                        center_y[b] -= shift_y * dir_y;
                    }
                }
            }
        }

        for (a = 0; a < node_count; ++a) {
            web_layout_set_center(bounds, &layouts[a], center_x[a], center_y[a]);
            center_x[a] = web_layout_center_x(&layouts[a]);
            center_y[a] = web_layout_center_y(&layouts[a]);
        }
    }
}

void apply_free_web_layout(KitRenderRect bounds,
                           const MemConsoleState *state,
                           const KitGraphStructEdge *edges,
                           uint32_t edge_count,
                           KitGraphStructNodeLayout *layouts,
                           uint32_t layout_count) {
    int node_count;
    int root_index;
    int component_count = 0;
    int component_of[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int component_sizes[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int component_has_root[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int component_indices[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int degree[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int cross_project_degree[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int bridge_score[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float target_center_x[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    float target_center_y[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int i;

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

    root_index = web_find_root_index(state, node_count);
    if (root_index < 0) {
        return;
    }

    web_compute_degree_and_bridge_score(state,
                                        edges,
                                        edge_count,
                                        node_count,
                                        root_index,
                                        degree,
                                        cross_project_degree,
                                        bridge_score);
    web_discover_components(edges,
                            edge_count,
                            node_count,
                            root_index,
                            component_of,
                            component_sizes,
                            component_has_root,
                            &component_count);

    for (i = 0; i < component_count; ++i) {
        component_indices[i] = i;
    }
    web_sort_components_by_root_size(component_sizes,
                                     component_has_root,
                                     component_indices,
                                     component_count);

    for (i = 0; i < node_count; ++i) {
        target_center_x[i] = web_layout_center_x(&layouts[i]);
        target_center_y[i] = web_layout_center_y(&layouts[i]);
    }

    for (i = 0; i < component_count; ++i) {
        int component_id = component_indices[i];
        int secondary_rank = component_has_root[component_id] ? -1 : i - 1;
        int secondary_count = component_count - 1;
        float anchor_x;
        float anchor_y;

        if (component_has_root[component_id]) {
            anchor_x = bounds.x + (bounds.width * 0.5f);
            anchor_y = bounds.y + (bounds.height * 0.5f);
        } else {
            web_component_anchor_for_rank(bounds,
                                          secondary_rank,
                                          secondary_count,
                                          &anchor_x,
                                          &anchor_y);
        }

        web_place_component_nodes(bounds,
                                  component_of,
                                  bridge_score,
                                  degree,
                                  component_id,
                                  anchor_x,
                                  anchor_y,
                                  layouts,
                                  node_count,
                                  target_center_x,
                                  target_center_y);
    }

    target_center_x[root_index] = bounds.x + (bounds.width * 0.5f);
    target_center_y[root_index] = bounds.y + (bounds.height * 0.5f);
    web_layout_set_center(bounds,
                          &layouts[root_index],
                          target_center_x[root_index],
                          target_center_y[root_index]);
    target_center_x[root_index] = web_layout_center_x(&layouts[root_index]);
    target_center_y[root_index] = web_layout_center_y(&layouts[root_index]);

    web_relax_node_overlap(bounds,
                           component_of,
                           root_index,
                           layouts,
                           node_count,
                           target_center_x,
                           target_center_y);
}
