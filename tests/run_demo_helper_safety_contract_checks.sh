#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_ROOT="$(cd "${ROOT_DIR}/.." && pwd)"
LIVE_DB="${WORK_ROOT}/data/codework_mem_console.sqlite"
BUILD_DB="${ROOT_DIR}/build/test_demo_helper_safety.sqlite"
BUILD_MANIFEST="${BUILD_DB}.manifest"

fail() {
    echo "demo helper safety contract failed: $1" >&2
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

check_contains "mem_console_demo_assert_safe_db_path()" "${ROOT_DIR}/demo/demo_db_safety.sh"
check_contains "MEM_CONSOLE_ALLOW_NON_DEMO_DB=1" "${ROOT_DIR}/demo/demo_db_safety.sh"
check_contains "Refusing to write non-demo DB" "${ROOT_DIR}/demo/demo_db_safety.sh"

for helper in reset_demo_db.sh reset_visual_graph_fixture.sh seed_large_list.sh; do
    check_contains "source \"\${ROOT_DIR}/mem_console/demo/demo_db_safety.sh\"" "${ROOT_DIR}/demo/${helper}"
    check_contains "mem_console_demo_assert_safe_db_path" "${ROOT_DIR}/demo/${helper}"
    check_absent 'DEFAULT_DB_PATH="${CODEWORK_MEMDB_PATH}"' "${ROOT_DIR}/demo/${helper}"
done

set +e
CODEWORK_MEMDB_PATH="${LIVE_DB}" "${ROOT_DIR}/demo/reset_demo_db.sh" >"${ROOT_DIR}/build/demo_helper_live_reset.stdout" 2>"${ROOT_DIR}/build/demo_helper_live_reset.stderr"
reset_rc=$?
set -e
if [[ "${reset_rc}" -ne 0 ]]; then
    fail "reset_demo_db default should ignore CODEWORK_MEMDB_PATH and use the demo DB"
fi

set +e
"${ROOT_DIR}/demo/reset_demo_db.sh" "${LIVE_DB}" >"${ROOT_DIR}/build/demo_helper_blocked.stdout" 2>"${ROOT_DIR}/build/demo_helper_blocked.stderr"
blocked_rc=$?
set -e
if [[ "${blocked_rc}" -eq 0 ]]; then
    fail "reset_demo_db should reject an explicit live workspace DB by default"
fi
rg -n --fixed-strings "Refusing to write non-demo DB" "${ROOT_DIR}/build/demo_helper_blocked.stderr" >/dev/null ||
    fail "blocked live DB message missing"

"${ROOT_DIR}/demo/reset_visual_graph_fixture.sh" "${BUILD_DB}" "${BUILD_MANIFEST}" >/dev/null
[[ -f "${BUILD_DB}" ]] || fail "build-lane fixture DB was not created"
[[ -f "${BUILD_MANIFEST}" ]] || fail "build-lane fixture manifest was not created"

echo "demo helper safety contract checks passed"
