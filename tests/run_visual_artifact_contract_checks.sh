#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_ROOT="${ROOT_DIR}/build/test_visual_artifact_contract"
LOG_PATH="${OUT_ROOT}/visual_artifact.stdout.log"
ARTIFACT_PATH="${OUT_ROOT}/mem_console_first_frame_web.svg"

fail() {
    echo "visual artifact contract failed: $1" >&2
    exit 1
}

check_file_contains() {
    local pattern="$1"
    local file="$2"
    if ! rg -n --fixed-strings -e "${pattern}" "${file}" >/dev/null; then
        fail "missing pattern in ${file}: ${pattern}"
    fi
}

rm -rf "${OUT_ROOT}"
mkdir -p "${OUT_ROOT}"

"${ROOT_DIR}/demo/render_visual_artifact.sh" --out-root "${OUT_ROOT}" --mode web >"${LOG_PATH}" 2>&1

check_file_contains "visual-artifact: ${ARTIFACT_PATH}" "${LOG_PATH}"
check_file_contains "visual-artifact ready: ${ARTIFACT_PATH}" "${LOG_PATH}"

[[ -s "${ARTIFACT_PATH}" ]] || fail "artifact missing or empty: ${ARTIFACT_PATH}"

check_file_contains '<svg xmlns="http://www.w3.org/2000/svg" width="1440" height="900"' "${ARTIFACT_PATH}"
check_file_contains "<title>mem_console visual artifact first frame</title>" "${ARTIFACT_PATH}"
check_file_contains "Recorded from the app-owned first-frame render command stream." "${ARTIFACT_PATH}"
check_file_contains "MEMORY CONSOLE" "${ARTIFACT_PATH}"
check_file_contains "mode:web" "${ARTIFACT_PATH}"
check_file_contains "<rect " "${ARTIFACT_PATH}"
check_file_contains "<text " "${ARTIFACT_PATH}"

desc_line="$(rg -n --fixed-strings -e "commands=" "${ARTIFACT_PATH}" | head -n 1 || true)"
[[ -n "${desc_line}" ]] || fail "artifact metadata line missing command counts"

if [[ "${desc_line}" =~ commands=([0-9]+)[[:space:]]visible=([0-9]+) ]]; then
    command_count="${BASH_REMATCH[1]}"
    visible_count="${BASH_REMATCH[2]}"
else
    fail "artifact metadata line did not expose command and visible counts"
fi

if [[ "${command_count}" -lt 50 ]]; then
    fail "expected at least 50 render commands, got ${command_count}"
fi
if [[ "${visible_count}" -lt 25 ]]; then
    fail "expected at least 25 visible render commands, got ${visible_count}"
fi

echo "visual artifact contract checks passed"
