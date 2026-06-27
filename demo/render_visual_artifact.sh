#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MEM_CONSOLE_DIR="${ROOT_DIR}/mem_console"
TARGET_TRIPLE="${TARGET_TRIPLE:-macOS-arm64}"
TOOLCHAIN="${BUILD_TOOLCHAIN:-clang}"
BIN_PATH="${MEM_CONSOLE_DIR}/build/targets/${TARGET_TRIPLE}/toolchains/${TOOLCHAIN}/bin/mem_console"
OUT_ROOT="${MEM_CONSOLE_DIR}/visual_artifacts"
RUNTIME_ROOT="${TMPDIR:-/private/tmp}/mem_console_visual_artifact"
DB_PATH="${RUNTIME_ROOT}/visual_artifact_fixture.sqlite"
MANIFEST_PATH="${RUNTIME_ROOT}/visual_artifact_fixture.env"
HOME_ROOT="${RUNTIME_ROOT}/home"
MODE="${MEM_CONSOLE_VISUAL_ARTIFACT_MODE:-web}"

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --out-root)
            if [[ "$#" -lt 2 ]]; then
                echo "--out-root requires a path" >&2
                exit 1
            fi
            OUT_ROOT="$2"
            shift 2
            ;;
        --mode)
            if [[ "$#" -lt 2 ]]; then
                echo "--mode requires focus, pods, or web" >&2
                exit 1
            fi
            MODE="$2"
            shift 2
            ;;
        *)
            echo "unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

case "${MODE}" in
    focus|pods|web) ;;
    *)
        echo "invalid visual artifact mode: ${MODE}" >&2
        exit 1
        ;;
esac

if [[ ! -x "${BIN_PATH}" ]]; then
    echo "mem_console binary not found: ${BIN_PATH}" >&2
    echo "build it with: make -C ${MEM_CONSOLE_DIR} all" >&2
    exit 1
fi

mkdir -p "${OUT_ROOT}" "${HOME_ROOT}/Library/Application Support/MemConsole" "${RUNTIME_ROOT}"
MEM_CONSOLE_ALLOW_NON_DEMO_DB=1 \
    "${MEM_CONSOLE_DIR}/demo/reset_visual_graph_fixture.sh" "${DB_PATH}" "${MANIFEST_PATH}" >/dev/null

# shellcheck disable=SC1090
source "${MANIFEST_PATH}"

ARTIFACT_PATH="${OUT_ROOT}/mem_console_first_frame_${MODE}.svg"
rm -f "${ARTIFACT_PATH}"

(
    cd "${ROOT_DIR}"
    HOME="${HOME_ROOT}" "${BIN_PATH}" \
        --db "${DB_PATH}" \
        --visual-artifact "${ARTIFACT_PATH}" \
        --visual-review \
        --visual-review-mode "${MODE}" \
        --visual-review-selected-id "${ROOT_ID}"
)

if [[ ! -s "${ARTIFACT_PATH}" ]]; then
    echo "visual-artifact failed: missing or empty artifact ${ARTIFACT_PATH}" >&2
    exit 1
fi

echo "visual-artifact ready: ${ARTIFACT_PATH}"
