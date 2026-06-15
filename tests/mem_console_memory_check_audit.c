#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "mem_console_state.h"
#include "../src/db/mem_console_db_graph_sort.h"

#define MEM_CONSOLE_AUDIT_GRAPH_KIND_ALL_MASK 0xffu
#define MEM_CONSOLE_AUDIT_NODE_KIND_ISSUE_MASK (1u << 2)
#define MEM_CONSOLE_AUDIT_NODE_KIND_ALL_MASK 0x7fu

int mem_console_graph_node_kind_is_enabled(const MemConsoleState *state, const char *kind) {
    uint32_t kind_mask = 0u;

    if (!state || !kind || kind[0] == '\0') {
        return 1;
    }
    if (strcmp(kind, "issue") == 0) {
        kind_mask = MEM_CONSOLE_AUDIT_NODE_KIND_ISSUE_MASK;
    } else if (strcmp(kind, "decision") == 0) {
        kind_mask = 1u << 1;
    } else if (strcmp(kind, "summary") == 0) {
        kind_mask = 1u << 4;
    }
    if (kind_mask == 0u) {
        return 1;
    }
    if (state->graph_node_kind_filter_all_override) {
        return 1;
    }
    return (state->graph_node_kind_filter_mask & kind_mask) != 0u ? 1 : 0;
}

int mem_console_graph_edge_limit_clamp(int value) {
    if (value < MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN) {
        return MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN;
    }
    if (value > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        return MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }
    return value;
}

static void seed_node(MemConsoleGraphNode *node,
                      int64_t item_id,
                      int64_t created_ns,
                      const char *kind,
                      const char *project) {
    memset(node, 0, sizeof(*node));
    node->item_id = item_id;
    node->created_ns = created_ns;
    snprintf(node->title, sizeof(node->title), "node-%lld", (long long)item_id);
    snprintf(node->kind, sizeof(node->kind), "%s", kind);
    snprintf(node->project_key, sizeof(node->project_key), "%s", project);
}

static void seed_edge(MemConsoleGraphEdge *edge, int from_index, int to_index, const char *kind) {
    memset(edge, 0, sizeof(*edge));
    edge->from_index = from_index;
    edge->to_index = to_index;
    snprintf(edge->kind, sizeof(edge->kind), "%s", kind);
}

int main(void) {
    MemConsoleState state;

    memset(&state, 0, sizeof(state));
    state.selected_item_id = 3;
    state.graph_center_item_id = 2;
    state.graph_edge_limit_text[0] = '\0';
    state.graph_kind_filter_all_override = 1;
    state.graph_kind_filter_mask = MEM_CONSOLE_AUDIT_GRAPH_KIND_ALL_MASK;
    state.graph_node_kind_filter_all_override = 1;
    state.graph_node_kind_filter_mask = MEM_CONSOLE_AUDIT_NODE_KIND_ALL_MASK;

    state.graph_node_count = 4;
    seed_node(&state.graph_nodes[0], 1, 100, "issue", "mem_console");
    seed_node(&state.graph_nodes[1], 2, 200, "decision", "mem_console");
    seed_node(&state.graph_nodes[2], 3, 300, "summary", "mem_console");
    seed_node(&state.graph_nodes[3], 4, 400, "issue", "shared");

    state.graph_edge_count = 6;
    seed_edge(&state.graph_edges[0], 0, 1, "related");
    seed_edge(&state.graph_edges[1], 1, 2, "depends_on");
    seed_edge(&state.graph_edges[2], 2, 3, "supports");
    seed_edge(&state.graph_edges[3], 0, 3, "references");
    seed_edge(&state.graph_edges[4], 0, 2, "implements");
    seed_edge(&state.graph_edges[5], 1, 3, "blocks");

    mem_console_db_apply_graph_node_sort(&state);
    mem_console_db_apply_graph_edge_priority(&state, 2);
    assert(state.graph_edge_count == MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN);

    state.graph_node_kind_filter_all_override = 0;
    state.graph_node_kind_filter_mask = MEM_CONSOLE_AUDIT_NODE_KIND_ISSUE_MASK;
    mem_console_db_compact_graph_by_node_kind(&state);
    assert(state.graph_node_count == 2);

    puts("mem_console_memory_check_audit: success");
    return 0;
}
