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
    if ! rg -n --fixed-strings -e "${pattern}" "${file}" >/dev/null; then
        fail "missing pattern in ${file}: ${pattern}"
    fi
}

check_absent() {
    local pattern="$1"
    local file="$2"
    if rg -n --fixed-strings -e "${pattern}" "${file}" >/dev/null; then
        fail "unexpected pattern in ${file}: ${pattern}"
    fi
}

# S5/S9 selection contract: inspect-only selection updates detail without
# forcing graph recenter, while navigation/double-click still reseeds graph center.
check_contains "void mem_console_select_item_for_inspection(" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "void mem_console_select_item_for_navigation(" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "mem_console_selection_set(state, item_id);" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "mem_console_graph_center_set(state, item_id);" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "*io_action = MEM_CONSOLE_ACTION_REFRESH_DETAIL;" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "*io_action = MEM_CONSOLE_ACTION_REFRESH;" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "if (action == MEM_CONSOLE_ACTION_REFRESH_DETAIL)" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "result = refresh_selected_detail_from_db(db, state);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_select_item_for_navigation(state," \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_contains "mem_console_select_item_for_inspection(state," \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_contains "mem_console_select_item_for_navigation(state, hit_item_id, 1, 1, io_action);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_panel.c"
check_contains "mem_console_select_item_for_inspection(state, hit_item_id, 1, io_action);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_panel.c"
check_contains "mem_console_select_item_for_inspection(state, next_item_id, 1, io_action);" \
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
check_contains "if (state->graph_center_item_id != 0)" \
    "${ROOT_DIR}/src/db/mem_console_db_graph_load.c"
check_contains "return state->graph_center_item_id;" \
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

# MCU1-S1 focus readability contract: selected-root focus mode keeps primary
# neighbors visually ranked and keeps non-root edges from dominating the view.
check_contains "static void focus_layout_rank_root_neighbors(" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_focus_helpers.c"
check_contains "static float focus_primary_neighbor_angle(" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_focus_helpers.c"
check_contains "focus_layout_rank_root_neighbors(edges," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_focus_helpers.c"
check_contains "edge_touches_selected" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_draw.c"
check_contains "return emphasized_node ? 0.68f : 0.90f;" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_mode_policy.c"

# MCU1-S5 free-web topology contract: WEB owns a dedicated topology helper
# with component discovery, bridge scoring, and separated component anchors.
check_contains "src/ui/graph/mem_console_ui_graph_layout_web.c" \
    "${ROOT_DIR}/make/sources.mk"
check_contains "void apply_free_web_layout(KitRenderRect bounds," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_web.c"
check_contains "static void web_discover_components(" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_web.c"
check_contains "static void web_compute_degree_and_bridge_score(" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_web.c"
check_contains "static void web_component_anchor_for_rank(" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_web.c"
check_contains "static void web_place_component_nodes(" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_web.c"
check_contains "web_sort_nodes_by_bridge_score(bridge_score, degree, node_indices, count);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_web.c"
check_contains "apply_free_web_layout(world_bounds," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout.c"

# MCU1-S7 rendering refinement contract: all graph modes use explicit edge
# emphasis rules and FOCUS reserves enough space around the selected root.
check_contains "static int graph_draw_state_edge_is_focus_primary(" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_draw.c"
check_contains "static int graph_draw_endpoint_marker_allowed(" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_draw.c"
check_contains "edge_label_visible = is_hovered_edge ? 1 : 0;" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_draw.c"
check_contains "edge_label_visible = (is_hovered_edge || is_hierarchy_edge) ? 1 : 0;" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_draw.c"
check_contains "96.0f + (crowd * 16.0f)" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_focus_helpers.c"
check_contains "min_root_radius = 48.0f + (layouts[a].rect.width * 0.5f) + (crowd * 12.0f);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout_focus_helpers.c"
check_contains "void graph_camera_apply_focus_initial_fit(MemConsoleState *state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_camera.c"
check_contains "graph_camera_viewport_is_default_focus_reset" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_camera.c"
check_contains "target_screen_y = graph_bounds.y + (graph_bounds.height * 0.46f);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_camera.c"
check_contains "graph_camera_apply_focus_initial_fit(state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout.c"

# R1-S3 graph status derivation contract: graph diagnostic and HUD strings are
# built by the graph-local status helper instead of render composition files.
check_contains "src/ui/graph/mem_console_ui_graph_status.c" \
    "${ROOT_DIR}/make/sources.mk"
check_contains "void graph_status_format_view_line(MemConsoleState *state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_internal.h"
check_contains "void graph_status_format_node_hud(MemConsoleState *state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_internal.h"
check_contains "void graph_status_format_edge_hud(MemConsoleState *state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_internal.h"
check_contains "void graph_status_format_anchor_visibility_line(MemConsoleState *state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_internal.h"
check_contains "graph_status_format_view_line(state, node_count, edge_count);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_overlay.c"
check_contains "graph_status_format_node_hud(state, hovered_node);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_hud.c"
check_contains "graph_status_format_edge_hud(state, from_node, to_node, edge_kind_label);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_hud.c"
check_contains "graph_status_format_anchor_visibility_line(state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_panel.c"
check_absent "snprintf(state->graph_status_line" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_overlay.c"
check_absent "snprintf(state->graph_hud_" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_hud.c"
check_absent "snprintf(state->status_line" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_panel.c"

# R2-S5 graph viewport ownership contract: graph-camera helpers own live
# viewport mutation, while prefs-version helpers own persisted viewport capture
# and restore.
check_contains "static void graph_camera_store_live_viewport(" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_camera.c"
check_contains "void graph_camera_pan_live_viewport_by_screen_delta(MemConsoleState *state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_internal.h"
check_contains "void graph_camera_pan_live_viewport_by_screen_delta(MemConsoleState *state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_camera.c"
check_contains "graph_camera_store_live_viewport(&state->graph_viewport," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_camera.c"
check_contains "graph_camera_pan_live_viewport_by_screen_delta(state, delta_x, delta_y);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout.c"
check_contains "static void prefs_store_graph_viewport_from_state(" \
    "${ROOT_DIR}/src/runtime/mem_console_prefs_versions.c"
check_contains "static void prefs_apply_graph_viewport_to_state(" \
    "${ROOT_DIR}/src/runtime/mem_console_prefs_versions.c"
check_contains "prefs_store_graph_viewport_from_state(state, out_prefs);" \
    "${ROOT_DIR}/src/runtime/mem_console_prefs_versions.c"
check_contains "prefs_apply_graph_viewport_to_state(state," \
    "${ROOT_DIR}/src/runtime/mem_console_prefs_versions.c"
check_absent "graph_camera_pan_by_screen_delta(&state->graph_viewport" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout.c"

# R3-S4 graph diagnostics contract: graph action failures include selected,
# center, filter, limit, and hop context while lower-level layout failures tell
# the operator how to recover missing layout/selection state.
check_contains "static void mem_console_app_set_graph_action_error_status(" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "Graph %s failed (selected=%lld center=%lld kind=%s limit=%d hops=%d): %s" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_graph_action_error_status(state, \"load\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_graph_action_error_status(state, \"refresh\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_graph_action_error_status(state, \"center\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_graph_action_error_status(state, \"center selected\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "graph layout unavailable; refresh graph first" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout.c"
check_contains "select a memory before centering selected graph node" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout.c"
check_contains "selected memory not present in graph" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout.c"

echo "graph contract checks ok: S5 + MCU1-S1 + MCU1-S5 + MCU1-S7 + MCU1-S8 + MCU1-S9 + R1-S3/R1-S4 + R2-S5 + R3-S4 invariants present"
