#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
    echo "detail relationship contract check failed: $1" >&2
    exit 1
}

check_contains() {
    local pattern="$1"
    local file="$2"
    if ! rg -n --fixed-strings -e "${pattern}" "${file}" >/dev/null; then
        fail "missing pattern in ${file}: ${pattern}"
    fi
}

# MCU1-S2: selected-detail relationship inspector is backed by DB rows.
check_contains "MEM_CONSOLE_DETAIL_RELATIONSHIP_LIMIT = 32" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "typedef struct MemConsoleRelationshipItem" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "CoreResult read_selected_relationships(CoreMemDb *db, MemConsoleState *state);" \
    "${ROOT_DIR}/src/db/mem_console_db_internal.h"
check_contains "result = read_selected_relationships(db, state);" \
    "${ROOT_DIR}/src/db/mem_console_db.c"
check_contains "FROM mem_link l" \
    "${ROOT_DIR}/src/db/mem_console_db_reads.c"
check_contains "JOIN mem_item m ON m.id = CASE WHEN l.from_item_id = ?1 THEN l.to_item_id ELSE l.from_item_id END" \
    "${ROOT_DIR}/src/db/mem_console_db_reads.c"

# MCU1-S2: the detail pane exposes grouped rows. MCU1-S9 keeps row clicks
# inspect-only so relationship browsing does not recenter the graph.
check_contains "CoreResult mem_console_ui_detail_draw_relationships(" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "mem_console_detail_relationship_group_changed(prev, item)" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "render_storage.relationship_group_labels[group_index]" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "render_storage.relationship_row_labels[i]" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "mem_console_detail_derive_relationship_row_label" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "mem_console_select_item_for_inspection(state," \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "state->detail_connection_scroll = 0.0f;" \
    "${ROOT_DIR}/src/runtime/mem_console_state.c"

# MCU1-S3: relationship editing remains narrow, selected-link scoped, and refresh-driven.
check_contains "MEM_CONSOLE_ACTION_ADD_RELATIONSHIP = 22" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "MEM_CONSOLE_INPUT_RELATIONSHIP_TARGET = 5" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "CoreResult create_selected_relationship_to_target(CoreMemDb *db," \
    "${ROOT_DIR}/include/mem_console/mem_console_db.h"
check_contains "CoreResult cycle_selected_relationship_kind(CoreMemDb *db," \
    "${ROOT_DIR}/include/mem_console/mem_console_db.h"
check_contains "CoreResult remove_selected_relationship(CoreMemDb *db," \
    "${ROOT_DIR}/include/mem_console/mem_console_db.h"
check_contains "WHERE id = ?1 AND (from_item_id = ?2 OR to_item_id = ?2)" \
    "${ROOT_DIR}/src/db/mem_console_db_relationship_mutations.c"
check_contains "*io_action = MEM_CONSOLE_ACTION_CYCLE_RELATIONSHIP_KIND;" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "*io_action = MEM_CONSOLE_ACTION_REMOVE_RELATIONSHIP;" \
    "${ROOT_DIR}/src/ui/mem_console_ui_detail_relationships.c"
check_contains "result = create_selected_relationship_to_target(db, state, &link_id);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "result = cycle_selected_relationship_kind(db, state, &link_id);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "result = remove_selected_relationship(db, state, &link_id);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_post_mutation_refresh_error_status(state, \"relationship add\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_post_mutation_refresh_error_status(state, \"relationship kind change\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_post_mutation_refresh_error_status(state, \"relationship remove\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"

# R3-S4 relationship diagnostics contract: mutation failures include selected,
# selected-link, and target context, and lower-level validation distinguishes
# missing selected memory from missing selected relationship.
check_contains "static void mem_console_app_set_relationship_action_error_status(" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "Relationship %s failed (selected=%lld link=%lld target=%s): %s" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_relationship_action_error_status(state, \"add\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_relationship_action_error_status(state, \"kind change\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_relationship_action_error_status(state, \"remove\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "select a memory before adding relationship" \
    "${ROOT_DIR}/src/db/mem_console_db_relationship_mutations.c"
check_contains "select a memory before changing relationship kind" \
    "${ROOT_DIR}/src/db/mem_console_db_relationship_mutations.c"
check_contains "select a relationship before changing kind" \
    "${ROOT_DIR}/src/db/mem_console_db_relationship_mutations.c"
check_contains "select a memory before removing relationship" \
    "${ROOT_DIR}/src/db/mem_console_db_relationship_mutations.c"
check_contains "select a relationship before removing" \
    "${ROOT_DIR}/src/db/mem_console_db_relationship_mutations.c"

echo "detail relationship contract checks passed"
