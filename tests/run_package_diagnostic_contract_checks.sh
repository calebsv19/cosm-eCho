#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
    echo "package diagnostic contract check failed: $1" >&2
    exit 1
}

check_contains() {
    local pattern="$1"
    local file="$2"
    if ! rg -n --fixed-strings "${pattern}" "${file}" >/dev/null; then
        fail "missing pattern in ${file}: ${pattern}"
    fi
}

LAUNCHER="${ROOT_DIR}/tools/packaging/macos/mem-console-launcher"
PACKAGE_MK="${ROOT_DIR}/make/package-macos.mk"
RELEASE_MK="${ROOT_DIR}/make/release.mk"

check_contains "launcher_print_config() {" "${LAUNCHER}"
check_contains "launcher_fail() {" "${LAUNCHER}"
check_contains "require_exec() {" "${LAUNCHER}"
check_contains "require_file() {" "${LAUNCHER}"
check_contains "require_dir() {" "${LAUNCHER}"

check_contains 'SCRIPT_DIR=$SCRIPT_DIR' "${LAUNCHER}"
check_contains 'APP_BINARY=$SCRIPT_DIR/mem-console-bin' "${LAUNCHER}"
check_contains 'LOG_FILE=$LOG_FILE' "${LAUNCHER}"
check_contains 'APP_BUNDLE_DB=$APP_BUNDLE_DB' "${LAUNCHER}"
check_contains 'diagnostic: log_file=$LOG_FILE app_contents=$APP_CONTENTS_DIR resources=$RES_DIR runtime=$MEM_CONSOLE_RUNTIME_DIR db=$CODEWORK_MEMDB_PATH shader_root=$VK_RENDERER_SHADER_ROOT' "${LAUNCHER}"
check_contains 'self-test failed: $message path=$path' "${LAUNCHER}"

check_contains 'require_exec "$SCRIPT_DIR/mem-console-bin"' "${LAUNCHER}"
check_contains 'require_file "$APP_CONTENTS_DIR/Info.plist"' "${LAUNCHER}"
check_contains 'require_file "$APP_BUNDLE_DB"' "${LAUNCHER}"
check_contains 'require_file "$MOLTENVK_DYLIB"' "${LAUNCHER}"
check_contains 'require_file "$RUNTIME_DIR/vk_renderer/shaders/fill.frag.spv"' "${LAUNCHER}"
check_contains 'require_file "$RUNTIME_DIR/shaders/fill.frag.spv"' "${LAUNCHER}"

check_contains "package-desktop self-test failed; launcher config follows." "${PACKAGE_MK}"
check_contains '"$(PACKAGE_MACOS_DIR)/mem-console-launcher" --print-config' "${PACKAGE_MK}"
check_contains 'cp "data/default.sqlite" "$(PACKAGE_RESOURCES_DIR)/data/default.sqlite"' "${PACKAGE_MK}"
check_contains 'Unexpected packaged data sidecar' "${PACKAGE_MK}"

check_contains '"$(PACKAGE_MACOS_DIR)/mem-console-launcher" --print-config > "$(RELEASE_DIR)/print_config.txt"' "${RELEASE_MK}"
check_contains "Missing MEM_CONSOLE_RUNTIME_DIR in launcher config" "${RELEASE_MK}"
check_contains "Missing VK_ICD_FILENAMES in launcher config" "${RELEASE_MK}"
check_contains 'find "$(PACKAGE_APP_DIR)" -print > "$(RELEASE_DIR)/bundle_manifest.txt"' "${RELEASE_MK}"
check_contains "Found private/generated path in release bundle" "${RELEASE_MK}"
check_contains "Found packaged UI prefs sidecar in release bundle" "${RELEASE_MK}"
check_contains "Unexpected release data sidecar" "${RELEASE_MK}"

if rg -n --fixed-strings "cp -R data" "${PACKAGE_MK}" >/dev/null; then
    fail "package rule must not copy the whole ignored data directory"
fi

echo "package diagnostic contract checks ok: launcher/package R3-S5 diagnostics present"
