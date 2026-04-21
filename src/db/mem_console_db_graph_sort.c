#include "mem_console_db_graph_sort.h"

#include <stdlib.h>
#include <string.h>

static int64_t graph_node_effective_anchor_item_id(const MemConsoleGraphNode *node) {
    if (!node) {
        return 0;
    }
    if (node->render_anchor_item_id > 0) {
        return node->render_anchor_item_id;
    }
    return node->item_id;
}

static int64_t graph_node_effective_anchor_created_ns(const MemConsoleGraphNode *node) {
    if (!node) {
        return 0;
    }
    if (node->render_anchor_created_ns > 0) {
        return node->render_anchor_created_ns;
    }
    return node->created_ns;
}

void mem_console_db_annotate_rollup_render_anchors(MemConsoleState *state) {
    int node_count;
    int edge_count;
    int i;

    if (!state) {
        return;
    }

    node_count = state->graph_node_count;
    if (node_count < 0) {
        node_count = 0;
    }
    if (node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }
    edge_count = state->graph_edge_count;
    if (edge_count < 0) {
        edge_count = 0;
    }
    if (edge_count > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        edge_count = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }

    for (i = 0; i < node_count; ++i) {
        state->graph_nodes[i].render_anchor_item_id = state->graph_nodes[i].item_id;
        state->graph_nodes[i].render_anchor_created_ns = state->graph_nodes[i].created_ns;
        state->graph_nodes[i].is_rollup_node = 0;
    }

    for (i = 0; i < edge_count; ++i) {
        int from_index = state->graph_edges[i].from_index;
        int to_index = state->graph_edges[i].to_index;
        MemConsoleGraphNode *rollup_node;
        const MemConsoleGraphNode *source_node;

        if (strcmp(state->graph_edges[i].kind, "summarizes") != 0) {
            continue;
        }
        if (from_index < 0 || to_index < 0 ||
            from_index >= node_count || to_index >= node_count) {
            continue;
        }

        rollup_node = &state->graph_nodes[from_index];
        source_node = &state->graph_nodes[to_index];
        if (source_node->item_id <= 0) {
            continue;
        }

        if (!rollup_node->is_rollup_node) {
            rollup_node->is_rollup_node = 1;
            rollup_node->render_anchor_item_id = source_node->item_id;
            rollup_node->render_anchor_created_ns = source_node->created_ns;
            continue;
        }

        if (source_node->item_id > rollup_node->render_anchor_item_id) {
            rollup_node->render_anchor_item_id = source_node->item_id;
            rollup_node->render_anchor_created_ns = source_node->created_ns;
        }
    }
}

static int graph_sort_nodes_should_swap(const MemConsoleState *state,
                                        const MemConsoleGraphNode *left,
                                        const MemConsoleGraphNode *right) {
    int64_t left_anchor_created_ns;
    int64_t right_anchor_created_ns;
    int64_t left_anchor_item_id;
    int64_t right_anchor_item_id;

    if (!state || !left || !right) {
        return 0;
    }

    left_anchor_created_ns = graph_node_effective_anchor_created_ns(left);
    right_anchor_created_ns = graph_node_effective_anchor_created_ns(right);
    left_anchor_item_id = graph_node_effective_anchor_item_id(left);
    right_anchor_item_id = graph_node_effective_anchor_item_id(right);

    if (state->graph_sort_mode == MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST) {
        if (left_anchor_created_ns > right_anchor_created_ns) {
            return 1;
        }
        if (left_anchor_created_ns < right_anchor_created_ns) {
            return 0;
        }
        if (left_anchor_item_id > right_anchor_item_id) {
            return 1;
        }
        if (left_anchor_item_id < right_anchor_item_id) {
            return 0;
        }
        return left->item_id > right->item_id ? 1 : 0;
    }

    if (left_anchor_created_ns < right_anchor_created_ns) {
        return 1;
    }
    if (left_anchor_created_ns > right_anchor_created_ns) {
        return 0;
    }
    if (left_anchor_item_id < right_anchor_item_id) {
        return 1;
    }
    if (left_anchor_item_id > right_anchor_item_id) {
        return 0;
    }
    return left->item_id < right->item_id ? 1 : 0;
}

void mem_console_db_apply_graph_node_sort(MemConsoleState *state) {
    MemConsoleGraphNode *sorted_nodes = 0;
    int *order = 0;
    int *old_to_new = 0;
    int node_count = 0;
    int edge_count = 0;
    int i;
    int j;
    int changed = 0;

    if (!state) {
        return;
    }

    node_count = state->graph_node_count;
    if (node_count <= 1) {
        return;
    }
    if (node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }
    edge_count = state->graph_edge_count;
    if (edge_count < 0) {
        edge_count = 0;
    }
    if (edge_count > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        edge_count = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }

    sorted_nodes = (MemConsoleGraphNode *)malloc((size_t)node_count * sizeof(*sorted_nodes));
    order = (int *)malloc((size_t)node_count * sizeof(*order));
    old_to_new = (int *)malloc((size_t)node_count * sizeof(*old_to_new));
    if (!sorted_nodes || !order || !old_to_new) {
        free(sorted_nodes);
        free(order);
        free(old_to_new);
        return;
    }

    for (i = 0; i < node_count; ++i) {
        order[i] = i;
    }

    for (i = 1; i < node_count; ++i) {
        int key = order[i];
        j = i - 1;
        while (j >= 0 &&
               graph_sort_nodes_should_swap(state,
                                            &state->graph_nodes[order[j]],
                                            &state->graph_nodes[key])) {
            order[j + 1] = order[j];
            j -= 1;
        }
        order[j + 1] = key;
    }

    for (i = 0; i < node_count; ++i) {
        if (order[i] != i) {
            changed = 1;
            break;
        }
    }
    if (!changed) {
        free(sorted_nodes);
        free(order);
        free(old_to_new);
        return;
    }

    for (i = 0; i < node_count; ++i) {
        sorted_nodes[i] = state->graph_nodes[order[i]];
        old_to_new[order[i]] = i;
    }
    for (i = 0; i < node_count; ++i) {
        state->graph_nodes[i] = sorted_nodes[i];
    }

    for (i = 0; i < edge_count; ++i) {
        int from_index = state->graph_edges[i].from_index;
        int to_index = state->graph_edges[i].to_index;
        if (from_index >= 0 && from_index < node_count) {
            state->graph_edges[i].from_index = old_to_new[from_index];
        }
        if (to_index >= 0 && to_index < node_count) {
            state->graph_edges[i].to_index = old_to_new[to_index];
        }
    }

    free(sorted_nodes);
    free(order);
    free(old_to_new);
}

void mem_console_db_apply_graph_edge_priority(MemConsoleState *state, int edge_limit) {
    MemConsoleGraphEdge *kept_edges = 0;
    int *order = 0;
    int node_count = 0;
    int edge_count = 0;
    int i;
    int j;

    if (!state) {
        return;
    }

    edge_limit = mem_console_graph_edge_limit_clamp(edge_limit);

    node_count = state->graph_node_count;
    if (node_count < 0) {
        node_count = 0;
    }
    if (node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }

    edge_count = state->graph_edge_count;
    if (edge_count < 0) {
        edge_count = 0;
    }
    if (edge_count > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        edge_count = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }
    if (edge_count <= edge_limit) {
        return;
    }

    kept_edges = (MemConsoleGraphEdge *)malloc((size_t)edge_limit * sizeof(*kept_edges));
    order = (int *)malloc((size_t)edge_count * sizeof(*order));
    if (!kept_edges || !order) {
        free(kept_edges);
        free(order);
        return;
    }

    for (i = 0; i < edge_count; ++i) {
        order[i] = i;
    }

    for (i = 1; i < edge_count; ++i) {
        int key = order[i];
        int key_from = state->graph_edges[key].from_index;
        int key_to = state->graph_edges[key].to_index;
        int key_low = MEM_CONSOLE_GRAPH_NODE_LIMIT + 1;
        int key_high = MEM_CONSOLE_GRAPH_NODE_LIMIT + 1;

        if (key_from >= 0 && key_from < node_count &&
            key_to >= 0 && key_to < node_count) {
            key_low = key_from < key_to ? key_from : key_to;
            key_high = key_from > key_to ? key_from : key_to;
        }

        j = i - 1;
        while (j >= 0) {
            int probe = order[j];
            int probe_from = state->graph_edges[probe].from_index;
            int probe_to = state->graph_edges[probe].to_index;
            int probe_low = MEM_CONSOLE_GRAPH_NODE_LIMIT + 1;
            int probe_high = MEM_CONSOLE_GRAPH_NODE_LIMIT + 1;
            int should_shift = 0;

            if (probe_from >= 0 && probe_from < node_count &&
                probe_to >= 0 && probe_to < node_count) {
                probe_low = probe_from < probe_to ? probe_from : probe_to;
                probe_high = probe_from > probe_to ? probe_from : probe_to;
            }

            if (probe_low > key_low) {
                should_shift = 1;
            } else if (probe_low == key_low && probe_high > key_high) {
                should_shift = 1;
            } else if (probe_low == key_low && probe_high == key_high && probe > key) {
                should_shift = 1;
            }

            if (!should_shift) {
                break;
            }
            order[j + 1] = probe;
            j -= 1;
        }
        order[j + 1] = key;
    }

    for (i = 0; i < edge_limit; ++i) {
        kept_edges[i] = state->graph_edges[order[i]];
    }

    memcpy(state->graph_edges, kept_edges, (size_t)edge_limit * sizeof(state->graph_edges[0]));
    state->graph_edge_count = edge_limit;

    free(kept_edges);
    free(order);
}

void mem_console_db_compact_graph_by_node_kind(MemConsoleState *state) {
    MemConsoleGraphNode *kept_nodes = 0;
    MemConsoleGraphEdge *kept_edges = 0;
    int *old_to_new = 0;
    int old_node_count;
    int old_edge_count;
    int kept_node_count = 0;
    int kept_edge_count = 0;
    int i;

    if (!state) {
        return;
    }

    old_node_count = state->graph_node_count;
    old_edge_count = state->graph_edge_count;
    if (old_node_count < 0) {
        old_node_count = 0;
    }
    if (old_node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        old_node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }
    if (old_edge_count < 0) {
        old_edge_count = 0;
    }
    if (old_edge_count > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        old_edge_count = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }

    if (old_node_count > 0) {
        kept_nodes = (MemConsoleGraphNode *)malloc((size_t)old_node_count * sizeof(*kept_nodes));
        old_to_new = (int *)malloc((size_t)old_node_count * sizeof(*old_to_new));
        if (!kept_nodes || !old_to_new) {
            free(kept_nodes);
            free(old_to_new);
            return;
        }
    }
    if (old_edge_count > 0) {
        kept_edges = (MemConsoleGraphEdge *)malloc((size_t)old_edge_count * sizeof(*kept_edges));
        if (!kept_edges) {
            free(kept_nodes);
            free(old_to_new);
            return;
        }
    }

    for (i = 0; i < old_node_count; ++i) {
        old_to_new[i] = -1;
        if (!mem_console_graph_node_kind_is_enabled(state, state->graph_nodes[i].kind)) {
            continue;
        }
        if (kept_node_count >= MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            continue;
        }
        kept_nodes[kept_node_count] = state->graph_nodes[i];
        old_to_new[i] = kept_node_count;
        kept_node_count += 1;
    }

    for (i = 0; i < old_edge_count; ++i) {
        int old_from = state->graph_edges[i].from_index;
        int old_to = state->graph_edges[i].to_index;
        int new_from;
        int new_to;

        if (kept_edge_count >= MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
            break;
        }
        if (old_from < 0 || old_to < 0 || old_from >= old_node_count || old_to >= old_node_count) {
            continue;
        }
        new_from = old_to_new[old_from];
        new_to = old_to_new[old_to];
        if (new_from < 0 || new_to < 0) {
            continue;
        }
        kept_edges[kept_edge_count] = state->graph_edges[i];
        kept_edges[kept_edge_count].from_index = new_from;
        kept_edges[kept_edge_count].to_index = new_to;
        kept_edge_count += 1;
    }

    if (kept_node_count > 0) {
        memcpy(state->graph_nodes, kept_nodes, (size_t)kept_node_count * sizeof(state->graph_nodes[0]));
    }
    if (kept_edge_count > 0) {
        memcpy(state->graph_edges, kept_edges, (size_t)kept_edge_count * sizeof(state->graph_edges[0]));
    }
    state->graph_node_count = kept_node_count;
    state->graph_edge_count = kept_edge_count;

    free(kept_nodes);
    free(kept_edges);
    free(old_to_new);
}
