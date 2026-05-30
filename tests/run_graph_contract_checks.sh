#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
    echo "graph contract check failed: $1" >&2
    exit 1
}

check_contains() {
    local pattern="$1"
    local file="$2"
    if ! rg -n --fixed-strings "${pattern}" "${file}" >/dev/null; then
        fail "missing pattern in ${file}: ${pattern}"
    fi
}

# S5 selection contract: one shared navigation helper drives graph/list reseed.
check_contains "void mem_console_select_item_for_navigation(" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "state->selected_item_id = item_id;" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "state->graph_center_item_id = item_id;" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "*io_action = MEM_CONSOLE_ACTION_REFRESH;" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "mem_console_select_item_for_navigation(state," \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_contains "mem_console_select_item_for_navigation(state, hit_item_id, 1, 1, io_action);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_panel.c"
check_contains "mem_console_select_item_for_navigation(state, hit_item_id, 1, 0, io_action);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_panel.c"
check_contains "mem_console_select_item_for_navigation(state, next_item_id, 1, 0, io_action);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_panel.c"

# S5 graph mode contract: explicit user-facing modes remain wired through controls.
check_contains "MEM_CONSOLE_GRAPH_VIEW_FOCUS = 0" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "MEM_CONSOLE_GRAPH_VIEW_PODS = 1" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "MEM_CONSOLE_GRAPH_VIEW_WEB = 2" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "int mem_console_graph_view_mode_set(MemConsoleState *state, int view_mode);" \
    "${ROOT_DIR}/include/mem_console/mem_console_state.h"
check_contains "label = \"FOCUS\";" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_controls.c"
check_contains "label = \"PODS\";" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_controls.c"
check_contains "label = \"WEB\";" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_controls.c"

# S5 center-aware budgeting: broad/full graph loads reserve the root neighborhood first.
check_contains "static CoreResult load_priority_graph_nodes(CoreMemDb *db," \
    "${ROOT_DIR}/src/db/mem_console_db_graph_load.c"
check_contains "priority_root_item_id = graph_priority_root_item_id(state);" \
    "${ROOT_DIR}/src/db/mem_console_db_graph_load.c"
check_contains "result = load_priority_graph_nodes(db," \
    "${ROOT_DIR}/src/db/mem_console_db_graph_load.c"
check_contains "result = load_full_scope_graph_nodes(db, state, sort_oldest_first);" \
    "${ROOT_DIR}/src/db/mem_console_db_graph_load.c"
check_contains "state->graph_center_item_id = state->selected_item_id;" \
    "${ROOT_DIR}/src/db/mem_console_db_graph_load.c"

# S5 node/edge ranking: distance-aware prioritization must remain active.
check_contains "static void graph_compute_node_distances(const MemConsoleState *state," \
    "${ROOT_DIR}/src/db/mem_console_db_graph_sort.c"
check_contains "primary_seed_index = graph_find_node_index_by_item_id(state, state->selected_item_id);" \
    "${ROOT_DIR}/src/db/mem_console_db_graph_sort.c"
check_contains "graph_compute_node_distances(state," \
    "${ROOT_DIR}/src/db/mem_console_db_graph_sort.c"
check_contains "probe_primary_min_depth > key_primary_min_depth" \
    "${ROOT_DIR}/src/db/mem_console_db_graph_sort.c"
check_contains "probe_touches_selected < key_touches_selected" \
    "${ROOT_DIR}/src/db/mem_console_db_graph_sort.c"

echo "graph contract checks ok: S5 invariants present"
