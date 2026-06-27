#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
    echo "data-path contract check failed: $1" >&2
    exit 1
}

check_contains() {
    local pattern="$1"
    local file="$2"
    if ! rg -n --fixed-strings "${pattern}" "${file}" >/dev/null; then
        fail "missing pattern in ${file}: ${pattern}"
    fi
}

check_contains "int mem_console_path_contract_normalize(" \
    "${ROOT_DIR}/src/runtime/mem_console_state_paths.c"
check_contains "int mem_console_path_has_sqlite_suffix(" \
    "${ROOT_DIR}/src/runtime/mem_console_state_paths.c"
check_contains "const char *mem_console_db_path_policy_error(" \
    "${ROOT_DIR}/src/runtime/mem_console_state_paths.c"
check_contains "int mem_console_build_app_prefs_path_for_output_root(" \
    "${ROOT_DIR}/src/runtime/mem_console_prefs.c"
check_contains "mem_console_build_app_prefs_path_for_output_root(ctx->output_root" \
    "${ROOT_DIR}/src/app/mem_console_app_main.c"
check_contains ".local/share/mem_console/mem_console.app.pack" \
    "${ROOT_DIR}/src/app/mem_console_app_main.c"

# S3 matrix lock: open/switch is reference mode and returns before suffix rewrite.
check_contains "if (!state->db_modal_create_mode) {" \
    "${ROOT_DIR}/src/runtime/mem_console_state_db_picker.c"
check_contains "Open/switch is explicit reference mode: validate without suffix rewrite." \
    "${ROOT_DIR}/src/runtime/mem_console_state_db_picker.c"
check_contains "return mem_console_db_path_is_safe(out_path);" \
    "${ROOT_DIR}/src/runtime/mem_console_state_db_picker.c"
check_contains "mem_console_db_path_policy_error(next_db_path)" \
    "${ROOT_DIR}/src/app/mem_console_app_db_switch.c"

# S3 create-name behavior: bare names route under input_root with sqlite suffix.
check_contains "mem_console_path_has_sqlite_suffix(out_path) ? \"%s/%s\" : \"%s/%s.sqlite\"" \
    "${ROOT_DIR}/src/runtime/mem_console_state_db_picker.c"
check_contains "const char *create_root = state->input_root[0] ? state->input_root : state->output_root;" \
    "${ROOT_DIR}/src/runtime/mem_console_state_db_picker.c"

# S2 keyboard controls remain present.
check_contains "MEM_CONSOLE_ACTION_BEGIN_INPUT_ROOT_PICKER" \
    "${ROOT_DIR}/src/app/mem_console_app_events.c"
check_contains "MEM_CONSOLE_ACTION_PICK_INPUT_ROOT_FOLDER" \
    "${ROOT_DIR}/src/app/mem_console_app_events.c"

# S6 list-picker behavior for LOAD DB.
check_contains "void mem_console_db_picker_rescan_entries(" \
    "${ROOT_DIR}/src/runtime/mem_console_state_db_picker.c"
check_contains "int mem_console_db_picker_move_selection(" \
    "${ROOT_DIR}/src/runtime/mem_console_state_db_picker.c"
check_contains "db_picker_entry_names" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "db_picker_entry_paths" \
    "${ROOT_DIR}/include/mem_console/mem_console_types.h"
check_contains "No .sqlite DBs found in input root." \
    "${ROOT_DIR}/src/ui/mem_console_ui.c"

echo "data-path contract checks ok: S1-S6 invariants present"
