#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MEM_CLI="${ROOT_DIR}/shared/core/core_memdb/build/mem_cli"
source "${ROOT_DIR}/mem_console/demo/demo_db_safety.sh"

DEFAULT_DB_PATH="${ROOT_DIR}/mem_console/demo/visual_graph_fixture.sqlite"
DB_PATH="${1:-${DEFAULT_DB_PATH}}"
DB_PATH="$(mem_console_demo_assert_safe_db_path "${ROOT_DIR}" "${DB_PATH}")"
MANIFEST_PATH="${2:-${DB_PATH}.manifest}"
DB_DIR="$(dirname "${DB_PATH}")"

mkdir -p "${DB_DIR}"
mkdir -p "$(dirname "${MANIFEST_PATH}")"

if [[ ! -x "${MEM_CLI}" ]]; then
    echo "mem_cli not found at ${MEM_CLI}" >&2
    echo "build it with: make -C shared/core/core_memdb all" >&2
    exit 1
fi

rm -f "${DB_PATH}" "${MANIFEST_PATH}"
"${MEM_CLI}" list --db "${DB_PATH}" >/dev/null

add_memory() {
    local stable_id="$1"
    local title="$2"
    local body="$3"
    local project="$4"
    local kind="$5"
    local output
    local id
    output="$("${MEM_CLI}" add \
        --db "${DB_PATH}" \
        --stable-id "${stable_id}" \
        --title "${title}" \
        --body "${body}" \
        --workspace codework \
        --project "${project}" \
        --kind "${kind}")"
    id="$(printf '%s\n' "${output}" | sed -n 's/.*id=\([0-9][0-9]*\).*/\1/p' | head -n 1)"
    if [[ -z "${id}" ]]; then
        echo "failed to parse id for ${stable_id}" >&2
        exit 1
    fi
    printf '%s' "${id}"
}

add_link() {
    local from_id="$1"
    local to_id="$2"
    local kind="$3"
    "${MEM_CLI}" link-add --db "${DB_PATH}" --from "${from_id}" --to "${to_id}" --kind "${kind}" >/dev/null
}

echo "Seeding deterministic graph visual fixture..."
ID_ROOT="$(add_memory "visual-fixture-root" "Graph Fixture Root" "Selected root for FOCUS mode: dense local links, mixed link kinds, and cross-project bridges." "memory_console" "summary")"
ID_FOCUS_A="$(add_memory "visual-fixture-focus-runtime" "Runtime refresh contract" "Primary hop-1 runtime neighbor for selected-root FOCUS readability." "memory_console" "runtime")"
ID_FOCUS_B="$(add_memory "visual-fixture-focus-policy" "Graph rendering policy" "Primary hop-1 policy neighbor with hierarchy-style support links." "memory_console" "policy")"
ID_FOCUS_C="$(add_memory "visual-fixture-focus-issue" "Focus clutter issue" "Dense non-primary edge candidate that should not dominate FOCUS mode." "memory_console" "issue")"
ID_FOCUS_D="$(add_memory "visual-fixture-focus-plan" "Visualizer S6 plan" "Plan node for fixture-driven visual acceptance." "memory_console" "plan")"
ID_FOCUS_E="$(add_memory "visual-fixture-focus-decision" "Relationship edit decision" "Decision node connected to the root and dense memory_console cluster." "memory_console" "decision")"
ID_SHARED_A="$(add_memory "visual-fixture-shared-kit" "Shared graph kit boundary" "Shared graph kit context bridged from the memory console graph." "shared" "scope")"
ID_SHARED_B="$(add_memory "visual-fixture-shared-render" "Shared render label contract" "Shared renderer label and edge clarity dependency." "shared" "policy")"
ID_RAY_A="$(add_memory "visual-fixture-ray-bridge" "Ray tracing bridge note" "Cross-project bridge node that should read in WEB mode." "ray_tracing" "summary")"
ID_RAY_B="$(add_memory "visual-fixture-ray-runtime" "Ray tracing runtime follow-up" "Secondary ray tracing cluster member for WEB component readability." "ray_tracing" "runtime")"
ID_FISICS_A="$(add_memory "visual-fixture-fisics-compiler" "fisiCs compiler blocker" "Separate project cluster with a bridge back through shared work." "fisics" "issue")"
ID_FISICS_B="$(add_memory "visual-fixture-fisics-plan" "fisiCs validation plan" "Companion node for the fisics cluster." "fisics" "plan")"
ID_BEHAVIOR_A="$(add_memory "visual-fixture-behavior-island" "Behavior policy island" "Disconnected component used to verify WEB island placement." "behavior_sim" "policy")"
ID_BEHAVIOR_B="$(add_memory "visual-fixture-behavior-note" "Behavior note child" "Small isolated component child." "behavior_sim" "summary")"
ID_DAW_A="$(add_memory "visual-fixture-daw-island" "DAW archive island" "Second small disconnected component to test non-root islands." "daw" "summary")"
ID_DAW_B="$(add_memory "visual-fixture-daw-child" "DAW child note" "Child of the DAW island." "daw" "runtime")"

"${MEM_CLI}" canonical --db "${DB_PATH}" --id "${ID_ROOT}" --on >/dev/null
"${MEM_CLI}" pin --db "${DB_PATH}" --id "${ID_ROOT}" --on >/dev/null
"${MEM_CLI}" pin --db "${DB_PATH}" --id "${ID_FOCUS_D}" --on >/dev/null

add_link "${ID_ROOT}" "${ID_FOCUS_A}" "supports"
add_link "${ID_ROOT}" "${ID_FOCUS_B}" "depends_on"
add_link "${ID_ROOT}" "${ID_FOCUS_C}" "related"
add_link "${ID_ROOT}" "${ID_FOCUS_D}" "summarizes"
add_link "${ID_ROOT}" "${ID_FOCUS_E}" "implements"
add_link "${ID_FOCUS_A}" "${ID_FOCUS_B}" "supports"
add_link "${ID_FOCUS_A}" "${ID_FOCUS_C}" "related"
add_link "${ID_FOCUS_B}" "${ID_FOCUS_D}" "depends_on"
add_link "${ID_FOCUS_D}" "${ID_FOCUS_E}" "supports"
add_link "${ID_FOCUS_C}" "${ID_FOCUS_E}" "references"
add_link "${ID_FOCUS_B}" "${ID_SHARED_A}" "depends_on"
add_link "${ID_SHARED_A}" "${ID_SHARED_B}" "supports"
add_link "${ID_SHARED_B}" "${ID_RAY_A}" "implements"
add_link "${ID_RAY_A}" "${ID_RAY_B}" "supports"
add_link "${ID_SHARED_A}" "${ID_FISICS_A}" "related"
add_link "${ID_FISICS_A}" "${ID_FISICS_B}" "summarizes"
add_link "${ID_FISICS_B}" "${ID_SHARED_B}" "references"
add_link "${ID_BEHAVIOR_A}" "${ID_BEHAVIOR_B}" "supports"
add_link "${ID_DAW_A}" "${ID_DAW_B}" "depends_on"

cat > "${MANIFEST_PATH}" <<EOF
DB_PATH=${DB_PATH}
ROOT_ID=${ID_ROOT}
FOCUS_MODE=focus
PODS_MODE=pods
WEB_MODE=web
NODE_COUNT=16
EDGE_COUNT=19
EOF

"${MEM_CLI}" health --db "${DB_PATH}" --format json
echo "Visual graph fixture ready: ${DB_PATH}"
echo "Manifest: ${MANIFEST_PATH}"
