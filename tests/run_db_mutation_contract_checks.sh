#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
    echo "db-mutation contract check failed: $1" >&2
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

check_contains "typedef enum MemConsoleItemFlag" \
    "${ROOT_DIR}/include/mem_console/mem_console_db.h"
check_contains "CoreResult set_selected_item_flag(" \
    "${ROOT_DIR}/include/mem_console/mem_console_db.h"
check_contains "static const char *selected_item_flag_update_sql(" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_contains "UPDATE mem_item SET pinned = ?1 WHERE id = ?2 AND archived_ns IS NULL RETURNING id;" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_contains "UPDATE mem_item SET canonical = ?1 WHERE id = ?2 AND archived_ns IS NULL RETURNING id;" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_contains "WHERE id = ?4 AND archived_ns IS NULL " \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_contains "selected memory not found" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_contains "title update returned unexpected id" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_contains "body update returned unexpected id" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_contains "flag update returned unexpected id" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_contains "set_selected_item_flag(db, state, MEM_CONSOLE_ITEM_FLAG_PINNED" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "set_selected_item_flag(db, state, MEM_CONSOLE_ITEM_FLAG_CANONICAL" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "static void mem_console_app_set_post_mutation_refresh_error_status(" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_post_mutation_refresh_error_status(state, \"create\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_post_mutation_refresh_error_status(state, \"title save\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_post_mutation_refresh_error_status(state, \"body save\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_post_mutation_refresh_error_status(state, \"pinned toggle\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"
check_contains "mem_console_app_set_post_mutation_refresh_error_status(state, \"canonical toggle\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"

check_absent "set_selected_flag(" \
    "${ROOT_DIR}/include/mem_console/mem_console_db.h"
check_absent "set_selected_flag(" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_absent "SET %s = ?1" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_absent "const char *field_name" \
    "${ROOT_DIR}/src/db/mem_console_db_mutations.c"
check_absent "mem_console_app_set_action_error_status(state, \"Refresh failed\", result);" \
    "${ROOT_DIR}/src/app/mem_console_app_actions.c"

echo "db-mutation contract checks ok: item flags use enum-backed constant SQL"
