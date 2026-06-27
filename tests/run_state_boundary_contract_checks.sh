#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
    echo "state-boundary contract check failed: $1" >&2
    exit 1
}

check_contains() {
    local pattern="$1"
    local file="$2"
    if ! rg -n --fixed-strings "${pattern}" "${file}" >/dev/null; then
        fail "missing pattern in ${file}: ${pattern}"
    fi
}

check_absent() {
    local pattern="$1"
    local file="$2"
    if rg -n --fixed-strings "${pattern}" "${file}" >/dev/null; then
        fail "unexpected pattern in ${file}: ${pattern}"
    fi
}

check_contains "typedef struct MemConsoleLeftPanelRenderState" \
    "${ROOT_DIR}/include/mem_console/mem_console_state_roles.h"
check_contains "typedef struct MemConsoleLeftPanelRenderStorage" \
    "${ROOT_DIR}/include/mem_console/mem_console_state_roles.h"
check_contains "typedef struct MemConsoleDetailRenderState" \
    "${ROOT_DIR}/include/mem_console/mem_console_state_roles.h"
check_contains "typedef struct MemConsoleDetailRenderStorage" \
    "${ROOT_DIR}/include/mem_console/mem_console_state_roles.h"
check_contains "typedef struct MemConsoleGraphRenderState" \
    "${ROOT_DIR}/include/mem_console/mem_console_state_roles.h"
check_contains "typedef struct MemConsoleGraphRenderStorage" \
    "${ROOT_DIR}/include/mem_console/mem_console_state_roles.h"
check_contains "mem_console_left_panel_render_state_from_state" \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_contains "mem_console_left_panel_derive_input_root_summary" \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_contains "mem_console_detail_derive_title_lines" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_section.c"
check_contains "mem_console_detail_derive_meta_line" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_section.c"
check_contains "mem_console_detail_derive_relationship_row_label" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "mem_console_detail_derive_relationship_group_label" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "mem_console_graph_derive_edge_label" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_draw.c"
check_contains "mem_console_graph_derive_node_label" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_draw.c"
check_contains "input_root_summary_draw_line" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "MEM_CONSOLE_GRAPH_EDGE_LABEL_CAP" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "MEM_CONSOLE_GRAPH_NODE_LABEL_CAP" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "typedef enum MemConsoleActionRole" \
    "${ROOT_DIR}/include/mem_console/mem_console_action_roles.h"
check_contains "mem_console_action_role(action)" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "MEM_CONSOLE_ACTION_ROLE_UNKNOWN" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "src/runtime/mem_console_state_roles.c" \
    "${ROOT_DIR}/make/sources.mk"
check_contains "src/app/mem_console_action_roles.c" \
    "${ROOT_DIR}/make/sources.mk"
check_contains "src/app/mem_console_app_status.c" \
    "${ROOT_DIR}/make/sources.mk"
check_contains "void mem_console_app_set_statusf(MemConsoleState *state, const char *fmt, ...);" \
    "${ROOT_DIR}/src/app/mem_console_app_internal.h"
check_contains "void mem_console_app_set_path_result_status(MemConsoleState *state," \
    "${ROOT_DIR}/src/app/mem_console_app_internal.h"
check_contains "mem_console_app_set_statusf(state, \"%s failed for %s: %s\", operation, path, message);" \
    "${ROOT_DIR}/src/app/mem_console_app_status.c"
check_contains "mem_console_app_set_path_result_status(state, \"DB open\", next_db_path, result);" \
    "${ROOT_DIR}/src/app/mem_console_app_db_switch.c"
check_contains "mem_console_app_set_path_result_status(state, \"UI prefs load\", prefs_path, result);" \
    "${ROOT_DIR}/src/app/mem_console_app_db_switch.c"
check_contains "mem_console_app_set_path_result_status(ctx->state, \"Pane prefs save\", ctx->prefs_path, result);" \
    "${ROOT_DIR}/src/app/mem_console_app_loop.c"
check_contains "mem_console_app_set_path_result_status(&ctx->state," \
    "${ROOT_DIR}/src/app/mem_console_app_main.c"
check_contains "mem_console_app_set_path_result_status(state," \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "void mem_console_input_target_set(MemConsoleState *state, MemConsoleInputTarget input_target);" \
    "${ROOT_DIR}/include/mem_console/mem_console_state.h"
check_contains "void mem_console_input_target_set(MemConsoleState *state, MemConsoleInputTarget input_target)" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "void mem_console_pane_prefs_mark_dirty(MemConsoleState *state);" \
    "${ROOT_DIR}/include/mem_console/mem_console_state.h"
check_contains "void mem_console_pane_prefs_mark_clean(MemConsoleState *state);" \
    "${ROOT_DIR}/include/mem_console/mem_console_state.h"
check_contains "void mem_console_selection_set(MemConsoleState *state, int64_t item_id);" \
    "${ROOT_DIR}/include/mem_console/mem_console_state.h"
check_contains "void mem_console_graph_center_set(MemConsoleState *state, int64_t item_id);" \
    "${ROOT_DIR}/include/mem_console/mem_console_state.h"
check_contains "void mem_console_selection_center_on(MemConsoleState *state, int64_t item_id);" \
    "${ROOT_DIR}/include/mem_console/mem_console_state.h"
check_contains "void mem_console_selection_apply_refreshed(MemConsoleState *state, const MemConsoleState *refreshed);" \
    "${ROOT_DIR}/include/mem_console/mem_console_state.h"
check_contains "void mem_console_pane_prefs_mark_dirty(MemConsoleState *state)" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "void mem_console_pane_prefs_mark_clean(MemConsoleState *state)" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "void mem_console_selection_set(MemConsoleState *state, int64_t item_id)" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "void mem_console_graph_center_set(MemConsoleState *state, int64_t item_id)" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "void mem_console_selection_center_on(MemConsoleState *state, int64_t item_id)" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "void mem_console_selection_apply_refreshed(MemConsoleState *state, const MemConsoleState *refreshed)" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"
check_contains "mem_console_app_set_path_result_status(ctx->state, \"Pane prefs save\", ctx->prefs_path, result);" \
    "${ROOT_DIR}/src/app/mem_console_app_loop.c"
check_contains "mem_console_app_set_statusf(state, \"Active DB switched to %s.\", state->db_path);" \
    "${ROOT_DIR}/src/app/mem_console_app_db_switch.c"
check_contains "mem_console_app_set_statusf(state, \"Theme switched to %s.\", state->theme_name);" \
    "${ROOT_DIR}/src/app/mem_console_app_theme.c"
check_contains "mem_console_app_set_statusf(state, \"Graph edge limit set to %d.\", parsed_limit);" \
    "${ROOT_DIR}/src/app/mem_console_app_events.c"
check_contains "mem_console_app_set_statusf(state, \"Authoring applied.\");" \
    "${ROOT_DIR}/src/app/mem_console_workspace_authoring_host.c"
check_contains "mem_console_app_set_statusf(&ctx->state, \"UI prefs restored.\");" \
    "${ROOT_DIR}/src/app/mem_console_app_main.c"
check_contains "mem_console_input_target_set(state, MEM_CONSOLE_INPUT_TITLE_EDIT);" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_section.c"
check_contains "mem_console_input_target_set(state, MEM_CONSOLE_INPUT_DB_PATH);" \
    "${ROOT_DIR}/src/ui/mem_console_ui.c"
check_contains "mem_console_input_target_set(state, MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT);" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_controls.c"
check_contains "mem_console_input_target_set(state, MEM_CONSOLE_INPUT_SEARCH);" \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_contains "mem_console_input_target_set(state, MEM_CONSOLE_INPUT_RELATIONSHIP_TARGET);" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "mem_console_pane_prefs_mark_dirty(ctx->state);" \
    "${ROOT_DIR}/src/app/mem_console_app_loop.c"
check_contains "mem_console_pane_prefs_mark_clean(ctx->state);" \
    "${ROOT_DIR}/src/app/mem_console_app_loop.c"
check_contains "mem_console_pane_prefs_mark_dirty(state);" \
    "${ROOT_DIR}/src/app/mem_console_app_theme.c"
check_contains "mem_console_pane_prefs_mark_dirty(state);" \
    "${ROOT_DIR}/src/layout/mem_console_pane_layout.c"
check_contains "mem_console_pane_prefs_mark_clean(state);" \
    "${ROOT_DIR}/src/runtime/mem_console_prefs.c"
check_contains "mem_console_pane_prefs_mark_clean(&ctx->state);" \
    "${ROOT_DIR}/src/app/mem_console_app_main.c"
check_contains "mem_console_selection_set(state, state->visible_items[0].id);" \
    "${ROOT_DIR}/src/db/mem_console_db.c"
check_contains "mem_console_graph_center_set(state, state->selected_item_id);" \
    "${ROOT_DIR}/src/db/mem_console_db.c"
check_contains "mem_console_graph_center_set(state, state->graph_nodes[0].item_id);" \
    "${ROOT_DIR}/src/db/mem_console_db_graph_load.c"
check_contains "mem_console_selection_center_on(&ctx->state, ctx->visual_review_selected_id);" \
    "${ROOT_DIR}/src/app/mem_console_app_main.c"
check_contains "mem_console_selection_set(state, created_id);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_selection_apply_refreshed(state, refreshed);" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime_refresh.c"

check_absent "left_panel_format_updated_ns" \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_absent "snprintf(state->list_item_labels" \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_absent "snprintf(state->db_summary_line" \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_absent "snprintf(state->visible_summary_line" \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
check_absent "format_created_timestamp_compact" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_section.c"
check_absent "snprintf(state->detail_meta_line" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_section.c"
check_absent "relationship_format_group_label" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_absent "relationship_format_row_label" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_absent "snprintf(state->detail_relationship_row_labels" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_absent "snprintf(state->detail_connection_summary_lines" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_absent "snprintf(state->graph_draw_edge_labels" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_draw.c"
check_absent "snprintf(state->graph_draw_node_labels" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_draw.c"
check_absent "snprintf(ctx->state->status_line" \
    "${ROOT_DIR}/src/app/mem_console_app_loop.c"
check_absent "snprintf(state->status_line" \
    "${ROOT_DIR}/src/app/mem_console_app_db_switch.c"
check_absent "snprintf(state->status_line" \
    "${ROOT_DIR}/src/app/mem_console_app_theme.c"
check_absent "UI prefs load failed." \
    "${ROOT_DIR}/src/app/mem_console_app_main.c"
check_absent "UI prefs load failed." \
    "${ROOT_DIR}/src/app/mem_console_app_db_switch.c"
check_absent "Pane prefs save failed." \
    "${ROOT_DIR}/src/app/mem_console_app_loop.c"
check_absent "DB switch failed: %s" \
    "${ROOT_DIR}/src/app/mem_console_app_loop.c"
check_absent "Input root set; app prefs save failed." \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_absent "snprintf(state->status_line" \
    "${ROOT_DIR}/src/app/mem_console_app_events.c"
check_absent "snprintf(state->status_line" \
    "${ROOT_DIR}/src/app/mem_console_workspace_authoring_host.c"
check_absent "snprintf(ctx->state.status_line" \
    "${ROOT_DIR}/src/app/mem_console_app_main.c"
check_absent "state->input_target = MEM_CONSOLE_INPUT_" \
    "${ROOT_DIR}/src/ui"
check_absent "state->input_target = MEM_CONSOLE_INPUT_" \
    "${ROOT_DIR}/src/app"
check_absent "state->input_target = MEM_CONSOLE_INPUT_" \
    "${ROOT_DIR}/src/runtime/mem_console_state_core.c"
check_absent "state->input_target = MEM_CONSOLE_INPUT_" \
    "${ROOT_DIR}/src/runtime/mem_console_state_db_picker.c"
check_absent "pane_prefs_dirty =" \
    "${ROOT_DIR}/src/app"
check_absent "pane_prefs_dirty =" \
    "${ROOT_DIR}/src/layout"
check_absent "pane_prefs_dirty =" \
    "${ROOT_DIR}/src/ui"
check_absent "pane_prefs_dirty =" \
    "${ROOT_DIR}/src/runtime/mem_console_prefs.c"
check_absent "pane_prefs_dirty =" \
    "${ROOT_DIR}/src/runtime/mem_console_state_core.c"
check_absent "state->selected_item_id = " \
    "${ROOT_DIR}/src/app"
check_absent "state->graph_center_item_id = " \
    "${ROOT_DIR}/src/app"
check_absent "state->selected_item_id = " \
    "${ROOT_DIR}/src/db"
check_absent "state->graph_center_item_id = " \
    "${ROOT_DIR}/src/db"
check_absent "state->selected_item_id = " \
    "${ROOT_DIR}/src/runtime/mem_console_prefs_versions.c"
check_absent "state->selected_item_id = " \
    "${ROOT_DIR}/src/runtime/mem_console_runtime_refresh.c"
check_absent "state->graph_center_item_id = " \
    "${ROOT_DIR}/src/runtime/mem_console_runtime_refresh.c"

echo "state-boundary contract checks ok: left/detail/graph render derivation, action roles, app status helpers, input-target ownership, pane-prefs dirty ownership, and selection/graph-center ownership are explicit"
