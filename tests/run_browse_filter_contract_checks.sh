#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

check_contains() {
    local needle="$1"
    local file="$2"
    local label="$3"

    if ! rg -Fq "$needle" "$file"; then
        echo "browse filter contract failed: missing ${label} in ${file}" >&2
        exit 1
    fi
}

check_contains "MEM_CONSOLE_ACTION_TOGGLE_BROWSE_PINNED = 25" \
    include/mem_console/mem_console_types.h \
    "browse pinned action"
check_contains "MEM_CONSOLE_ACTION_TOGGLE_BROWSE_CANONICAL = 26" \
    include/mem_console/mem_console_types.h \
    "browse canonical action"
check_contains "MEM_CONSOLE_ACTION_CYCLE_BROWSE_KIND = 27" \
    include/mem_console/mem_console_types.h \
    "browse kind action"
check_contains "int browse_pinned_only;" \
    include/mem_console/mem_console_types.h \
    "browse pinned state"
check_contains "int browse_canonical_only;" \
    include/mem_console/mem_console_types.h \
    "browse canonical state"
check_contains "int browse_kind_index;" \
    include/mem_console/mem_console_types.h \
    "browse kind state"

check_contains "AND (?20 = 0 OR pinned = 1)" \
    src/db/mem_console_db_reads.c \
    "pinned SQL predicate"
check_contains "AND (?21 = 0 OR canonical = 1)" \
    src/db/mem_console_db_reads.c \
    "canonical SQL predicate"
check_contains "AND (?22 = '' OR kind = ?22)" \
    src/db/mem_console_db_reads.c \
    "kind SQL predicate"
check_contains "LIMIT ?23 OFFSET ?24" \
    src/db/mem_console_db_reads.c \
    "shifted window bind slots"

check_contains "mem_console_ui_left_draw_browse_filters" \
    src/ui/mem_console_ui_left_section.c \
    "left pane browse filter helper call"
check_contains "src/ui/mem_console_ui_left_browse_filters.c" \
    make/sources.mk \
    "browse filter source registration"
check_contains "state->browse_pinned_only = state->browse_pinned_only ? 0 : 1;" \
    src/app/mem_console_app_actions.c \
    "pinned action mutation"
check_contains "state->browse_canonical_only = state->browse_canonical_only ? 0 : 1;" \
    src/app/mem_console_app_actions.c \
    "canonical action mutation"
check_contains "mem_console_browse_kind_cycle(state)" \
    src/app/mem_console_app_actions.c \
    "kind cycle action"

check_contains "out_browse_pinned_only" \
    src/runtime/mem_console_runtime_refresh.c \
    "runtime captured pinned filter"
check_contains "pending_browse_pinned_only" \
    src/runtime/mem_console_runtime.c \
    "runtime pending pinned filter"
check_contains "in_flight_browse_pinned_only" \
    include/mem_console/mem_console_runtime.h \
    "runtime in-flight pinned filter"

echo "browse filter contract checks passed."
