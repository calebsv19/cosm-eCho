#!/usr/bin/env bash
set -euo pipefail

WORK_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MEM_CONSOLE_DIR="${WORK_ROOT}/mem_console"
CAPTURE_BIN="${WORK_ROOT}/desktop_capture/build/bin/desktop_capture"
MEM_CONSOLE_BIN="${MEM_CONSOLE_DIR}/build/targets/macOS-arm64/toolchains/clang/bin/mem_console"
STAMP="$(date +%Y%m%d_%H%M%S)"
PLAN_ONLY=0
OUT_ROOT="${WORK_ROOT}/_private_workspace_artifacts/desktop_capture/mem_console_s6_visual_fixture_${STAMP}"
DB_PATH="${OUT_ROOT}/visual_graph_fixture.sqlite"
MANIFEST_PATH="${OUT_ROOT}/visual_graph_fixture.env"
APP_PID=""

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --plan-only)
            PLAN_ONLY=1
            shift
            ;;
        --out-root)
            if [[ "$#" -lt 2 ]]; then
                echo "--out-root requires a path" >&2
                exit 1
            fi
            OUT_ROOT="$2"
            shift 2
            ;;
        *)
            OUT_ROOT="$1"
            shift
            ;;
    esac
done

DB_PATH="${OUT_ROOT}/visual_graph_fixture.sqlite"
MANIFEST_PATH="${OUT_ROOT}/visual_graph_fixture.env"

cleanup_app() {
    if [[ -n "${APP_PID}" ]]; then
        kill "${APP_PID}" >/dev/null 2>&1 || true
        wait "${APP_PID}" >/dev/null 2>&1 || true
        APP_PID=""
    fi
}

trap cleanup_app EXIT

mkdir -p "${OUT_ROOT}"

write_capture_plan() {
    cat > "${OUT_ROOT}/capture_plan.env" <<EOF
OUT_ROOT=${OUT_ROOT}
DB_PATH=${DB_PATH}
FIXTURE_MANIFEST=${MANIFEST_PATH}
PLAN_ONLY=${PLAN_ONLY}
MODES=focus,pods,web
EOF

    cat > "${OUT_ROOT}/capture_manifest.tsv" <<EOF
mode	screenshot	stdout	stderr	capture_json
focus	${OUT_ROOT}/focus.png	${OUT_ROOT}/focus.stdout.log	${OUT_ROOT}/focus.stderr.log	${OUT_ROOT}/focus.capture.json
pods	${OUT_ROOT}/pods.png	${OUT_ROOT}/pods.stdout.log	${OUT_ROOT}/pods.stderr.log	${OUT_ROOT}/pods.capture.json
web	${OUT_ROOT}/web.png	${OUT_ROOT}/web.stdout.log	${OUT_ROOT}/web.stderr.log	${OUT_ROOT}/web.capture.json
EOF
}

if [[ "${PLAN_ONLY}" -eq 1 ]]; then
    write_capture_plan
    echo "Visual fixture capture plan ready: ${OUT_ROOT}"
    exit 0
fi

if [[ ! -x "${MEM_CONSOLE_BIN}" ]]; then
    echo "mem_console binary not found: ${MEM_CONSOLE_BIN}" >&2
    echo "build it with: make -C ${MEM_CONSOLE_DIR} all" >&2
    exit 1
fi
if [[ ! -x "${CAPTURE_BIN}" ]]; then
    echo "desktop_capture binary not found: ${CAPTURE_BIN}" >&2
    echo "build it with: make -C ${WORK_ROOT}/desktop_capture" >&2
    exit 1
fi

"${MEM_CONSOLE_DIR}/demo/reset_visual_graph_fixture.sh" "${DB_PATH}" "${MANIFEST_PATH}" > "${OUT_ROOT}/fixture_seed.log"

# shellcheck disable=SC1090
source "${MANIFEST_PATH}"

write_capture_plan

for mode in focus pods web; do
    cleanup_app
    (
        cd "${WORK_ROOT}"
        "${MEM_CONSOLE_BIN}" \
            --db "${DB_PATH}" \
            --visual-review \
            --visual-review-mode "${mode}" \
            --visual-review-selected-id "${ROOT_ID}"
    ) > "${OUT_ROOT}/${mode}.stdout.log" 2> "${OUT_ROOT}/${mode}.stderr.log" &
    APP_PID="$!"
    sleep 4
    "${CAPTURE_BIN}" screenshot --title mem_console --out "${OUT_ROOT}/${mode}.png" > "${OUT_ROOT}/${mode}.capture.json"
done

cleanup_app
echo "Visual fixture captures ready: ${OUT_ROOT}"
