#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_ROOT="$(cd "${ROOT_DIR}/.." && pwd)"
FIXTURE_DB="${ROOT_DIR}/build/test_visual_graph_fixture.sqlite"
FIXTURE_MANIFEST="${ROOT_DIR}/build/test_visual_graph_fixture.env"
CAPTURE_PLAN_ROOT="${ROOT_DIR}/build/test_visual_capture_plan"
MEM_CLI="${WORK_ROOT}/shared/core/core_memdb/build/mem_cli"

fail() {
    echo "visual fixture contract failed: $1" >&2
    exit 1
}

check_file_contains() {
    local pattern="$1"
    local file="$2"
    if ! rg -n --fixed-strings -e "${pattern}" "${file}" >/dev/null; then
        fail "missing pattern in ${file}: ${pattern}"
    fi
}

check_file_contains "--visual-review-mode" "${ROOT_DIR}/src/runtime/mem_console_state_paths.c"
check_file_contains "mem_console_parse_visual_review_mode" "${ROOT_DIR}/src/app/mem_console_app_main.c"
check_file_contains "mem_console_graph_view_mode_set(&ctx->state, ctx->visual_review_mode);" \
    "${ROOT_DIR}/src/app/mem_console_app_main.c"
check_file_contains "graph_camera_apply_focus_initial_fit(state," \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_layout.c"
check_file_contains "graph_camera_viewport_is_default_focus_reset" \
    "${ROOT_DIR}/src/ui/graph/mem_console_ui_graph_camera.c"
check_file_contains "ROOT_ID=" "${ROOT_DIR}/demo/reset_visual_graph_fixture.sh"
check_file_contains "visual-fixture-behavior-island" "${ROOT_DIR}/demo/reset_visual_graph_fixture.sh"
check_file_contains "visual-fixture-ray-bridge" "${ROOT_DIR}/demo/reset_visual_graph_fixture.sh"
check_file_contains "--plan-only" "${ROOT_DIR}/demo/capture_visual_graph_fixture.sh"
check_file_contains "capture_plan.env" "${ROOT_DIR}/demo/capture_visual_graph_fixture.sh"
check_file_contains "capture_manifest.tsv" "${ROOT_DIR}/demo/capture_visual_graph_fixture.sh"
check_file_contains "focus.stdout.log" "${ROOT_DIR}/demo/capture_visual_graph_fixture.sh"
check_file_contains "web.capture.json" "${ROOT_DIR}/demo/capture_visual_graph_fixture.sh"

"${ROOT_DIR}/demo/reset_visual_graph_fixture.sh" "${FIXTURE_DB}" "${FIXTURE_MANIFEST}" >/dev/null

[[ -f "${FIXTURE_DB}" ]] || fail "fixture DB was not created"
[[ -f "${FIXTURE_MANIFEST}" ]] || fail "fixture manifest was not created"

# shellcheck disable=SC1090
source "${FIXTURE_MANIFEST}"

[[ "${ROOT_ID:-0}" -gt 0 ]] || fail "manifest ROOT_ID missing"
[[ "${NODE_COUNT:-0}" -eq 16 ]] || fail "manifest NODE_COUNT mismatch"
[[ "${EDGE_COUNT:-0}" -eq 19 ]] || fail "manifest EDGE_COUNT mismatch"

"${MEM_CLI}" health --db "${FIXTURE_DB}" --format json | rg '"ok":1' >/dev/null ||
    fail "fixture DB health failed"

neighbor_count="$("${MEM_CLI}" neighbors --db "${FIXTURE_DB}" --item-id "${ROOT_ID}" --max-edges 16 --max-nodes 16 --format json | jq 'length')"
if [[ "${neighbor_count}" -lt 5 ]]; then
    fail "root should have at least five direct visual neighbors"
fi

rm -rf "${CAPTURE_PLAN_ROOT}"
"${ROOT_DIR}/demo/capture_visual_graph_fixture.sh" --plan-only --out-root "${CAPTURE_PLAN_ROOT}" >/dev/null

[[ -f "${CAPTURE_PLAN_ROOT}/capture_plan.env" ]] || fail "capture plan env was not created"
[[ -f "${CAPTURE_PLAN_ROOT}/capture_manifest.tsv" ]] || fail "capture manifest was not created"

check_file_contains "PLAN_ONLY=1" "${CAPTURE_PLAN_ROOT}/capture_plan.env"
check_file_contains "MODES=focus,pods,web" "${CAPTURE_PLAN_ROOT}/capture_plan.env"
check_file_contains $'focus\t'"${CAPTURE_PLAN_ROOT}/focus.png" "${CAPTURE_PLAN_ROOT}/capture_manifest.tsv"
check_file_contains $'pods\t'"${CAPTURE_PLAN_ROOT}/pods.png" "${CAPTURE_PLAN_ROOT}/capture_manifest.tsv"
check_file_contains $'web\t'"${CAPTURE_PLAN_ROOT}/web.png" "${CAPTURE_PLAN_ROOT}/capture_manifest.tsv"
check_file_contains "${CAPTURE_PLAN_ROOT}/focus.capture.json" "${CAPTURE_PLAN_ROOT}/capture_manifest.tsv"
check_file_contains "${CAPTURE_PLAN_ROOT}/web.stderr.log" "${CAPTURE_PLAN_ROOT}/capture_manifest.tsv"

echo "visual fixture contract checks passed"
