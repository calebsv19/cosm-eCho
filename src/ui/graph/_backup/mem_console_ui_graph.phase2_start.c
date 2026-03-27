#include "mem_console_ui_graph.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mem_console_ui_common.h"
#include "mem_console_ui_hud.h"

static const CoreFontTextSizeTier k_graph_node_label_tiers[] = {
    CORE_FONT_TEXT_SIZE_CAPTION
};

typedef struct GraphEdgeLegendEntry {
    const char *kind;
    const char *label;
    KitRenderColor color;
} GraphEdgeLegendEntry;

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

static const GraphEdgeLegendEntry *graph_edge_legend_entry_for_kind(const char *kind) {
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

static KitRenderColor graph_edge_color_for_kind(const char *kind) {
    const GraphEdgeLegendEntry *entry = graph_edge_legend_entry_for_kind(kind);
    if (entry) {
        return entry->color;
    }
    return (KitRenderColor){ 188, 196, 210, 255 };
}

static const char *graph_edge_display_label_for_kind(const char *kind) {
    const GraphEdgeLegendEntry *entry = graph_edge_legend_entry_for_kind(kind);
    if (entry) {
        return entry->label;
    }
    if (!kind || !kind[0]) {
        return "RELATED";
    }
    return kind;
}

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

static const float k_graph_node_zoom_size_cap = 1.35f;

static float graph_clampf(float value, float min_v, float max_v);
static int graph_edge_is_hierarchy_kind(const char *kind);

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

static void graph_format_project_display_name(const char *project_key,
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

static GraphBucketRole graph_bucket_role_for_node(const MemConsoleGraphNode *node) {
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

static const char *graph_bucket_role_label(GraphBucketRole role) {
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

static KitRenderColor graph_bucket_border_color(GraphBucketRole role) {
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

static void graph_node_project_key(const MemConsoleGraphNode *node,
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

static int graph_find_project_pod(GraphProjectPod *pods,
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

static int graph_collect_project_pods(const MemConsoleState *state,
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

static void apply_project_pod_layout(KitRenderRect bounds,
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

static void graph_build_node_label_text(const MemConsoleGraphNode *node,
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

static int graph_edge_is_hierarchy_kind(const char *kind) {
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

static void configure_graph_layout_style(KitGraphStructLayoutStyle *style) {
    if (!style) {
        return;
    }
    kit_graph_struct_layout_style_default(style);
    style->padding = 12.0f;
    style->level_gap = 84.0f;
    style->sibling_gap = 34.0f;
    style->node_width = 44.0f;
    style->node_height = 18.0f;
    style->node_min_width = 30.0f;
    style->node_max_width = 78.0f;
    style->node_padding_x = 3.0f;
    style->label_char_width = 5.2f;
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

static void apply_multi_edge_route_lanes(const KitGraphStructEdge *edges,
                                         uint32_t edge_count,
                                         const KitGraphStructNodeLayout *layouts,
                                         uint32_t layout_count,
                                         KitGraphStructEdgeRoute *routes) {
    const float lane_spacing = 8.0f;
    uint32_t i;

    if (!edges || !layouts || !routes || edge_count == 0u) {
        return;
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

static float graph_clampf(float value, float min_v, float max_v) {
    if (value < min_v) return min_v;
    if (value > max_v) return max_v;
    return value;
}

static void graph_route_nearest_point_with_tangent(const KitGraphStructEdgeRoute *route,
                                                   KitRenderVec2 target,
                                                   KitRenderVec2 *out_point,
                                                   KitRenderVec2 *out_tangent) {
    float best_dist_sq = 0.0f;
    int has_best = 0;
    uint32_t p;

    if (!route || route->point_count < 2u || !out_point || !out_tangent) {
        return;
    }

    for (p = 0u; p + 1u < route->point_count; ++p) {
        KitRenderVec2 a = route->points[p];
        KitRenderVec2 b = route->points[p + 1u];
        float vx = b.x - a.x;
        float vy = b.y - a.y;
        float seg_len_sq = (vx * vx) + (vy * vy);
        float t = 0.0f;
        float cx;
        float cy;
        float dx;
        float dy;
        float dist_sq;

        if (seg_len_sq <= 0.00001f) {
            continue;
        }

        t = ((target.x - a.x) * vx + (target.y - a.y) * vy) / seg_len_sq;
        t = graph_clampf(t, 0.0f, 1.0f);
        cx = a.x + (vx * t);
        cy = a.y + (vy * t);
        dx = target.x - cx;
        dy = target.y - cy;
        dist_sq = (dx * dx) + (dy * dy);

        if (!has_best || dist_sq < best_dist_sq) {
            float inv_len = 1.0f / sqrtf(seg_len_sq);
            best_dist_sq = dist_sq;
            has_best = 1;
            *out_point = (KitRenderVec2){ cx, cy };
            *out_tangent = (KitRenderVec2){ vx * inv_len, vy * inv_len };
        }
    }

    if (!has_best) {
        KitRenderVec2 a = route->points[0];
        KitRenderVec2 b = route->points[1];
        float vx = b.x - a.x;
        float vy = b.y - a.y;
        float len_sq = (vx * vx) + (vy * vy);
        float inv_len = len_sq > 0.00001f ? (1.0f / sqrtf(len_sq)) : 1.0f;
        *out_point = a;
        *out_tangent = (KitRenderVec2){ vx * inv_len, vy * inv_len };
    }
}

static int graph_point_in_rect(KitRenderRect rect, float x, float y) {
    return (x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height);
}

static float graph_orient2d(KitRenderVec2 a, KitRenderVec2 b, KitRenderVec2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static int graph_on_segment(KitRenderVec2 a, KitRenderVec2 b, KitRenderVec2 p) {
    const float eps = 0.0001f;
    if (p.x < (a.x < b.x ? a.x : b.x) - eps || p.x > (a.x > b.x ? a.x : b.x) + eps) return 0;
    if (p.y < (a.y < b.y ? a.y : b.y) - eps || p.y > (a.y > b.y ? a.y : b.y) + eps) return 0;
    return 1;
}

static int graph_segments_intersect(KitRenderVec2 a0,
                                    KitRenderVec2 a1,
                                    KitRenderVec2 b0,
                                    KitRenderVec2 b1) {
    float o1 = graph_orient2d(a0, a1, b0);
    float o2 = graph_orient2d(a0, a1, b1);
    float o3 = graph_orient2d(b0, b1, a0);
    float o4 = graph_orient2d(b0, b1, a1);
    const float eps = 0.0001f;

    if (((o1 > eps && o2 < -eps) || (o1 < -eps && o2 > eps)) &&
        ((o3 > eps && o4 < -eps) || (o3 < -eps && o4 > eps))) {
        return 1;
    }
    if (fabsf(o1) <= eps && graph_on_segment(a0, a1, b0)) return 1;
    if (fabsf(o2) <= eps && graph_on_segment(a0, a1, b1)) return 1;
    if (fabsf(o3) <= eps && graph_on_segment(b0, b1, a0)) return 1;
    if (fabsf(o4) <= eps && graph_on_segment(b0, b1, a1)) return 1;
    return 0;
}

static int graph_segment_intersects_rect(KitRenderVec2 a,
                                         KitRenderVec2 b,
                                         KitRenderRect rect) {
    KitRenderVec2 r0 = { rect.x, rect.y };
    KitRenderVec2 r1 = { rect.x + rect.width, rect.y };
    KitRenderVec2 r2 = { rect.x + rect.width, rect.y + rect.height };
    KitRenderVec2 r3 = { rect.x, rect.y + rect.height };

    if (graph_point_in_rect(rect, a.x, a.y) || graph_point_in_rect(rect, b.x, b.y)) {
        return 1;
    }
    if (graph_segments_intersect(a, b, r0, r1)) return 1;
    if (graph_segments_intersect(a, b, r1, r2)) return 1;
    if (graph_segments_intersect(a, b, r2, r3)) return 1;
    if (graph_segments_intersect(a, b, r3, r0)) return 1;
    return 0;
}

static int graph_label_rect_edge_overlap_count(const KitGraphStructEdgeRoute *routes,
                                               uint32_t route_count,
                                               KitRenderRect rect) {
    uint32_t i;
    int overlap_count = 0;
    const float edge_pad = 1.0f;
    KitRenderRect test_rect = {
        rect.x - edge_pad,
        rect.y - edge_pad,
        rect.width + (edge_pad * 2.0f),
        rect.height + (edge_pad * 2.0f)
    };

    for (i = 0u; i < route_count; ++i) {
        const KitGraphStructEdgeRoute *route = &routes[i];
        uint32_t p;
        int hit = 0;

        if (route->point_count < 2u) {
            continue;
        }
        for (p = 0u; p + 1u < route->point_count; ++p) {
            if (graph_segment_intersects_rect(route->points[p], route->points[p + 1u], test_rect)) {
                hit = 1;
                break;
            }
        }
        if (hit) {
            overlap_count += 1;
        }
    }
    return overlap_count;
}

static int graph_rects_overlap(KitRenderRect a,
                               KitRenderRect b,
                               float pad) {
    float ax0 = a.x - pad;
    float ay0 = a.y - pad;
    float ax1 = a.x + a.width + pad;
    float ay1 = a.y + a.height + pad;
    float bx0 = b.x - pad;
    float by0 = b.y - pad;
    float bx1 = b.x + b.width + pad;
    float by1 = b.y + b.height + pad;

    if (ax1 <= bx0 || bx1 <= ax0) return 0;
    if (ay1 <= by0 || by1 <= ay0) return 0;
    return 1;
}

static int graph_label_rect_label_overlap_count(const KitGraphStructEdgeLabelLayout *label_layouts,
                                                uint32_t placed_count,
                                                KitRenderRect rect) {
    uint32_t i;
    int overlap_count = 0;
    const float label_pad = 2.0f;

    if (!label_layouts || placed_count == 0u) {
        return 0;
    }

    for (i = 0u; i < placed_count; ++i) {
        const KitRenderRect other = label_layouts[i].rect;
        if (other.width <= 0.0f || other.height <= 0.0f) {
            continue;
        }
        if (graph_rects_overlap(other, rect, label_pad)) {
            overlap_count += 1;
        }
    }

    return overlap_count;
}

static void refine_edge_label_layouts_for_callouts(KitRenderRect bounds,
                                                    const KitGraphStructEdgeRoute *routes,
                                                    uint32_t route_count,
                                                    KitGraphStructEdgeLabelLayout *label_layouts) {
    uint32_t i;

    if (!routes || !label_layouts) {
        return;
    }

    for (i = 0u; i < route_count; ++i) {
        const KitGraphStructEdgeRoute *route = &routes[i];
        KitGraphStructEdgeLabelLayout *layout = &label_layouts[i];
        KitRenderRect rect = layout->rect;
        KitRenderVec2 target = layout->anchor;
        KitRenderVec2 anchor = {0.0f, 0.0f};
        KitRenderVec2 tangent = {1.0f, 0.0f};
        KitRenderVec2 normal;
        float normal_len;
        float width;
        float height;
        float center_x;
        float center_y;
        float to_center_x;
        float to_center_y;
        float side_dot;
        const float callout_gap = 10.0f;
        const float pad = 2.0f;
        float min_x;
        float max_x;
        float min_y;
        float max_y;
        float best_score = 1e9f;
        int best_label_overlap_count = 2147483647;
        int best_overlap_count = 2147483647;
        uint32_t side_index;
        uint32_t dist_index;
        uint32_t lateral_index;
        const float distance_opts[] = { 10.0f, 16.0f, 24.0f, 34.0f, 46.0f };
        const float lateral_opts[] = { 0.0f, 10.0f, -10.0f, 18.0f, -18.0f, 28.0f, -28.0f };
        KitRenderRect best_rect = rect;
        int found_zero_overlap = 0;

        if (route->point_count < 2u || rect.width <= 0.0f || rect.height <= 0.0f) {
            continue;
        }

        if (target.x == 0.0f && target.y == 0.0f) {
            target.x = rect.x + (rect.width * 0.5f);
            target.y = rect.y + (rect.height * 0.5f);
        }

        graph_route_nearest_point_with_tangent(route, target, &anchor, &tangent);
        normal = (KitRenderVec2){ -tangent.y, tangent.x };
        normal_len = sqrtf((normal.x * normal.x) + (normal.y * normal.y));
        if (normal_len <= 0.00001f) {
            normal = (KitRenderVec2){0.0f, -1.0f};
        } else {
            normal.x /= normal_len;
            normal.y /= normal_len;
        }

        center_x = rect.x + (rect.width * 0.5f);
        center_y = rect.y + (rect.height * 0.5f);
        to_center_x = center_x - anchor.x;
        to_center_y = center_y - anchor.y;
        side_dot = (to_center_x * normal.x) + (to_center_y * normal.y);
        if (side_dot < 0.0f) {
            normal.x = -normal.x;
            normal.y = -normal.y;
        } else if (fabsf(side_dot) < 0.01f && (i & 1u)) {
            normal.x = -normal.x;
            normal.y = -normal.y;
        }

        width = rect.width;
        height = rect.height;

        min_x = bounds.x + pad;
        max_x = bounds.x + bounds.width - width - pad;
        min_y = bounds.y + pad;
        max_y = bounds.y + bounds.height - height - pad;
        if (max_x < min_x) max_x = min_x;
        if (max_y < min_y) max_y = min_y;

        for (side_index = 0u; side_index < 2u; ++side_index) {
            KitRenderVec2 trial_normal = normal;
            if (side_index == 1u) {
                trial_normal.x = -trial_normal.x;
                trial_normal.y = -trial_normal.y;
            }

            for (dist_index = 0u; dist_index < (uint32_t)(sizeof(distance_opts) / sizeof(distance_opts[0])); ++dist_index) {
                for (lateral_index = 0u; lateral_index < (uint32_t)(sizeof(lateral_opts) / sizeof(lateral_opts[0])); ++lateral_index) {
                    KitRenderRect trial_rect;
                    float dist = distance_opts[dist_index] + callout_gap;
                    float lateral = lateral_opts[lateral_index];
                    float score;
                    int overlaps;
                    int label_overlaps;

                    center_x = anchor.x + (trial_normal.x * dist) + (tangent.x * lateral);
                    center_y = anchor.y + (trial_normal.y * dist) + (tangent.y * lateral);

                    trial_rect.width = width;
                    trial_rect.height = height;
                    trial_rect.x = graph_clampf(center_x - (width * 0.5f), min_x, max_x);
                    trial_rect.y = graph_clampf(center_y - (height * 0.5f), min_y, max_y);

                    overlaps = graph_label_rect_edge_overlap_count(routes, route_count, trial_rect);
                    label_overlaps = graph_label_rect_label_overlap_count(label_layouts, i, trial_rect);
                    score = (float)overlaps * 10000.0f +
                            (float)label_overlaps * 16000.0f +
                            dist * 1.0f +
                            fabsf(lateral) * 0.2f +
                            (side_index == 1u ? 0.5f : 0.0f);

                    if (overlaps == 0 && label_overlaps == 0) {
                        if (!found_zero_overlap || score < best_score) {
                            found_zero_overlap = 1;
                            best_score = score;
                            best_rect = trial_rect;
                        }
                    } else if (!found_zero_overlap) {
                        if (label_overlaps < best_label_overlap_count ||
                            (label_overlaps == best_label_overlap_count &&
                             (overlaps < best_overlap_count ||
                              (overlaps == best_overlap_count && score < best_score)))) {
                            best_label_overlap_count = label_overlaps;
                            best_overlap_count = overlaps;
                            best_score = score;
                            best_rect = trial_rect;
                        }
                    }
                }
            }
        }

        layout->rect = best_rect;
        layout->anchor = anchor;
    }
}

static KitRenderVec2 compute_label_attach_point(KitRenderRect rect, KitRenderVec2 anchor) {
    KitRenderVec2 out = {
        graph_clampf(anchor.x, rect.x, rect.x + rect.width),
        graph_clampf(anchor.y, rect.y, rect.y + rect.height)
    };

    if (anchor.x >= rect.x && anchor.x <= rect.x + rect.width &&
        anchor.y >= rect.y && anchor.y <= rect.y + rect.height) {
        float left_d = anchor.x - rect.x;
        float right_d = (rect.x + rect.width) - anchor.x;
        float top_d = anchor.y - rect.y;
        float bottom_d = (rect.y + rect.height) - anchor.y;
        float best = left_d;
        out = (KitRenderVec2){ rect.x, anchor.y };
        if (right_d < best) {
            best = right_d;
            out = (KitRenderVec2){ rect.x + rect.width, anchor.y };
        }
        if (top_d < best) {
            best = top_d;
            out = (KitRenderVec2){ anchor.x, rect.y };
        }
        if (bottom_d < best) {
            out = (KitRenderVec2){ anchor.x, rect.y + rect.height };
        }
    }

    return out;
}

static CoreResult draw_rect_outline(KitRenderFrame *frame,
                                    KitRenderRect rect,
                                    float thickness,
                                    KitRenderColor color) {
    KitRenderRectCommand rect_cmd;
    CoreResult result;
    float inset;
    float horizontal_w;
    float vertical_h;

    if (!frame || thickness <= 0.0f || rect.width <= 0.0f || rect.height <= 0.0f) {
        return core_result_ok();
    }

    inset = thickness;
    if (inset > rect.width * 0.5f) {
        inset = rect.width * 0.5f;
    }
    if (inset > rect.height * 0.5f) {
        inset = rect.height * 0.5f;
    }
    if (inset <= 0.0f) {
        return core_result_ok();
    }

    rect_cmd.corner_radius = 0.0f;
    rect_cmd.color = color;
    rect_cmd.transform = kit_render_identity_transform();

    rect_cmd.rect = (KitRenderRect){ rect.x, rect.y, rect.width, inset };
    result = kit_render_push_rect(frame, &rect_cmd);
    if (result.code != CORE_OK) return result;

    rect_cmd.rect = (KitRenderRect){ rect.x, rect.y + rect.height - inset, rect.width, inset };
    result = kit_render_push_rect(frame, &rect_cmd);
    if (result.code != CORE_OK) return result;

    horizontal_w = rect.width - (inset * 2.0f);
    vertical_h = rect.height - (inset * 2.0f);
    if (horizontal_w <= 0.0f || vertical_h <= 0.0f) {
        return core_result_ok();
    }

    rect_cmd.rect = (KitRenderRect){ rect.x, rect.y + inset, inset, vertical_h };
    result = kit_render_push_rect(frame, &rect_cmd);
    if (result.code != CORE_OK) return result;

    rect_cmd.rect = (KitRenderRect){ rect.x + rect.width - inset, rect.y + inset, inset, vertical_h };
    return kit_render_push_rect(frame, &rect_cmd);
}

static uint32_t graph_hash_project_key(const char *text) {
    uint32_t hash = 2166136261u;
    uint32_t i = 0u;

    if (!text) {
        return hash;
    }
    while (text[i] != '\0') {
        hash ^= (uint32_t)(uint8_t)text[i];
        hash *= 16777619u;
        i += 1u;
    }
    return hash;
}

static KitRenderColor graph_pod_border_for_key(const char *project_key) {
    static const KitRenderColor palette[] = {
        { 110, 172, 255, 255 },
        { 124, 216, 162, 255 },
        { 244, 188, 108, 255 },
        { 224, 146, 244, 255 },
        { 238, 128, 128, 255 },
        { 140, 198, 214, 255 },
        { 212, 184, 120, 255 }
    };
    uint32_t hash = graph_hash_project_key(project_key);
    uint32_t index = hash % (uint32_t)(sizeof(palette) / sizeof(palette[0]));
    return palette[index];
}

static CoreResult draw_project_pod_overlays(const KitRenderContext *render_ctx,
                                            KitRenderFrame *frame,
                                            MemConsoleState *state,
                                            KitRenderRect bounds) {
    GraphProjectPod pods[MEM_CONSOLE_GRAPH_PROJECT_POD_LIMIT];
    int pod_count = 0;
    int p;

    if (!render_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid pod overlay draw request" };
    }
    if (!state->graph_scope_full_mode_enabled || state->graph_layout_node_count == 0u) {
        return core_result_ok();
    }

    memset(pods, 0, sizeof(pods));
    pod_count = graph_collect_project_pods(state,
                                           state->graph_layout_node_layouts,
                                           state->graph_layout_node_count,
                                           pods,
                                           MEM_CONSOLE_GRAPH_PROJECT_POD_LIMIT);
    if (pod_count <= 0) {
        return core_result_ok();
    }

    for (p = 0; p < pod_count; ++p) {
        KitRenderRect pod_rect = pods[p].bounds;
        KitRenderColor border = graph_pod_border_for_key(pods[p].key);
        KitRenderColor fill = border;
        KitRenderColor guide = border;
        KitRenderRectCommand rect_cmd;
        CoreResult result;

        if (pods[p].node_count <= 0) {
            continue;
        }

        pod_rect.x -= 10.0f;
        pod_rect.y -= 14.0f;
        pod_rect.width += 20.0f;
        pod_rect.height += 20.0f;
        if (pod_rect.width <= 0.0f || pod_rect.height <= 0.0f) {
            continue;
        }
        if (pod_rect.x < bounds.x) {
            float delta = bounds.x - pod_rect.x;
            pod_rect.x = bounds.x;
            pod_rect.width -= delta;
        }
        if (pod_rect.y < bounds.y) {
            float delta = bounds.y - pod_rect.y;
            pod_rect.y = bounds.y;
            pod_rect.height -= delta;
        }
        if (pod_rect.x + pod_rect.width > bounds.x + bounds.width) {
            pod_rect.width = (bounds.x + bounds.width) - pod_rect.x;
        }
        if (pod_rect.y + pod_rect.height > bounds.y + bounds.height) {
            pod_rect.height = (bounds.y + bounds.height) - pod_rect.y;
        }
        if (pod_rect.width <= 4.0f || pod_rect.height <= 4.0f) {
            continue;
        }

        fill.a = state->graph_viewport.zoom <= 0.70f ? 34u : 22u;
        border.a = 132u;
        rect_cmd.rect = pod_rect;
        rect_cmd.corner_radius = 10.0f;
        rect_cmd.color = fill;
        rect_cmd.transform = kit_render_identity_transform();
        result = kit_render_push_rect(frame, &rect_cmd);
        if (result.code != CORE_OK) {
            return result;
        }
        result = draw_rect_outline(frame, pod_rect, 1.2f, border);
        if (result.code != CORE_OK) {
            return result;
        }

        if (pod_rect.width > 80.0f && pod_rect.height > 40.0f) {
            KitRenderLineCommand line_cmd;
            float lane_gap = 4.0f;
            float pad_x = 8.0f;
            float lane_w = (pod_rect.width - (pad_x * 2.0f) - (lane_gap * 3.0f)) / 4.0f;
            int lane;
            guide.a = 64u;
            for (lane = 1; lane <= 3; ++lane) {
                float x = pod_rect.x + pad_x + ((lane_w + lane_gap) * (float)lane) - (lane_gap * 0.5f);
                line_cmd.p0 = (KitRenderVec2){ x, pod_rect.y + 18.0f };
                line_cmd.p1 = (KitRenderVec2){ x, pod_rect.y + pod_rect.height - 5.0f };
                line_cmd.thickness = 1.0f;
                line_cmd.color = guide;
                line_cmd.transform = kit_render_identity_transform();
                result = kit_render_push_line(frame, &line_cmd);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }

        if (state->graph_viewport.zoom <= 0.78f) {
            KitRenderTextCommand text_cmd = {0};
            graph_format_project_display_name(pods[p].key,
                                              state->graph_draw_pod_labels[p],
                                              sizeof(state->graph_draw_pod_labels[p]));
            text_cmd.origin.x = pod_rect.x + 7.0f;
            text_cmd.origin.y = pod_rect.y + 9.0f;
            text_cmd.text = state->graph_draw_pod_labels[p];
            text_cmd.font_role = CORE_FONT_ROLE_UI_MEDIUM;
            text_cmd.text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
            text_cmd.color_token = CORE_THEME_COLOR_TEXT_MUTED;
            text_cmd.transform = kit_render_identity_transform();
            result = kit_render_push_text(frame, &text_cmd);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    (void)render_ctx;
    return core_result_ok();
}

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

static void graph_camera_apply_to_layouts(KitGraphStructNodeLayout *layouts,
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
    node_zoom = safe_zoom;
    if (node_zoom > k_graph_node_zoom_size_cap) {
        node_zoom = k_graph_node_zoom_size_cap;
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

        layouts[i].rect.x = graph_bounds.x + viewport->pan_y + (world_x * safe_zoom) + center_compensate_x;
        layouts[i].rect.y = graph_bounds.y + viewport->pan_x + (world_y * safe_zoom) + center_compensate_y;
        layouts[i].rect.width = capped_scaled_w;
        layouts[i].rect.height = capped_scaled_h;
    }
}

static void graph_camera_pan_by_screen_delta(KitGraphStructViewport *viewport,
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
                                                0.05f,
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
    uint32_t viewport_bits[3];
    int node_count = 0;
    int edge_count = 0;
    int i;

    if (!state) {
        return hash;
    }

    bounds_bits[0] = float_bits(bounds.x);
    bounds_bits[1] = float_bits(bounds.y);
    bounds_bits[2] = float_bits(bounds.width);
    bounds_bits[3] = float_bits(bounds.height);
    viewport_bits[0] = float_bits(state->graph_viewport.pan_x);
    viewport_bits[1] = float_bits(state->graph_viewport.pan_y);
    viewport_bits[2] = float_bits(state->graph_viewport.zoom);

    hash = graph_hash_bytes(hash, &state->graph_node_count, sizeof(state->graph_node_count));
    hash = graph_hash_bytes(hash, &state->graph_edge_count, sizeof(state->graph_edge_count));
    hash = graph_hash_bytes(hash, bounds_bits, sizeof(bounds_bits));
    hash = graph_hash_bytes(hash, viewport_bits, sizeof(viewport_bits));
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
    kit_graph_struct_edge_label_options_default(&label_options);
    label_options.current_zoom = state->graph_viewport.zoom;
    label_options.min_zoom_for_labels = 0.72f;
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

static KitRenderColor mix_color(KitRenderColor a, KitRenderColor b, float t) {
    float clamped_t = t;
    float inv_t;
    KitRenderColor out;

    if (clamped_t < 0.0f) clamped_t = 0.0f;
    if (clamped_t > 1.0f) clamped_t = 1.0f;
    inv_t = 1.0f - clamped_t;

    out.r = (uint8_t)((a.r * inv_t) + (b.r * clamped_t));
    out.g = (uint8_t)((a.g * inv_t) + (b.g * clamped_t));
    out.b = (uint8_t)((a.b * inv_t) + (b.b * clamped_t));
    out.a = 255u;
    return out;
}

static KitRenderColor derive_subtle_outline_color(KitRenderColor fill_color) {
    KitRenderColor out = fill_color;
    int luminance = ((int)fill_color.r * 30 +
                     (int)fill_color.g * 59 +
                     (int)fill_color.b * 11) / 100;
    int delta = 18;

    if (luminance < 96) {
        out.r = (uint8_t)(((int)fill_color.r + delta) > 255 ? 255 : ((int)fill_color.r + delta));
        out.g = (uint8_t)(((int)fill_color.g + delta) > 255 ? 255 : ((int)fill_color.g + delta));
        out.b = (uint8_t)(((int)fill_color.b + delta) > 255 ? 255 : ((int)fill_color.b + delta));
    } else {
        out.r = (uint8_t)(((int)fill_color.r - delta) < 0 ? 0 : ((int)fill_color.r - delta));
        out.g = (uint8_t)(((int)fill_color.g - delta) < 0 ? 0 : ((int)fill_color.g - delta));
        out.b = (uint8_t)(((int)fill_color.b - delta) < 0 ? 0 : ((int)fill_color.b - delta));
    }

    out.a = 255u;
    return out;
}

static CoreResult draw_graph_endpoint_markers(const KitRenderContext *render_ctx,
                                              KitRenderFrame *frame,
                                              const MemConsoleState *state) {
    CoreResult result;
    KitRenderColor out_color;
    KitRenderColor in_color;
    uint32_t i;

    if (!render_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph endpoint marker request" };
    }

    result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_STATUS_OK, &out_color);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_STATUS_ERROR, &in_color);
    if (result.code != CORE_OK) {
        return result;
    }

    for (i = 0u; i < state->graph_layout_edge_count; ++i) {
        const KitGraphStructEdgeRoute *route = &state->graph_layout_edge_routes[i];
        uint32_t last_index;
        KitRenderVec2 start;
        KitRenderVec2 end;
        const float marker_r = 2.8f;
        KitRenderRectCommand marker_rect;

        if (route->point_count < 2u) {
            continue;
        }
        last_index = route->point_count - 1u;
        start = route->points[0];
        end = route->points[last_index];

        marker_rect.corner_radius = marker_r;
        marker_rect.transform = kit_render_identity_transform();

        marker_rect.rect = (KitRenderRect){
            start.x - marker_r,
            start.y - marker_r,
            marker_r * 2.0f,
            marker_r * 2.0f
        };
        marker_rect.color = out_color;
        result = kit_render_push_rect(frame, &marker_rect);
        if (result.code != CORE_OK) {
            return result;
        }

        marker_rect.rect = (KitRenderRect){
            end.x - marker_r,
            end.y - marker_r,
            marker_r * 2.0f,
            marker_r * 2.0f
        };
        marker_rect.color = in_color;
        result = kit_render_push_rect(frame, &marker_rect);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    return core_result_ok();
}

static CoreResult draw_graph_edge_legend(KitUiContext *ui_ctx,
                                         const KitRenderContext *render_ctx,
                                         const KitUiInputState *input,
                                         KitRenderFrame *frame,
                                         KitRenderRect bounds,
                                         MemConsoleState *state,
                                         int *out_click_consumed,
                                         int *out_filter_changed) {
    const GraphEdgeLegendEntry *rows[9];
    int row_count = 1;
    int i;
    float row_h = 13.0f;
    float title_h = 12.0f;
    float pad = 4.0f;
    float legend_w = 144.0f;
    float legend_h;
    KitRenderRect legend_outer;
    KitRenderRect legend_inner;
    CoreResult result;
    int hovered_row = -1;
    int clicked_row = -1;

    if (!ui_ctx || !render_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph legend draw request" };
    }
    if (out_click_consumed) {
        *out_click_consumed = 0;
    }
    if (out_filter_changed) {
        *out_filter_changed = 0;
    }

    rows[0] = 0; /* Row 0 is ALL. */

    for (i = 0; i < (int)(sizeof(k_graph_edge_legend_entries) / sizeof(k_graph_edge_legend_entries[0])); ++i) {
        if (row_count < (int)(sizeof(rows) / sizeof(rows[0]))) {
            rows[row_count++] = &k_graph_edge_legend_entries[i];
        }
    }

    if (row_count <= 0) {
        return core_result_ok();
    }

    legend_h = pad + title_h + 2.0f + ((float)row_count * row_h) + pad;
    legend_outer = (KitRenderRect){
        bounds.x + bounds.width - legend_w - 8.0f,
        bounds.y + 8.0f,
        legend_w,
        legend_h
    };
    legend_inner = (KitRenderRect){
        legend_outer.x + 2.0f,
        legend_outer.y + 2.0f,
        legend_outer.width - 4.0f,
        legend_outer.height - 4.0f
    };

    result = mem_console_ui_push_themed_rect(render_ctx, frame, legend_outer, 6.0f, CORE_THEME_COLOR_SURFACE_1);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_push_themed_rect(render_ctx, frame, legend_inner, 5.0f, CORE_THEME_COLOR_SURFACE_0);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  (KitRenderRect){
                                                      legend_inner.x + 5.0f,
                                                      legend_inner.y + 2.0f,
                                                      legend_inner.width - 10.0f,
                                                      title_h
                                                  },
                                                  "EDGE KEY",
                                                  CORE_THEME_COLOR_TEXT_MUTED,
                                                  CORE_FONT_ROLE_UI_MEDIUM,
                                                  CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }

    if (input && kit_ui_point_in_rect(legend_inner, input->mouse_x, input->mouse_y)) {
        for (i = 0; i < row_count; ++i) {
            float y = legend_inner.y + pad + title_h + 2.0f + ((float)i * row_h);
            KitRenderRect row_rect = {
                legend_inner.x + 4.0f,
                y - 1.0f,
                legend_inner.width - 8.0f,
                row_h
            };
            if (kit_ui_point_in_rect(row_rect, input->mouse_x, input->mouse_y)) {
                hovered_row = i;
                break;
            }
        }
    }

    if (hovered_row >= 0 && input && input->mouse_released) {
        clicked_row = hovered_row;
        if (out_click_consumed) {
            *out_click_consumed = 1;
        }
    }

    for (i = 0; i < row_count; ++i) {
        float y = legend_inner.y + pad + title_h + 2.0f + ((float)i * row_h);
        KitRenderRect row_rect = {
            legend_inner.x + 4.0f,
            y - 1.0f,
            legend_inner.width - 8.0f,
            row_h
        };
        const char *label = "ALL";
        KitRenderColor row_color = (KitRenderColor){ 188, 196, 210, 255 };
        int row_selected = 0;
        KitRenderLineCommand swatch_line;
        KitRenderLineCommand underline;

        if (rows[i]) {
            label = rows[i]->label;
            row_color = rows[i]->color;
        }

        if (i == 0) {
            row_selected = state->graph_kind_filter_all_override ? 1 : 0;
        } else if (rows[i]) {
            row_selected = mem_console_graph_kind_is_enabled(state, rows[i]->kind);
        }

        if (row_selected) {
            result = draw_rect_outline(frame, row_rect, 1.0f, row_color);
            if (result.code != CORE_OK) {
                return result;
            }
        }

        swatch_line.p0 = (KitRenderVec2){ legend_inner.x + 6.0f, y + (row_h * 0.5f) + 0.5f };
        swatch_line.p1 = (KitRenderVec2){ legend_inner.x + 19.0f, y + (row_h * 0.5f) + 0.5f };
        swatch_line.thickness = 2.4f;
        swatch_line.color = row_color;
        swatch_line.transform = kit_render_identity_transform();
        result = kit_render_push_line(frame, &swatch_line);
        if (result.code != CORE_OK) {
            return result;
        }

        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      (KitRenderRect){
                                                          legend_inner.x + 24.0f,
                                                          y,
                                                          legend_inner.width - 28.0f,
                                                          row_h
                                                      },
                                                      label,
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }

        if (i == hovered_row) {
            underline.p0 = (KitRenderVec2){ legend_inner.x + 24.0f, y + row_h - 1.0f };
            underline.p1 = (KitRenderVec2){ legend_inner.x + legend_inner.width - 6.0f, y + row_h - 1.0f };
            underline.thickness = 1.0f;
            underline.color = row_color;
            underline.transform = kit_render_identity_transform();
            result = kit_render_push_line(frame, &underline);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    if (clicked_row >= 0) {
        uint32_t before_mask = state->graph_kind_filter_mask;
        int before_all_override = state->graph_kind_filter_all_override;
        if (clicked_row == 0) {
            (void)mem_console_graph_kind_toggle_all_override(state);
        } else if (rows[clicked_row]) {
            (void)mem_console_graph_kind_toggle_enabled(state, rows[clicked_row]->kind);
        }
        if (before_mask != state->graph_kind_filter_mask ||
            before_all_override != state->graph_kind_filter_all_override) {
            if (out_filter_changed) {
                *out_filter_changed = 1;
            }
        }
    }

    return core_result_ok();
}

static const char *graph_layout_mode_text(int layout_mode) {
    return layout_mode == MEM_CONSOLE_GRAPH_LAYOUT_TREE ? "tree" : "dag";
}

static const char *graph_scope_mode_text(const MemConsoleState *state) {
    if (!state) {
        return "focus";
    }
    return state->graph_scope_full_mode_enabled ? "full" : "focus";
}

static CoreResult draw_graph_view_diagnostics(KitUiContext *ui_ctx,
                                              const KitRenderContext *render_ctx,
                                              KitRenderFrame *frame,
                                              MemConsoleState *state,
                                              KitRenderRect bounds,
                                              uint32_t node_count,
                                              uint32_t edge_count) {
    CoreResult result;
    KitRenderRect bar_rect = {
        bounds.x + 8.0f,
        bounds.y + bounds.height - 18.0f,
        300.0f,
        12.0f
    };

    if (!ui_ctx || !render_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph diagnostics draw request" };
    }
    if (bounds.width <= 28.0f || bounds.height <= 18.0f) {
        return core_result_ok();
    }

    if (bar_rect.width > bounds.width - 16.0f) {
        bar_rect.width = bounds.width - 16.0f;
    }
    if (bar_rect.y < bounds.y + 2.0f) {
        bar_rect.y = bounds.y + 2.0f;
    }

    (void)snprintf(state->graph_status_line,
                   sizeof(state->graph_status_line),
                   "scope:%s  layout:%s  zoom:%.2fx  n:%u e:%u",
                   graph_scope_mode_text(state),
                   graph_layout_mode_text(state->graph_layout_mode),
                   state->graph_viewport.zoom,
                   node_count,
                   edge_count);

    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             (KitRenderRect){
                                                 bar_rect.x - 2.0f,
                                                 bar_rect.y - 1.0f,
                                                 bar_rect.width + 4.0f,
                                                 bar_rect.height + 2.0f
                                             },
                                             4.0f,
                                             CORE_THEME_COLOR_SURFACE_0);
    if (result.code != CORE_OK) {
        return result;
    }

    return mem_console_ui_draw_info_line_custom(ui_ctx,
                                                frame,
                                                bar_rect,
                                                state->graph_status_line,
                                                CORE_THEME_COLOR_TEXT_MUTED,
                                                CORE_FONT_ROLE_UI_REGULAR,
                                                CORE_FONT_TEXT_SIZE_CAPTION);
}

static float graph_edge_route_hit_radius_for_zoom(const MemConsoleState *state) {
    float zoom = 1.0f;
    float radius = 9.5f;

    if (state) {
        zoom = state->graph_viewport.zoom;
    }
    if (zoom < 1.0f) {
        radius += (1.0f - zoom) * 2.4f;
    } else if (zoom > 1.4f) {
        radius -= (zoom - 1.4f) * 1.6f;
    }
    if (radius < 6.0f) {
        radius = 6.0f;
    }
    if (radius > 13.0f) {
        radius = 13.0f;
    }
    return radius;
}

static int graph_find_edge_index_at_point(const MemConsoleState *state,
                                          float mouse_x,
                                          float mouse_y,
                                          float route_hit_radius_px,
                                          uint32_t *out_edge_index) {
    CoreResult result;
    KitGraphStructEdgeHit edge_hit;
    KitGraphStructEdgeLabelHit label_hit;

    if (!state || !out_edge_index || state->graph_layout_edge_count == 0u) {
        return 0;
    }

    result = kit_graph_struct_hit_test_edge_labels(state->graph_layout_edge_label_layouts,
                                                   state->graph_layout_edge_count,
                                                   mouse_x,
                                                   mouse_y,
                                                   &label_hit);
    if (result.code == CORE_OK &&
        label_hit.active &&
        label_hit.edge_index < state->graph_layout_edge_count) {
        *out_edge_index = label_hit.edge_index;
        return 1;
    }

    result = kit_graph_struct_hit_test_edge_routes(state->graph_layout_edge_routes,
                                                   state->graph_layout_edge_count,
                                                   mouse_x,
                                                   mouse_y,
                                                   route_hit_radius_px,
                                                   &edge_hit);
    if (result.code == CORE_OK &&
        edge_hit.active &&
        edge_hit.edge_index < state->graph_layout_edge_count) {
        *out_edge_index = edge_hit.edge_index;
        return 1;
    }

    return 0;
}

static int graph_resolve_layout_edge_node_indices(const MemConsoleState *state,
                                                  uint32_t edge_index,
                                                  int *out_from_index,
                                                  int *out_to_index) {
    int from_index = -1;
    int to_index = -1;

    if (!state || !out_from_index || !out_to_index ||
        edge_index >= state->graph_layout_edge_count) {
        return 0;
    }

    from_index = (int)state->graph_layout_edges[edge_index].from_id - 1;
    to_index = (int)state->graph_layout_edges[edge_index].to_id - 1;
    if (from_index < 0 || to_index < 0 ||
        from_index >= state->graph_node_count ||
        to_index >= state->graph_node_count) {
        return 0;
    }

    *out_from_index = from_index;
    *out_to_index = to_index;
    return 1;
}

static uint64_t graph_hud_hash_u64(uint64_t seed, uint64_t value) {
    uint64_t hash = seed;
    uint32_t i;
    for (i = 0u; i < 8u; ++i) {
        hash ^= (value & 0xffu);
        hash *= 1099511628211ull;
        value >>= 8u;
    }
    return hash;
}

static int graph_build_node_hud_spec(MemConsoleState *state,
                                     int hovered_node_index,
                                     MemConsoleUiHudCardSpec *out_spec) {
    const MemConsoleGraphNode *hovered_node;
    const char *raw_body;

    if (!state || !out_spec || hovered_node_index < 0 || hovered_node_index >= state->graph_node_count) {
        return 0;
    }
    hovered_node = &state->graph_nodes[hovered_node_index];
    raw_body = hovered_node->body_preview[0] ? hovered_node->body_preview : "(no body)";

    memset(out_spec, 0, sizeof(*out_spec));
    out_spec->width_ratio = 0.48f;
    out_spec->min_width = 220.0f;
    out_spec->max_width = 560.0f;
    out_spec->edge_margin = 10.0f;
    out_spec->row_count = 4;
    out_spec->rows[0] = (MemConsoleUiHudRowSpec){
        .text = state->graph_hud_id_line,
        .token = CORE_THEME_COLOR_TEXT_MUTED,
        .font_role = CORE_FONT_ROLE_UI_MEDIUM,
        .text_tier = CORE_FONT_TEXT_SIZE_CAPTION,
        .max_lines = 1
    };
    out_spec->rows[1] = (MemConsoleUiHudRowSpec){
        .text = hovered_node->title[0] ? hovered_node->title : "UNTITLED",
        .token = CORE_THEME_COLOR_TEXT_PRIMARY,
        .font_role = CORE_FONT_ROLE_UI_BOLD,
        .text_tier = CORE_FONT_TEXT_SIZE_PARAGRAPH,
        .max_lines = 3
    };
    out_spec->rows[2] = (MemConsoleUiHudRowSpec){
        .text = state->graph_hud_flags,
        .token = CORE_THEME_COLOR_TEXT_MUTED,
        .font_role = CORE_FONT_ROLE_UI_REGULAR,
        .text_tier = CORE_FONT_TEXT_SIZE_CAPTION,
        .max_lines = 2
    };
    out_spec->rows[3] = (MemConsoleUiHudRowSpec){
        .text = raw_body,
        .token = CORE_THEME_COLOR_TEXT_MUTED,
        .font_role = CORE_FONT_ROLE_UI_REGULAR,
        .text_tier = CORE_FONT_TEXT_SIZE_BASIC,
        .max_lines = 8
    };
    (void)snprintf(state->graph_hud_id_line,
                   sizeof(state->graph_hud_id_line),
                   "ID %lld",
                   (long long)hovered_node->item_id);
    (void)snprintf(state->graph_hud_flags,
                   sizeof(state->graph_hud_flags),
                   "PIN %s | CAN %s",
                   hovered_node->pinned ? "ON" : "OFF",
                   hovered_node->canonical ? "ON" : "OFF");
    out_spec->cache_key = graph_hud_hash_u64(1469598103934665603ull, state->graph_layout_signature);
    out_spec->cache_key = graph_hud_hash_u64(out_spec->cache_key, (uint64_t)hovered_node->item_id);
    out_spec->cache_key = graph_hud_hash_u64(out_spec->cache_key, 1ull);
    return 1;
}

static int graph_build_edge_hud_spec(MemConsoleState *state,
                                     int hovered_edge_index,
                                     MemConsoleUiHudCardSpec *out_spec) {
    int from_index = -1;
    int to_index = -1;
    const MemConsoleGraphNode *from_node;
    const MemConsoleGraphNode *to_node;
    const char *from_title;
    const char *to_title;
    const char *edge_kind_raw = "related";
    const char *edge_kind_label = "RELATED";
    int state_edge_index;

    if (!state || !out_spec ||
        hovered_edge_index < 0 ||
        hovered_edge_index >= (int)state->graph_layout_edge_count) {
        return 0;
    }
    if (!graph_resolve_layout_edge_node_indices(state,
                                                (uint32_t)hovered_edge_index,
                                                &from_index,
                                                &to_index)) {
        return 0;
    }

    from_node = &state->graph_nodes[from_index];
    to_node = &state->graph_nodes[to_index];
    from_title = from_node->title[0] ? from_node->title : "UNKNOWN";
    to_title = to_node->title[0] ? to_node->title : "UNKNOWN";
    state_edge_index = state->graph_layout_edge_state_indices[hovered_edge_index];
    if (state_edge_index >= 0 &&
        state_edge_index < state->graph_edge_count &&
        state->graph_edges[state_edge_index].kind[0] != '\0') {
        edge_kind_raw = state->graph_edges[state_edge_index].kind;
    }
    edge_kind_label = graph_edge_display_label_for_kind(edge_kind_raw);

    (void)snprintf(state->graph_hud_id_line,
                   sizeof(state->graph_hud_id_line),
                   "EDGE %s",
                   edge_kind_label);
    (void)snprintf(state->graph_hud_flags,
                   sizeof(state->graph_hud_flags),
                   "%lld --%s--> %lld",
                   (long long)from_node->item_id,
                   edge_kind_label,
                   (long long)to_node->item_id);
    (void)snprintf(state->graph_hud_body,
                   sizeof(state->graph_hud_body),
                   "LINK: %s -> %s",
                   from_title,
                   to_title);

    memset(out_spec, 0, sizeof(*out_spec));
    out_spec->width_ratio = 0.52f;
    out_spec->min_width = 240.0f;
    out_spec->max_width = 620.0f;
    out_spec->edge_margin = 10.0f;
    out_spec->row_count = 5;
    out_spec->rows[0] = (MemConsoleUiHudRowSpec){
        .text = state->graph_hud_id_line,
        .token = CORE_THEME_COLOR_TEXT_MUTED,
        .font_role = CORE_FONT_ROLE_UI_MEDIUM,
        .text_tier = CORE_FONT_TEXT_SIZE_CAPTION,
        .max_lines = 1
    };
    out_spec->rows[1] = (MemConsoleUiHudRowSpec){
        .text = from_title,
        .token = CORE_THEME_COLOR_TEXT_PRIMARY,
        .font_role = CORE_FONT_ROLE_UI_BOLD,
        .text_tier = CORE_FONT_TEXT_SIZE_PARAGRAPH,
        .max_lines = 8
    };
    out_spec->rows[2] = (MemConsoleUiHudRowSpec){
        .text = state->graph_hud_flags,
        .token = CORE_THEME_COLOR_TEXT_MUTED,
        .font_role = CORE_FONT_ROLE_UI_REGULAR,
        .text_tier = CORE_FONT_TEXT_SIZE_CAPTION,
        .max_lines = 4
    };
    out_spec->rows[3] = (MemConsoleUiHudRowSpec){
        .text = to_title,
        .token = CORE_THEME_COLOR_TEXT_PRIMARY,
        .font_role = CORE_FONT_ROLE_UI_BOLD,
        .text_tier = CORE_FONT_TEXT_SIZE_PARAGRAPH,
        .max_lines = 8
    };
    out_spec->rows[4] = (MemConsoleUiHudRowSpec){
        .text = state->graph_hud_body,
        .token = CORE_THEME_COLOR_TEXT_MUTED,
        .font_role = CORE_FONT_ROLE_UI_REGULAR,
        .text_tier = CORE_FONT_TEXT_SIZE_CAPTION,
        .max_lines = 4
    };
    out_spec->cache_key = graph_hud_hash_u64(1469598103934665603ull, state->graph_layout_signature);
    out_spec->cache_key = graph_hud_hash_u64(out_spec->cache_key, (uint64_t)from_node->item_id);
    out_spec->cache_key = graph_hud_hash_u64(out_spec->cache_key, (uint64_t)to_node->item_id);
    out_spec->cache_key = graph_hud_hash_u64(out_spec->cache_key, (uint64_t)(state_edge_index + 2));
    return 1;
}

int mem_console_ui_graph_find_node_index_at_point(const MemConsoleState *state,
                                                  float x,
                                                  float y,
                                                  uint32_t *out_index) {
    uint32_t i;

    if (!state || !out_index) {
        return 0;
    }

    for (i = 0u; i < state->graph_layout_node_count; ++i) {
        if (kit_ui_point_in_rect(state->graph_layout_node_layouts[i].rect, x, y)) {
            *out_index = i;
            return 1;
        }
    }

    return 0;
}

int mem_console_ui_graph_select_neighbor_from_edge_click(const MemConsoleState *state,
                                                         float mouse_x,
                                                         float mouse_y,
                                                         int64_t *out_item_id) {
    uint32_t edge_index = 0u;
    int best_from_index = -1;
    int best_to_index = -1;

    if (!state || !out_item_id) {
        return 0;
    }

    if (!graph_find_edge_index_at_point(state,
                                        mouse_x,
                                        mouse_y,
                                        graph_edge_route_hit_radius_for_zoom(state),
                                        &edge_index)) {
        return 0;
    }

    if (!graph_resolve_layout_edge_node_indices(state,
                                                edge_index,
                                                &best_from_index,
                                                &best_to_index)) {
        return 0;
    }

    {
        int64_t from_item_id = state->graph_nodes[best_from_index].item_id;
        int64_t to_item_id = state->graph_nodes[best_to_index].item_id;
        int64_t next_item_id = 0;

        if (state->selected_item_id == from_item_id && to_item_id != 0) {
            next_item_id = to_item_id;
        } else if (state->selected_item_id == to_item_id && from_item_id != 0) {
            next_item_id = from_item_id;
        } else if (to_item_id != 0 && to_item_id != state->selected_item_id) {
            next_item_id = to_item_id;
        } else {
            next_item_id = from_item_id;
        }

        if (next_item_id != 0 && next_item_id != state->selected_item_id) {
            *out_item_id = next_item_id;
            return 1;
        }
    }
    return 0;
}

CoreResult mem_console_ui_graph_draw_preview(const KitRenderContext *render_ctx,
                                             KitUiContext *ui_ctx,
                                             const KitUiInputState *input,
                                             KitRenderFrame *frame,
                                             KitRenderRect bounds,
                                             MemConsoleState *state,
                                             int *out_legend_click_consumed,
                                             int *out_graph_filter_changed) {
    CoreResult result;
    KitRenderColor edge_white;
    KitRenderColor node_base_color;
    KitRenderColor node_lane_color;
    KitRenderColor node_selected_color;
    KitRenderColor node_center_outline_color;
    CoreResult color_result;
    uint32_t node_count = 0u;
    uint32_t edge_count = 0u;
    uint32_t i;
    int has_graph_data = 0;
    int hovered_node_index = -1;
    int hovered_edge_index = -1;

    if (!render_ctx || !ui_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    if (out_legend_click_consumed) {
        *out_legend_click_consumed = 0;
    }
    if (out_graph_filter_changed) {
        *out_graph_filter_changed = 0;
    }

    result = mem_console_ui_graph_ensure_layout_cache(render_ctx, state, bounds);
    if (result.code != CORE_OK) {
        return result;
    }
    node_count = state->graph_layout_node_count;
    edge_count = state->graph_layout_edge_count;
    has_graph_data = state->graph_layout_has_graph_data;

    if (input && has_graph_data && node_count > 0u &&
        kit_ui_point_in_rect(bounds, input->mouse_x, input->mouse_y)) {
        uint32_t rect_hit_index = 0u;
        if (mem_console_ui_graph_find_node_index_at_point(state,
                                                          input->mouse_x,
                                                          input->mouse_y,
                                                          &rect_hit_index) &&
            rect_hit_index < node_count) {
            hovered_node_index = (int)rect_hit_index;
        } else {
            KitGraphStructHit hit = {0};
            CoreResult hit_result = kit_graph_struct_hit_test(state->graph_layout_node_layouts,
                                                              node_count,
                                                              input->mouse_x,
                                                              input->mouse_y,
                                                              &hit);
            if (hit_result.code == CORE_OK && hit.active && hit.node_index < node_count) {
                hovered_node_index = (int)hit.node_index;
            }
        }
        if (hovered_node_index < 0 && edge_count > 0u) {
            uint32_t edge_index = 0u;
            if (graph_find_edge_index_at_point(state,
                                               input->mouse_x,
                                               input->mouse_y,
                                               graph_edge_route_hit_radius_for_zoom(state),
                                               &edge_index)) {
                hovered_edge_index = (int)edge_index;
            }
        }
    }

    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_TEXT_PRIMARY, &edge_white);
    if (color_result.code != CORE_OK) {
        return color_result;
    }
    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_1, &node_base_color);
    if (color_result.code != CORE_OK) {
        return color_result;
    }
    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_2, &node_lane_color);
    if (color_result.code != CORE_OK) {
        return color_result;
    }
    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_ACCENT_PRIMARY, &node_selected_color);
    if (color_result.code != CORE_OK) {
        return color_result;
    }
    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_STATUS_OK, &node_center_outline_color);
    if (color_result.code != CORE_OK) {
        return color_result;
    }

    result = draw_project_pod_overlays(render_ctx, frame, state, bounds);
    if (result.code != CORE_OK) {
        return result;
    }

    for (i = 0u; i < edge_count; ++i) {
        KitRenderLineCommand line_cmd;
        KitRenderTextCommand label_text_cmd;
        KitRenderRect label_bg_rect = state->graph_layout_edge_label_layouts[i].rect;
        KitRenderVec2 label_anchor = state->graph_layout_edge_label_layouts[i].anchor;
        KitRenderVec2 label_attach;
        const KitGraphStructEdgeRoute *route = &state->graph_layout_edge_routes[i];
        int state_edge_index = state->graph_layout_edge_state_indices[i];
        const char *edge_kind_raw = "related";
        const char *edge_kind_label = "RELATED";
        KitRenderColor edge_draw_color;
        KitRenderColor label_outline_color;
        int is_hovered_edge = (int)i == hovered_edge_index;
        int is_hierarchy_edge = 0;
        if (state_edge_index >= 0 &&
            state_edge_index < state->graph_edge_count &&
            state->graph_edges[state_edge_index].kind[0] != '\0') {
            edge_kind_raw = state->graph_edges[state_edge_index].kind;
        }
        edge_kind_label = graph_edge_display_label_for_kind(edge_kind_raw);
        edge_draw_color = graph_edge_color_for_kind(edge_kind_raw);
        is_hierarchy_edge = graph_edge_is_hierarchy_kind(edge_kind_raw);
        if (!is_hierarchy_edge) {
            edge_draw_color = mix_color(edge_draw_color, edge_white, 0.38f);
        }
        if (route->point_count < 2u) {
            continue;
        }
        {
            float text_w = mem_console_ui_measure_text_width_px(render_ctx,
                                                                CORE_FONT_ROLE_UI_REGULAR,
                                                                CORE_FONT_TEXT_SIZE_CAPTION,
                                                                edge_kind_label);
            float desired_w = text_w + 6.0f;
            float center_x = label_bg_rect.x + (label_bg_rect.width * 0.5f);
            float min_x = bounds.x + 2.0f;
            float max_x = (bounds.x + bounds.width) - desired_w - 2.0f;

            if (desired_w < 14.0f) {
                desired_w = 14.0f;
            }
            if (desired_w > bounds.width - 4.0f) {
                desired_w = bounds.width - 4.0f;
            }
            if (label_bg_rect.width < desired_w) {
                label_bg_rect.width = desired_w;
            }
            label_bg_rect.x = center_x - (label_bg_rect.width * 0.5f);
            if (max_x < min_x) {
                max_x = min_x;
            }
            if (label_bg_rect.x < min_x) {
                label_bg_rect.x = min_x;
            }
            if (label_bg_rect.x > max_x) {
                label_bg_rect.x = max_x;
            }
        }

        {
            uint32_t p;
            for (p = 0u; p + 1u < route->point_count; ++p) {
                line_cmd.p0 = route->points[p];
                line_cmd.p1 = route->points[p + 1u];
                line_cmd.thickness = is_hierarchy_edge
                                         ? (is_hovered_edge ? 4.0f : 2.8f)
                                         : (is_hovered_edge ? 3.0f : 1.6f);
                line_cmd.color = edge_draw_color;
                if (is_hovered_edge) {
                    line_cmd.color = mix_color(edge_draw_color, edge_white, 0.26f);
                }
                line_cmd.transform = kit_render_identity_transform();
                result = kit_render_push_line(frame, &line_cmd);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
        if (label_bg_rect.width <= 0.0f || label_bg_rect.height <= 0.0f) {
            continue;
        }
        label_attach = compute_label_attach_point(label_bg_rect, label_anchor);
        line_cmd.p0 = label_anchor;
        line_cmd.p1 = label_attach;
        line_cmd.thickness = is_hierarchy_edge ? 1.8f : 1.1f;
        line_cmd.color = edge_draw_color;
        if (is_hovered_edge) {
            line_cmd.color = mix_color(edge_draw_color, edge_white, 0.22f);
        }
        line_cmd.color.a = 220u;
        line_cmd.transform = kit_render_identity_transform();
        result = kit_render_push_line(frame, &line_cmd);
        if (result.code != CORE_OK) {
            return result;
        }

        result = mem_console_ui_push_themed_rect(render_ctx,
                                                 frame,
                                                 label_bg_rect,
                                                 2.0f,
                                                 CORE_THEME_COLOR_SURFACE_0);
        if (result.code != CORE_OK) {
            return result;
        }
        label_outline_color = edge_draw_color;
        if (is_hovered_edge) {
            label_outline_color = mix_color(label_outline_color, edge_white, 0.20f);
        }
        label_outline_color.a = 230u;
        result = draw_rect_outline(frame, label_bg_rect, 1.0f, label_outline_color);
        if (result.code != CORE_OK) {
            return result;
        }
        result = kit_render_push_rect(frame,
                                      &(KitRenderRectCommand){
                                          (KitRenderRect){
                                              label_anchor.x - 1.5f,
                                              label_anchor.y - 1.5f,
                                              3.0f,
                                              3.0f
                                          },
                                          1.5f,
                                          label_outline_color,
                                          kit_render_identity_transform()
                                      });
        if (result.code != CORE_OK) {
            return result;
        }

        label_text_cmd.origin.x = label_bg_rect.x + 3.0f;
        label_text_cmd.origin.y = label_bg_rect.y + (label_bg_rect.height * 0.5f);
        (void)snprintf(state->graph_draw_edge_labels[i],
                       sizeof(state->graph_draw_edge_labels[i]),
                       "%s",
                       edge_kind_label);
        label_text_cmd.text = state->graph_draw_edge_labels[i];
        label_text_cmd.font_role = CORE_FONT_ROLE_UI_REGULAR;
        label_text_cmd.text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
        label_text_cmd.color_token = is_hierarchy_edge
                                         ? CORE_THEME_COLOR_TEXT_PRIMARY
                                         : CORE_THEME_COLOR_TEXT_MUTED;
        label_text_cmd.transform = kit_render_identity_transform();
        result = kit_render_push_text(frame, &label_text_cmd);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    for (i = 0u; i < node_count; ++i) {
        KitRenderColor fill_color = node_base_color;
        KitRenderColor node_outline_color;
        KitRenderRectCommand rect_cmd;
        KitRenderTextCommand text_cmd;
        KitUiTextFitResult node_text_fit;
        char node_id_text[24];

        if (has_graph_data) {
            const MemConsoleGraphNode *graph_node = &state->graph_nodes[i];
            GraphBucketRole bucket_role = graph_bucket_role_for_node(graph_node);
            if (graph_node->item_id == state->selected_item_id) {
                fill_color = mix_color(node_base_color, node_selected_color, 0.38f);
            } else if (graph_node->canonical || graph_node->pinned) {
                fill_color = node_lane_color;
            }
            if (bucket_role != GRAPH_BUCKET_ROLE_NONE) {
                KitRenderColor bucket_border = graph_bucket_border_color(bucket_role);
                fill_color = mix_color(fill_color, bucket_border, 0.08f);
            }
            if ((int)i == hovered_node_index && graph_node->item_id != state->selected_item_id) {
                fill_color = mix_color(fill_color, node_selected_color, 0.10f);
                fill_color = mix_color(fill_color, edge_white, 0.12f);
            }
        }

        node_outline_color = derive_subtle_outline_color(fill_color);

        rect_cmd.rect = state->graph_layout_node_layouts[i].rect;
        rect_cmd.corner_radius = 2.4f;
        rect_cmd.color = fill_color;
        rect_cmd.transform = kit_render_identity_transform();
        result = kit_render_push_rect(frame, &rect_cmd);
        if (result.code != CORE_OK) {
            return result;
        }

        result = draw_rect_outline(frame,
                                   state->graph_layout_node_layouts[i].rect,
                                   1.0f,
                                   node_outline_color);
        if (result.code != CORE_OK) {
            return result;
        }

        if (has_graph_data &&
            i < (uint32_t)state->graph_node_count &&
            state->graph_nodes[i].item_id == state->selected_item_id) {
            KitRenderColor selected_outline = mix_color(node_selected_color, edge_white, 0.32f);
            result = draw_rect_outline(frame,
                                       state->graph_layout_node_layouts[i].rect,
                                       1.4f,
                                       selected_outline);
            if (result.code != CORE_OK) {
                return result;
            }
        }
        if (has_graph_data &&
            i < (uint32_t)state->graph_node_count) {
            GraphBucketRole bucket_role = graph_bucket_role_for_node(&state->graph_nodes[i]);
            if (bucket_role != GRAPH_BUCKET_ROLE_NONE) {
                result = draw_rect_outline(frame,
                                           state->graph_layout_node_layouts[i].rect,
                                           2.2f,
                                           graph_bucket_border_color(bucket_role));
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }

        if (has_graph_data &&
            i < (uint32_t)state->graph_node_count &&
            state->graph_nodes[i].item_id == state->graph_center_item_id) {
            result = draw_rect_outline(frame,
                                       state->graph_layout_node_layouts[i].rect,
                                       2.0f,
                                       node_center_outline_color);
            if (result.code != CORE_OK) {
                return result;
            }
        }

        text_cmd.origin.x = state->graph_layout_node_layouts[i].rect.x + 3.0f;
        text_cmd.origin.y = state->graph_layout_node_layouts[i].rect.y +
                            (state->graph_layout_node_layouts[i].rect.height * 0.5f);
        if (has_graph_data && i < (uint32_t)state->graph_node_count) {
            graph_build_node_label_text(&state->graph_nodes[i],
                                        node_id_text,
                                        sizeof(node_id_text));
        } else {
            (void)snprintf(node_id_text, sizeof(node_id_text), "0");
        }
        result = kit_ui_fit_text_to_rect(ui_ctx,
                                         node_id_text,
                                         CORE_FONT_ROLE_UI_REGULAR,
                                         k_graph_node_label_tiers,
                                         (uint32_t)(sizeof(k_graph_node_label_tiers) /
                                                    sizeof(k_graph_node_label_tiers[0])),
                                         state->graph_layout_node_layouts[i].rect.width - 6.0f,
                                         state->graph_layout_node_layouts[i].rect.height - 3.0f,
                                         state->graph_draw_node_labels[i],
                                         sizeof(state->graph_draw_node_labels[i]),
                                         &node_text_fit);
        if (result.code != CORE_OK) {
            return result;
        }
        if (state->graph_draw_node_labels[i][0] == '\0') {
            if (node_id_text[0] != '\0') {
                state->graph_draw_node_labels[i][0] = node_id_text[0];
                state->graph_draw_node_labels[i][1] = '\0';
            } else {
                state->graph_draw_node_labels[i][0] = '0';
                state->graph_draw_node_labels[i][1] = '\0';
            }
            node_text_fit.text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
        }
        text_cmd.text = state->graph_draw_node_labels[i];
        text_cmd.font_role = CORE_FONT_ROLE_UI_REGULAR;
        text_cmd.text_tier = node_text_fit.text_tier;
        text_cmd.color_token = CORE_THEME_COLOR_TEXT_PRIMARY;
        text_cmd.transform = kit_render_identity_transform();
        result = kit_render_push_text(frame, &text_cmd);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    result = draw_graph_endpoint_markers(render_ctx, frame, state);
    if (result.code != CORE_OK) {
        return result;
    }
    result = draw_graph_edge_legend(ui_ctx,
                                    render_ctx,
                                    input,
                                    frame,
                                    bounds,
                                    state,
                                    out_legend_click_consumed,
                                    out_graph_filter_changed);
    if (result.code != CORE_OK) {
        return result;
    }
    result = draw_graph_view_diagnostics(ui_ctx,
                                         render_ctx,
                                         frame,
                                         state,
                                         bounds,
                                         node_count,
                                         edge_count);
    if (result.code != CORE_OK) {
        return result;
    }

    if (has_graph_data &&
        hovered_node_index >= 0 &&
        hovered_node_index < state->graph_node_count) {
        MemConsoleUiHudCardSpec hud_spec;
        if (graph_build_node_hud_spec(state, hovered_node_index, &hud_spec)) {
            result = mem_console_ui_hud_draw_cached(render_ctx,
                                                    ui_ctx,
                                                    frame,
                                                    bounds,
                                                    state,
                                                    &hud_spec);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    } else if (has_graph_data &&
               hovered_edge_index >= 0 &&
               hovered_edge_index < (int)state->graph_layout_edge_count) {
        MemConsoleUiHudCardSpec hud_spec;
        if (graph_build_edge_hud_spec(state, hovered_edge_index, &hud_spec)) {
            result = mem_console_ui_hud_draw_cached(render_ctx,
                                                    ui_ctx,
                                                    frame,
                                                    bounds,
                                                    state,
                                                    &hud_spec);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    return core_result_ok();
}
