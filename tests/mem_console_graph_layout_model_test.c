#include "ui/graph/mem_console_ui_graph_internal.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

float graph_clampf(float value, float min_v, float max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

static float layout_center_x(const KitGraphStructNodeLayout *layout) {
    return layout->rect.x + (layout->rect.width * 0.5f);
}

static float layout_center_y(const KitGraphStructNodeLayout *layout) {
    return layout->rect.y + (layout->rect.height * 0.5f);
}

static float distance_from(float ax, float ay, float bx, float by) {
    float dx = ax - bx;
    float dy = ay - by;
    return sqrtf((dx * dx) + (dy * dy));
}

static void seed_layout(KitGraphStructNodeLayout *layout, uint32_t node_id) {
    layout->node_id = node_id;
    layout->rect.x = 20.0f + ((float)node_id * 8.0f);
    layout->rect.y = 20.0f + ((float)node_id * 6.0f);
    layout->rect.width = 52.0f;
    layout->rect.height = 18.0f;
    layout->depth = 0u;
}

static void seed_graph_state(MemConsoleState *state,
                             KitGraphStructNodeLayout *layouts,
                             KitGraphStructEdge *edges) {
    int i;

    memset(state, 0, sizeof(*state));
    state->graph_node_count = 5;
    state->selected_item_id = 101;
    state->graph_center_item_id = 101;

    for (i = 0; i < state->graph_node_count; ++i) {
        state->graph_nodes[i].item_id = 101 + i;
        (void)snprintf(state->graph_nodes[i].title, sizeof(state->graph_nodes[i].title), "Node %d", i + 1);
        seed_layout(&layouts[i], (uint32_t)i + 1u);
    }

    (void)snprintf(state->graph_nodes[0].project_key, sizeof(state->graph_nodes[0].project_key), "alpha");
    (void)snprintf(state->graph_nodes[1].project_key, sizeof(state->graph_nodes[1].project_key), "beta");
    (void)snprintf(state->graph_nodes[2].project_key, sizeof(state->graph_nodes[2].project_key), "alpha");
    (void)snprintf(state->graph_nodes[3].project_key, sizeof(state->graph_nodes[3].project_key), "gamma");
    (void)snprintf(state->graph_nodes[4].project_key, sizeof(state->graph_nodes[4].project_key), "gamma");
    state->graph_nodes[1].canonical = 1;
    state->graph_nodes[3].pinned = 1;

    edges[0].from_id = 1u;
    edges[0].to_id = 2u;
    edges[1].from_id = 2u;
    edges[1].to_id = 3u;
    edges[2].from_id = 4u;
    edges[2].to_id = 5u;
    state->graph_edge_count = 3;
}

static void test_web_layout_keeps_root_center_and_components_separated(void) {
    MemConsoleState state;
    KitGraphStructNodeLayout layouts[5];
    KitGraphStructEdge edges[3];
    KitRenderRect bounds = { 0.0f, 0.0f, 420.0f, 300.0f };
    float root_x;
    float root_y;
    float bounds_center_x = bounds.x + (bounds.width * 0.5f);
    float bounds_center_y = bounds.y + (bounds.height * 0.5f);
    float connected_neighbor_distance;
    float secondary_a_distance;
    float secondary_b_distance;

    seed_graph_state(&state, layouts, edges);
    apply_free_web_layout(bounds, &state, edges, 3u, layouts, 5u);

    root_x = layout_center_x(&layouts[0]);
    root_y = layout_center_y(&layouts[0]);
    connected_neighbor_distance = distance_from(layout_center_x(&layouts[1]),
                                                layout_center_y(&layouts[1]),
                                                root_x,
                                                root_y);
    secondary_a_distance = distance_from(layout_center_x(&layouts[3]),
                                         layout_center_y(&layouts[3]),
                                         root_x,
                                         root_y);
    secondary_b_distance = distance_from(layout_center_x(&layouts[4]),
                                         layout_center_y(&layouts[4]),
                                         root_x,
                                         root_y);

    assert(fabsf(root_x - bounds_center_x) < 0.01f);
    assert(fabsf(root_y - bounds_center_y) < 0.01f);
    assert(connected_neighbor_distance < secondary_a_distance);
    assert(connected_neighbor_distance < secondary_b_distance);
    assert(secondary_a_distance > 56.0f);
    assert(secondary_b_distance > 56.0f);
    assert(layout_center_y(&layouts[3]) < root_y);
    assert(layout_center_y(&layouts[4]) < root_y);
}

int main(void) {
    test_web_layout_keeps_root_center_and_components_separated();
    puts("mem_console_graph_layout_model_test: success");
    return 0;
}
