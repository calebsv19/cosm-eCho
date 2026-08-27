#!/usr/bin/env bash
set -euo pipefail

PROGRAM_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MEM_CLI="${PROGRAM_ROOT}/third_party/codework_shared/core/core_memdb/build/mem_cli"
source "${PROGRAM_ROOT}/demo/demo_db_safety.sh"

DEFAULT_DB_PATH="${PROGRAM_ROOT}/demo/demo_mem_console.sqlite"
DB_PATH="${1:-${DEFAULT_DB_PATH}}"
DB_PATH="$(mem_console_demo_assert_safe_db_path "${PROGRAM_ROOT}" "${DB_PATH}")"
DB_DIR="$(dirname "${DB_PATH}")"
COUNT="${2:-160}"

mkdir -p "${DB_DIR}"

if [[ ! -x "${MEM_CLI}" ]]; then
    echo "mem_cli not found at ${MEM_CLI}" >&2
    echo "build it with: make -C shared/core/core_memdb all" >&2
    exit 1
fi

if ! [[ "${COUNT}" =~ ^[0-9]+$ ]] || [[ "${COUNT}" -le 0 ]]; then
    echo "count must be a positive integer (got '${COUNT}')" >&2
    exit 1
fi

echo "Seeding ${COUNT} memories into ${DB_PATH}"

for ((i = 1; i <= COUNT; ++i)); do
    title="Large List Seed ${i}"
    body="Seed row ${i} for mem_console list scrolling validation."
    stable_id="large-list-seed-${i}"
    "${MEM_CLI}" add --db "${DB_PATH}" --title "${title}" --body "${body}" --stable-id "${stable_id}" >/dev/null
done

echo "Seed complete."
echo "Quick count:"
"${MEM_CLI}" list --db "${DB_PATH}" | wc -l | awk '{print "rows listed:", $1}'
