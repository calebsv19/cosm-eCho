#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
    echo "runtime-refresh contract check failed: $1" >&2
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

check_contains "static void mem_console_runtime_store_pending_intent(" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_contains "static void mem_console_runtime_clear_pending_intent(" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_contains "static void mem_console_runtime_apply_pending_intent_to_state(" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_contains "static void mem_console_runtime_store_in_flight_intent(" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime_refresh.c"
check_contains "static void mem_console_runtime_store_latest_refresh_error(" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"

check_contains "mem_console_runtime_store_pending_intent(runtime," \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_contains "mem_console_runtime_apply_pending_intent_to_state(runtime, &pending_state);" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_contains "mem_console_runtime_clear_pending_intent(runtime);" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_contains "mem_console_runtime_store_in_flight_intent(runtime, task);" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime_refresh.c"
check_contains "mem_console_runtime_capture_intent_from_state(state," \
    "${ROOT_DIR}/src/runtime/mem_console_runtime_refresh.c"
check_contains "mem_console_runtime_publish_metrics(runtime, state);" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_contains "mem_console_runtime_store_latest_refresh_error(runtime, error_text);" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_contains "runtime->latest_refresh_error_message" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_contains "state->runtime_latest_refresh_error_id = runtime->latest_refresh_error_id;" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime_refresh.c"
check_contains "state->runtime_latest_refresh_error_message" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime_refresh.c"
check_contains "last error: %s" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime_refresh.c"
check_contains "mem_console_left_panel_derive_runtime_summary(" \
    "${ROOT_DIR}/src/runtime/mem_console_state_roles.c"
check_contains "render_storage.runtime_summary_draw_line" \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"

check_absent "runtime->pending_selected_item_id = current_selected_item_id;" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_absent "pending_state.selected_item_id = runtime->pending_selected_item_id;" \
    "${ROOT_DIR}/src/runtime/mem_console_runtime.c"
check_absent "last error: %s" \
    "${ROOT_DIR}/src/ui/mem_console_ui_left_section.c"
echo "runtime-refresh contract checks ok: pending/in-flight intent and latest async error ownership stay runtime-owned and helper-routed"
