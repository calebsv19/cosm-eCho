#!/usr/bin/env bash

mem_console_demo_resolve_path() {
    local path="$1"
    local dir
    local base

    dir="$(dirname "${path}")"
    base="$(basename "${path}")"
    mkdir -p "${dir}"
    dir="$(cd "${dir}" && pwd -P)"
    printf '%s/%s\n' "${dir}" "${base}"
}

mem_console_demo_assert_safe_db_path() {
    local root_dir="$1"
    local db_path="$2"
    local resolved_db_path
    local demo_root
    local build_root

    resolved_db_path="$(mem_console_demo_resolve_path "${db_path}")"
    demo_root="$(cd "${root_dir}/mem_console/demo" && pwd -P)"
    build_root="$(cd "${root_dir}/mem_console/build" && pwd -P)"

    case "${resolved_db_path}" in
        "${demo_root}"/*.sqlite|"${build_root}"/*.sqlite)
            printf '%s\n' "${resolved_db_path}"
            return 0
            ;;
    esac

    if [[ "${MEM_CONSOLE_ALLOW_NON_DEMO_DB:-0}" == "1" ]]; then
        printf '%s\n' "${resolved_db_path}"
        return 0
    fi

    echo "Refusing to write non-demo DB: ${resolved_db_path}" >&2
    echo "Allowed roots: ${demo_root}, ${build_root}" >&2
    echo "Set MEM_CONSOLE_ALLOW_NON_DEMO_DB=1 only for an intentional non-demo target." >&2
    return 1
}
