#include "mem_console_ui_graph_internal.h"

#include <string.h>

float graph_edge_route_hit_radius_for_zoom(const MemConsoleState *state) {
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

int graph_find_edge_index_at_point(const MemConsoleState *state,
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

int graph_build_node_hud_spec(MemConsoleState *state,
                              int hovered_node_index,
                              MemConsoleUiHudCardSpec *out_spec) {
    const MemConsoleGraphNode *hovered_node;
    const char *raw_body;

    if (!state || !out_spec || hovered_node_index < 0 || hovered_node_index >= state->graph_node_count) {
        return 0;
    }
    hovered_node = &state->graph_nodes[hovered_node_index];
    raw_body = hovered_node->body_preview[0] ? hovered_node->body_preview : "(no body)";

    graph_status_format_node_hud(state, hovered_node);
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
    out_spec->cache_key = graph_hud_hash_u64(1469598103934665603ull, state->graph_layout_signature);
    out_spec->cache_key = graph_hud_hash_u64(out_spec->cache_key, (uint64_t)hovered_node->item_id);
    out_spec->cache_key = graph_hud_hash_u64(out_spec->cache_key, 1ull);
    return 1;
}

int graph_build_edge_hud_spec(MemConsoleState *state,
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

    graph_status_format_edge_hud(state, from_node, to_node, edge_kind_label);
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
