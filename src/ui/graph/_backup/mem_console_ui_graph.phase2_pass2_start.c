#include "mem_console_ui_graph_internal.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const GraphEdgeLegendEntry k_graph_edge_legend_entries[] = {
    { "next_step", "NEXT", { 120, 214, 250, 255 } },
    { "child_of", "CHILD", { 184, 212, 120, 255 } },
    { "supports", "SUPPORTS", { 64, 208, 128, 255 } },
    { "depends_on", "DEPENDS", { 232, 162, 56, 255 } },
    { "references", "REFS", { 74, 184, 255, 255 } },
    { "summarizes", "SUMMARY", { 178, 120, 255, 255 } },
    { "related", "RELATED", { 168, 178, 196, 255 } },
    { "implements", "IMPLEMENTS", { 156, 214, 78, 255 } },
    { "blocks", "BLOCKS", { 230, 92, 92, 255 } },
    { "contradicts", "CONTRADICTS", { 255, 96, 152, 255 } }
};

const GraphEdgeLegendEntry *graph_edge_legend_entry_for_kind(const char *kind) {
    uint32_t i;

    if (!kind || !kind[0]) {
        kind = "related";
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_graph_edge_legend_entries) / sizeof(k_graph_edge_legend_entries[0])); ++i) {
        if (strcmp(k_graph_edge_legend_entries[i].kind, kind) == 0) {
            return &k_graph_edge_legend_entries[i];
        }
    }
    return 0;
}

uint32_t graph_edge_legend_entry_count(void) {
    return (uint32_t)(sizeof(k_graph_edge_legend_entries) / sizeof(k_graph_edge_legend_entries[0]));
}

const GraphEdgeLegendEntry *graph_edge_legend_entry_at(uint32_t index) {
    if (index >= graph_edge_legend_entry_count()) {
        return 0;
    }
    return &k_graph_edge_legend_entries[index];
}

KitRenderColor graph_edge_color_for_kind(const char *kind) {
    const GraphEdgeLegendEntry *entry = graph_edge_legend_entry_for_kind(kind);
    if (entry) {
        return entry->color;
    }
    return (KitRenderColor){ 188, 196, 210, 255 };
}

const char *graph_edge_display_label_for_kind(const char *kind) {
    const GraphEdgeLegendEntry *entry = graph_edge_legend_entry_for_kind(kind);
    if (entry) {
        return entry->label;
    }
    if (!kind || !kind[0]) {
        return "RELATED";
    }
    return kind;
}

static int graph_text_has_prefix(const char *text, const char *prefix) {
    size_t prefix_len;

    if (!text || !prefix) {
        return 0;
    }
    prefix_len = strlen(prefix);
    if (prefix_len == 0u) {
        return 0;
    }
    return strncmp(text, prefix, prefix_len) == 0 ? 1 : 0;
}

static int graph_project_char_is_separator(unsigned char c) {
    return c == '_' || c == '-' || c == '/' || c == '.';
}

void graph_format_project_display_name(const char *project_key,
                                       char *out_text,
                                       size_t out_cap) {
    size_t write_index = 0u;
    int start_of_word = 1;
    size_t i = 0u;

    if (!out_text || out_cap == 0u) {
        return;
    }

    out_text[0] = '\0';
    if (!project_key || project_key[0] == '\0') {
        (void)snprintf(out_text, out_cap, "Misc");
        return;
    }

    while (project_key[i] != '\0' && write_index + 1u < out_cap) {
        unsigned char c = (unsigned char)project_key[i];
        if (graph_project_char_is_separator(c)) {
            if (write_index > 0u && out_text[write_index - 1u] != ' ') {
                out_text[write_index++] = ' ';
            }
            start_of_word = 1;
            i += 1u;
            continue;
        }
        if (start_of_word) {
            out_text[write_index++] = (char)toupper(c);
            start_of_word = 0;
        } else {
            out_text[write_index++] = (char)tolower(c);
        }
        i += 1u;
    }

    while (write_index > 0u && out_text[write_index - 1u] == ' ') {
        write_index -= 1u;
    }
    out_text[write_index] = '\0';

    if (out_text[0] == '\0') {
        (void)snprintf(out_text, out_cap, "Misc");
    }
}

GraphBucketRole graph_bucket_role_for_node(const MemConsoleGraphNode *node) {
    if (!node) {
        return GRAPH_BUCKET_ROLE_NONE;
    }
    if (graph_text_has_prefix(node->stable_id, "scope-")) {
        return GRAPH_BUCKET_ROLE_SCOPE;
    }
    if (graph_text_has_prefix(node->stable_id, "plans-")) {
        return GRAPH_BUCKET_ROLE_PLANS;
    }
    if (graph_text_has_prefix(node->stable_id, "decisions-")) {
        return GRAPH_BUCKET_ROLE_DECISIONS;
    }
    if (graph_text_has_prefix(node->stable_id, "issues-")) {
        return GRAPH_BUCKET_ROLE_ISSUES;
    }
    if (graph_text_has_prefix(node->stable_id, "misc-")) {
        return GRAPH_BUCKET_ROLE_MISC;
    }
    return GRAPH_BUCKET_ROLE_NONE;
}

const char *graph_bucket_role_label(GraphBucketRole role) {
    switch (role) {
        case GRAPH_BUCKET_ROLE_SCOPE: return "SCOPE";
        case GRAPH_BUCKET_ROLE_PLANS: return "PLANS";
        case GRAPH_BUCKET_ROLE_DECISIONS: return "DECISIONS";
        case GRAPH_BUCKET_ROLE_ISSUES: return "ISSUES";
        case GRAPH_BUCKET_ROLE_MISC: return "MISC";
        default: break;
    }
    return "";
}

KitRenderColor graph_bucket_border_color(GraphBucketRole role) {
    switch (role) {
        case GRAPH_BUCKET_ROLE_SCOPE: return (KitRenderColor){ 110, 170, 255, 255 };
        case GRAPH_BUCKET_ROLE_PLANS: return (KitRenderColor){ 128, 224, 152, 255 };
        case GRAPH_BUCKET_ROLE_DECISIONS: return (KitRenderColor){ 244, 190, 92, 255 };
        case GRAPH_BUCKET_ROLE_ISSUES: return (KitRenderColor){ 244, 128, 128, 255 };
        case GRAPH_BUCKET_ROLE_MISC: return (KitRenderColor){ 178, 178, 196, 255 };
        default: break;
    }
    return (KitRenderColor){ 188, 196, 210, 255 };
}

void graph_node_project_key(const MemConsoleGraphNode *node,
                            char *out_key,
                            size_t out_cap) {
    const char *prefixes[] = { "scope-", "plans-", "decisions-", "issues-", "misc-" };
    uint32_t i;

    if (!out_key || out_cap == 0u) {
        return;
    }
    out_key[0] = '\0';
    if (!node) {
        (void)snprintf(out_key, out_cap, "misc");
        return;
    }
    if (node->project_key[0] != '\0') {
        (void)snprintf(out_key, out_cap, "%s", node->project_key);
        return;
    }
    for (i = 0u; i < (uint32_t)(sizeof(prefixes) / sizeof(prefixes[0])); ++i) {
        const char *prefix = prefixes[i];
        size_t prefix_len = strlen(prefix);
        if (strncmp(node->stable_id, prefix, prefix_len) == 0 && node->stable_id[prefix_len] != '\0') {
            (void)snprintf(out_key, out_cap, "%s", node->stable_id + prefix_len);
            return;
        }
    }
    (void)snprintf(out_key, out_cap, "misc");
}

int graph_find_project_pod(GraphProjectPod *pods,
                           int pod_count,
                           const char *key) {
    int i;

    if (!pods || !key || key[0] == '\0') {
        return -1;
    }
    for (i = 0; i < pod_count; ++i) {
        if (strcmp(pods[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

static GraphBucketRole graph_guess_role_from_node_kind(const MemConsoleGraphNode *node) {
    if (!node) {
        return GRAPH_BUCKET_ROLE_MISC;
    }
    if (strstr(node->kind, "plan") != 0) {
        return GRAPH_BUCKET_ROLE_PLANS;
    }
    if (strstr(node->kind, "decision") != 0) {
        return GRAPH_BUCKET_ROLE_DECISIONS;
    }
    if (strstr(node->kind, "issue") != 0) {
        return GRAPH_BUCKET_ROLE_ISSUES;
    }
    if (strstr(node->kind, "scope") != 0) {
        return GRAPH_BUCKET_ROLE_SCOPE;
    }
    return GRAPH_BUCKET_ROLE_MISC;
}

static int graph_role_to_lane_index(GraphBucketRole role) {
    switch (role) {
        case GRAPH_BUCKET_ROLE_PLANS: return 0;
        case GRAPH_BUCKET_ROLE_DECISIONS: return 1;
        case GRAPH_BUCKET_ROLE_ISSUES: return 2;
        case GRAPH_BUCKET_ROLE_MISC: return 3;
        case GRAPH_BUCKET_ROLE_SCOPE: return 0;
        default: break;
    }
    return 3;
}

static GraphBucketRole graph_pick_lane_role_for_node(const MemConsoleState *state,
                                                     int node_index,
                                                     const uint8_t node_in_pod[MEM_CONSOLE_GRAPH_NODE_LIMIT]) {
    int e;
    int scores[6] = {0, 0, 0, 0, 0, 0};
    GraphBucketRole best_role = GRAPH_BUCKET_ROLE_NONE;
    int best_score = 0;

    if (!state || node_index < 0 || node_index >= state->graph_node_count) {
        return GRAPH_BUCKET_ROLE_MISC;
    }

    for (e = 0; e < state->graph_edge_count; ++e) {
        int from_index = state->graph_edges[e].from_index;
        int to_index = state->graph_edges[e].to_index;
        int other_index = -1;
        int weight = 1;
        GraphBucketRole other_role;

        if (from_index == node_index) {
            other_index = to_index;
        } else if (to_index == node_index) {
            other_index = from_index;
        } else {
            continue;
        }
        if (other_index < 0 ||
            other_index >= state->graph_node_count ||
            other_index >= MEM_CONSOLE_GRAPH_NODE_LIMIT ||
            !node_in_pod[other_index]) {
            continue;
        }
        other_role = graph_bucket_role_for_node(&state->graph_nodes[other_index]);
        if (other_role == GRAPH_BUCKET_ROLE_NONE) {
            continue;
        }
        if (graph_edge_is_hierarchy_kind(state->graph_edges[e].kind)) {
            weight = 3;
        }
        scores[(int)other_role] += weight;
    }

    {
        int role_index;
        for (role_index = (int)GRAPH_BUCKET_ROLE_SCOPE; role_index <= (int)GRAPH_BUCKET_ROLE_MISC; ++role_index) {
            if (scores[role_index] > best_score) {
                best_score = scores[role_index];
                best_role = (GraphBucketRole)role_index;
            }
        }
    }

    if (best_role == GRAPH_BUCKET_ROLE_NONE) {
        best_role = graph_guess_role_from_node_kind(&state->graph_nodes[node_index]);
    }
    if (best_role == GRAPH_BUCKET_ROLE_NONE) {
        best_role = GRAPH_BUCKET_ROLE_MISC;
    }
    return best_role;
}

static void graph_place_nodes_in_lane(KitGraphStructNodeLayout *layouts,
                                      const int *indices,
                                      int count,
                                      KitRenderRect lane_rect) {
    int grid_cols = 1;
    int idx;
    int grid_rows;

    if (!layouts || !indices || count <= 0 || lane_rect.width <= 2.0f || lane_rect.height <= 2.0f) {
        return;
    }

    while (grid_cols * grid_cols < count) {
        grid_cols += 1;
    }
    grid_rows = (count + grid_cols - 1) / grid_cols;
    if (grid_rows < 1) {
        grid_rows = 1;
    }

    for (idx = 0; idx < count; ++idx) {
        int node_index = indices[idx];
        int grid_row = idx / grid_cols;
        int grid_col = idx % grid_cols;
        float cell_w = lane_rect.width / (float)grid_cols;
        float cell_h = lane_rect.height / (float)grid_rows;
        float node_w = layouts[node_index].rect.width;
        float node_h = layouts[node_index].rect.height;
        float cx = lane_rect.x + ((float)grid_col + 0.5f) * cell_w;
        float cy = lane_rect.y + ((float)grid_row + 0.5f) * cell_h;

        layouts[node_index].rect.x = graph_clampf(cx - (node_w * 0.5f),
                                                  lane_rect.x + 1.0f,
                                                  lane_rect.x + lane_rect.width - node_w - 1.0f);
        layouts[node_index].rect.y = graph_clampf(cy - (node_h * 0.5f),
                                                  lane_rect.y + 1.0f,
                                                  lane_rect.y + lane_rect.height - node_h - 1.0f);
    }
}

int graph_collect_project_pods(const MemConsoleState *state,
                               const KitGraphStructNodeLayout *layouts,
                               uint32_t layout_count,
                               GraphProjectPod *out_pods,
                               int out_pod_cap) {
    uint32_t node_count = 0u;
    uint32_t i;
    int pod_count = 0;

    if (!state || !out_pods || out_pod_cap <= 0) {
        return 0;
    }

    node_count = (uint32_t)state->graph_node_count;
    if (node_count > layout_count) {
        node_count = layout_count;
    }

    for (i = 0u; i < node_count; ++i) {
        char key[64];
        int pod_index;

        graph_node_project_key(&state->graph_nodes[i], key, sizeof(key));
        pod_index = graph_find_project_pod(out_pods, pod_count, key);
        if (pod_index < 0) {
            if (pod_count >= out_pod_cap) {
                continue;
            }
            pod_index = pod_count;
            memset(&out_pods[pod_index], 0, sizeof(out_pods[pod_index]));
            (void)snprintf(out_pods[pod_index].key, sizeof(out_pods[pod_index].key), "%s", key);
            out_pods[pod_index].bounds = (KitRenderRect){ 0.0f, 0.0f, 0.0f, 0.0f };
            pod_count += 1;
        }
        if (out_pods[pod_index].node_count < MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            int write_index = out_pods[pod_index].node_count;
            out_pods[pod_index].node_indices[write_index] = (int)i;
            out_pods[pod_index].node_count += 1;
        }
    }

    if (layouts) {
        int p;
        for (p = 0; p < pod_count; ++p) {
            int n;
            for (n = 0; n < out_pods[p].node_count; ++n) {
                int node_index = out_pods[p].node_indices[n];
                KitRenderRect rect;
                if (node_index < 0 || (uint32_t)node_index >= layout_count) {
                    continue;
                }
                rect = layouts[node_index].rect;
                if (n == 0) {
                    out_pods[p].bounds = rect;
                } else {
                    float left = out_pods[p].bounds.x < rect.x ? out_pods[p].bounds.x : rect.x;
                    float top = out_pods[p].bounds.y < rect.y ? out_pods[p].bounds.y : rect.y;
                    float right_a = out_pods[p].bounds.x + out_pods[p].bounds.width;
                    float right_b = rect.x + rect.width;
                    float bottom_a = out_pods[p].bounds.y + out_pods[p].bounds.height;
                    float bottom_b = rect.y + rect.height;
                    float right = right_a > right_b ? right_a : right_b;
                    float bottom = bottom_a > bottom_b ? bottom_a : bottom_b;
                    out_pods[p].bounds.x = left;
                    out_pods[p].bounds.y = top;
                    out_pods[p].bounds.width = right - left;
                    out_pods[p].bounds.height = bottom - top;
                }
            }
        }
    }

    return pod_count;
}

void apply_project_pod_layout(KitRenderRect bounds,
                              const MemConsoleState *state,
                              KitGraphStructNodeLayout *layouts,
                              uint32_t layout_count) {
    GraphProjectPod pods[MEM_CONSOLE_GRAPH_PROJECT_POD_LIMIT];
    int pod_count;
    int cols;
    int rows;
    float gap_x = 30.0f;
    float gap_y = 28.0f;
    float cell_w;
    float cell_h;
    int p;

    if (!state || !layouts || layout_count == 0u) {
        return;
    }

    memset(pods, 0, sizeof(pods));
    pod_count = graph_collect_project_pods(state, 0, layout_count, pods, MEM_CONSOLE_GRAPH_PROJECT_POD_LIMIT);
    if (pod_count <= 0) {
        return;
    }

    cols = 1;
    while (cols * cols < pod_count) {
        cols += 1;
    }
    rows = (pod_count + cols - 1) / cols;
    if (rows < 1) {
        rows = 1;
    }

    cell_w = (bounds.width - (gap_x * (float)(cols + 1))) / (float)cols;
    cell_h = (bounds.height - (gap_y * (float)(rows + 1))) / (float)rows;
    if (cell_w < 96.0f) {
        cell_w = 96.0f;
    }
    if (cell_h < 84.0f) {
        cell_h = 84.0f;
    }

    for (p = 0; p < pod_count; ++p) {
        int row = p / cols;
        int col = p % cols;
        KitRenderRect pod_rect = {
            bounds.x + gap_x + ((float)col * (cell_w + gap_x)),
            bounds.y + gap_y + ((float)row * (cell_h + gap_y)),
            cell_w,
            cell_h
        };
        int n;
        uint8_t node_in_pod[MEM_CONSOLE_GRAPH_NODE_LIMIT];
        int anchor_by_role[6][MEM_CONSOLE_GRAPH_NODE_LIMIT];
        int anchor_counts[6] = {0, 0, 0, 0, 0, 0};
        int regular_by_lane[4][MEM_CONSOLE_GRAPH_NODE_LIMIT];
        int regular_counts[4] = {0, 0, 0, 0};
        KitRenderRect scope_lane;
        KitRenderRect anchor_lanes[4];
        KitRenderRect body_lanes[4];
        float pod_pad = 8.0f;
        float section_gap = 5.0f;
        float scope_h;
        float anchor_h;
        float body_y;
        float body_h;
        float lane_gap = 4.0f;
        float lane_w;
        int lane_index;

        if (pod_rect.x + pod_rect.width > bounds.x + bounds.width) {
            pod_rect.x = bounds.x + bounds.width - pod_rect.width - 2.0f;
        }
        if (pod_rect.y + pod_rect.height > bounds.y + bounds.height) {
            pod_rect.y = bounds.y + bounds.height - pod_rect.height - 2.0f;
        }
        memset(node_in_pod, 0, sizeof(node_in_pod));

        for (n = 0; n < pods[p].node_count; ++n) {
            int node_index = pods[p].node_indices[n];
            GraphBucketRole role;
            if (node_index < 0 || (uint32_t)node_index >= layout_count) {
                continue;
            }
            if (node_index < MEM_CONSOLE_GRAPH_NODE_LIMIT) {
                node_in_pod[node_index] = 1u;
            }
            role = graph_bucket_role_for_node(&state->graph_nodes[node_index]);
            if (role != GRAPH_BUCKET_ROLE_NONE) {
                int role_slot = (int)role;
                if (role_slot >= 0 &&
                    role_slot < 6 &&
                    anchor_counts[role_slot] < MEM_CONSOLE_GRAPH_NODE_LIMIT) {
                    int write_index = anchor_counts[role_slot];
                    anchor_by_role[role_slot][write_index] = node_index;
                    anchor_counts[role_slot] += 1;
                }
            } else {
                GraphBucketRole lane_role = graph_pick_lane_role_for_node(state, node_index, node_in_pod);
                int lane = graph_role_to_lane_index(lane_role);
                if (lane < 0 || lane > 3) {
                    lane = 3;
                }
                if (regular_counts[lane] < MEM_CONSOLE_GRAPH_NODE_LIMIT) {
                    int write_index = regular_counts[lane];
                    regular_by_lane[lane][write_index] = node_index;
                    regular_counts[lane] += 1;
                }
            }
        }

        scope_h = pod_rect.height * 0.14f;
        if (scope_h < 14.0f) {
            scope_h = 14.0f;
        }
        anchor_h = pod_rect.height * 0.16f;
        if (anchor_h < 16.0f) {
            anchor_h = 16.0f;
        }
        body_y = pod_rect.y + pod_pad + scope_h + section_gap + anchor_h + section_gap;
        body_h = (pod_rect.y + pod_rect.height - pod_pad) - body_y;
        if (body_h < 22.0f) {
            body_h = 22.0f;
        }

        lane_w = (pod_rect.width - (pod_pad * 2.0f) - (lane_gap * 3.0f)) / 4.0f;
        if (lane_w < 24.0f) {
            lane_w = 24.0f;
        }
        scope_lane = (KitRenderRect){
            pod_rect.x + pod_pad,
            pod_rect.y + pod_pad,
            pod_rect.width - (pod_pad * 2.0f),
            scope_h
        };
        for (lane_index = 0; lane_index < 4; ++lane_index) {
            float lane_x = pod_rect.x + pod_pad + ((float)lane_index * (lane_w + lane_gap));
            anchor_lanes[lane_index] = (KitRenderRect){
                lane_x,
                scope_lane.y + scope_lane.height + section_gap,
                lane_w,
                anchor_h
            };
            body_lanes[lane_index] = (KitRenderRect){
                lane_x,
                body_y,
                lane_w,
                body_h
            };
        }

        graph_place_nodes_in_lane(layouts,
                                  anchor_by_role[(int)GRAPH_BUCKET_ROLE_SCOPE],
                                  anchor_counts[(int)GRAPH_BUCKET_ROLE_SCOPE],
                                  scope_lane);
        graph_place_nodes_in_lane(layouts,
                                  anchor_by_role[(int)GRAPH_BUCKET_ROLE_PLANS],
                                  anchor_counts[(int)GRAPH_BUCKET_ROLE_PLANS],
                                  anchor_lanes[0]);
        graph_place_nodes_in_lane(layouts,
                                  anchor_by_role[(int)GRAPH_BUCKET_ROLE_DECISIONS],
                                  anchor_counts[(int)GRAPH_BUCKET_ROLE_DECISIONS],
                                  anchor_lanes[1]);
        graph_place_nodes_in_lane(layouts,
                                  anchor_by_role[(int)GRAPH_BUCKET_ROLE_ISSUES],
                                  anchor_counts[(int)GRAPH_BUCKET_ROLE_ISSUES],
                                  anchor_lanes[2]);
        graph_place_nodes_in_lane(layouts,
                                  anchor_by_role[(int)GRAPH_BUCKET_ROLE_MISC],
                                  anchor_counts[(int)GRAPH_BUCKET_ROLE_MISC],
                                  anchor_lanes[3]);

        for (lane_index = 0; lane_index < 4; ++lane_index) {
            graph_place_nodes_in_lane(layouts,
                                      regular_by_lane[lane_index],
                                      regular_counts[lane_index],
                                      body_lanes[lane_index]);
        }
    }
}

void graph_build_node_label_text(const MemConsoleGraphNode *node,
                                 char *out_text,
                                 size_t out_cap) {
    GraphBucketRole role;

    if (!out_text || out_cap == 0u) {
        return;
    }
    out_text[0] = '\0';
    if (!node) {
        (void)snprintf(out_text, out_cap, "0");
        return;
    }

    role = graph_bucket_role_for_node(node);
    if (role != GRAPH_BUCKET_ROLE_NONE) {
        (void)snprintf(out_text, out_cap, "%s", graph_bucket_role_label(role));
        return;
    }

    (void)snprintf(out_text, out_cap, "%lld", (long long)node->item_id);
}

int graph_edge_is_hierarchy_kind(const char *kind) {
    if (!kind || !kind[0]) {
        return 0;
    }
    if (strcmp(kind, "next_step") == 0 ||
        strcmp(kind, "child_of") == 0 ||
        strcmp(kind, "summarizes") == 0 ||
        strcmp(kind, "implements") == 0 ||
        strcmp(kind, "depends_on") == 0 ||
        strcmp(kind, "supports") == 0) {
        return 1;
    }
    return strstr(kind, "step") != 0 || strstr(kind, "child") != 0;
}
