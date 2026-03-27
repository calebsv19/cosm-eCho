#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_PATH="${ROOT_DIR}/build/mem_console"

if [[ ! -x "${BIN_PATH}" ]]; then
    echo "headless smoke failed: missing binary at ${BIN_PATH}" >&2
    echo "build with: make -C mem_console" >&2
    exit 1
fi

set +e
SMOKE_OUTPUT="$("${BIN_PATH}" --invalid-smoke-flag 2>&1)"
SMOKE_STATUS=$?
set -e

if [[ ${SMOKE_STATUS} -eq 0 ]]; then
    echo "headless smoke failed: expected non-zero exit for invalid flag path" >&2
    exit 1
fi

if [[ "${SMOKE_OUTPUT}" != *"usage:"* ]]; then
    echo "headless smoke failed: expected usage output for invalid flag path" >&2
    printf '%s\n' "${SMOKE_OUTPUT}" >&2
    exit 1
fi

echo "headless smoke ok: invalid-flag guard path is active"
