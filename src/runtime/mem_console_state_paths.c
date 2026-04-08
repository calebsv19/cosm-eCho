#include "mem_console_state.h"

#include <SDL2/SDL.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

const char *k_mem_console_default_db_path =
    "mem_console/data/default.sqlite";

static int path_is_absolute(const char *path) {
    return path && path[0] == '/';
}

static int path_is_inside_prefix(const char *path, const char *prefix) {
    size_t prefix_len = 0u;

    if (!path || !path[0] || !prefix || !prefix[0]) {
        return 0;
    }
    prefix_len = strlen(prefix);
    if (strncmp(path, prefix, prefix_len) != 0) {
        return 0;
    }
    if (path[prefix_len] == '\0' || path[prefix_len] == '/') {
        return 1;
    }
    return 0;
}

int mem_console_path_is_directory(const char *path) {
    struct stat st;

    if (!path || !path[0]) {
        return 0;
    }
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

int mem_console_path_exists(const char *path) {
    struct stat st;

    if (!path || !path[0]) {
        return 0;
    }
    return stat(path, &st) == 0 ? 1 : 0;
}

int mem_console_path_parent(const char *path, char *out_path, size_t out_cap) {
    size_t len = 0u;
    size_t parent_len = 0u;
    const char *slash = 0;

    if (!path || !path[0] || !out_path || out_cap == 0u) {
        return 0;
    }
    out_path[0] = '\0';
    len = strlen(path);
    if (len >= out_cap) {
        return 0;
    }
    if (snprintf(out_path, out_cap, "%s", path) <= 0 || strlen(out_path) >= out_cap) {
        out_path[0] = '\0';
        return 0;
    }
    slash = strrchr(out_path, '/');
    if (!slash) {
        return 0;
    }
    parent_len = (size_t)(slash - out_path);
    while (parent_len > 0u && out_path[parent_len - 1u] == '/') {
        parent_len -= 1u;
    }
    if (parent_len == 0u) {
        if (out_cap < 2u) {
            out_path[0] = '\0';
            return 0;
        }
        out_path[0] = '/';
        out_path[1] = '\0';
        return 1;
    }
    out_path[parent_len] = '\0';
    return 1;
}

int mem_console_ensure_directory(const char *path) {
    if (!path || !path[0]) {
        return 0;
    }
    if (mem_console_path_is_directory(path)) {
        return 1;
    }
    if (mkdir(path, 0755) == 0) {
        return 1;
    }
    return errno == EEXIST && mem_console_path_is_directory(path);
}

int mem_console_ensure_parent_directory(const char *path) {
    char buffer[1024];
    char *slash = 0;

    if (!path || !path[0]) {
        return 0;
    }

    if (strlen(path) >= sizeof(buffer)) {
        return 0;
    }

    (void)snprintf(buffer, sizeof(buffer), "%s", path);
    slash = strrchr(buffer, '/');
    if (!slash) {
        return 1;
    }

    while (slash > buffer && *slash == '/') {
        *slash = '\0';
        slash -= 1;
    }
    if (buffer[0] == '\0') {
        return 1;
    }

    {
        char partial[1024];
        size_t index = 0u;
        size_t length = strlen(buffer);

        memset(partial, 0, sizeof(partial));
        if (buffer[0] == '/') {
            partial[0] = '/';
            index = 1u;
        }

        while (index < length) {
            size_t next_index = index;
            size_t partial_len;

            while (buffer[next_index] != '\0' && buffer[next_index] != '/') {
                next_index += 1u;
            }

            partial_len = strlen(partial);
            if (partial_len > 0u && partial[partial_len - 1u] != '/') {
                if (partial_len + 1u >= sizeof(partial)) {
                    return 0;
                }
                partial[partial_len++] = '/';
                partial[partial_len] = '\0';
            }

            if (partial_len + (next_index - index) >= sizeof(partial)) {
                return 0;
            }
            memcpy(partial + partial_len, buffer + index, next_index - index);
            partial[partial_len + (next_index - index)] = '\0';

            if (!mem_console_ensure_directory(partial)) {
                return 0;
            }

            if (buffer[next_index] == '\0') {
                break;
            }
            index = next_index + 1u;
            while (buffer[index] == '/') {
                index += 1u;
            }
        }
    }

    return 1;
}

int mem_console_resolve_app_data_dir(char *out_path, size_t out_cap) {
    const char *home_path = 0;
    char *base_path = 0;
    int written = 0;

    if (!out_path || out_cap == 0u) {
        return 0;
    }

    out_path[0] = '\0';
    home_path = getenv("HOME");
    if (home_path && home_path[0]) {
#if defined(__APPLE__)
        written = snprintf(out_path, out_cap, "%s/Library/Application Support/MemConsole/runtime", home_path);
#else
        written = snprintf(out_path, out_cap, "%s/.local/share/mem_console", home_path);
#endif
        if (written > 0 && (size_t)written < out_cap) {
            return 1;
        }
        out_path[0] = '\0';
    }

    base_path = SDL_GetBasePath();
    if (!base_path) {
        written = snprintf(out_path, out_cap, "mem_console/data");
        return written > 0 && (size_t)written < out_cap;
    }

    written = snprintf(out_path, out_cap, "%s../data", base_path);
    SDL_free(base_path);
    return written > 0 && (size_t)written < out_cap;
}

int mem_console_path_is_mutable_root_safe(const char *path) {
    char *base_path = 0;
    int safe = 1;

    if (!path || !path[0]) {
        return 0;
    }
    if (!path_is_absolute(path)) {
        return 0;
    }
    base_path = SDL_GetBasePath();
    if (base_path && base_path[0]) {
        if (path_is_inside_prefix(path, base_path)) {
            safe = 0;
        }
    }
    if (base_path) {
        SDL_free(base_path);
    }
    return safe;
}

int resolve_default_db_path(char *out_path, size_t out_cap) {
    const char *env_db_path = 0;
    char data_dir[1024];
    int written = 0;

    if (!out_path || out_cap == 0u) {
        return 0;
    }

    out_path[0] = '\0';
    env_db_path = getenv("CODEWORK_MEMDB_PATH");
    if (env_db_path && env_db_path[0]) {
        written = snprintf(out_path, out_cap, "%s", env_db_path);
        return written > 0 && (size_t)written < out_cap;
    }

    if (mem_console_resolve_app_data_dir(data_dir, sizeof(data_dir))) {
        written = snprintf(out_path, out_cap, "%s/default.sqlite", data_dir);
        if (written > 0 && (size_t)written < out_cap) {
            return 1;
        }
    }

    written = snprintf(out_path, out_cap, "%s", k_mem_console_default_db_path);
    return written > 0 && (size_t)written < out_cap;
}

int mem_console_path_contract_normalize(const char *input_root_hint,
                                        const char *output_root_hint,
                                        const char *active_db_hint,
                                        char *out_input_root,
                                        size_t out_input_root_cap,
                                        char *out_output_root,
                                        size_t out_output_root_cap,
                                        char *out_active_db_path,
                                        size_t out_active_db_path_cap) {
    char resolved_output_root[1024];
    char resolved_input_root[1024];
    char resolved_active_db[1024];
    char default_db_path[1024];
    char active_parent[1024];
    int written = 0;

    if (!out_input_root || out_input_root_cap == 0u || !out_output_root || out_output_root_cap == 0u ||
        !out_active_db_path || out_active_db_path_cap == 0u) {
        return 0;
    }

    out_input_root[0] = '\0';
    out_output_root[0] = '\0';
    out_active_db_path[0] = '\0';
    resolved_output_root[0] = '\0';
    resolved_input_root[0] = '\0';
    resolved_active_db[0] = '\0';

    if (output_root_hint && output_root_hint[0] &&
        snprintf(resolved_output_root, sizeof(resolved_output_root), "%s", output_root_hint) > 0 &&
        strlen(resolved_output_root) < sizeof(resolved_output_root) &&
        mem_console_path_is_mutable_root_safe(resolved_output_root)) {
        /* Keep caller-provided output root. */
    } else if (!mem_console_resolve_app_data_dir(resolved_output_root, sizeof(resolved_output_root))) {
        if (!path_is_absolute(k_mem_console_default_db_path) ||
            !mem_console_path_parent(k_mem_console_default_db_path, resolved_output_root, sizeof(resolved_output_root))) {
            return 0;
        }
    }
    if (!mem_console_path_is_mutable_root_safe(resolved_output_root)) {
        return 0;
    }
    if (!mem_console_ensure_directory(resolved_output_root)) {
        return 0;
    }

    if (active_db_hint && active_db_hint[0] &&
        snprintf(resolved_active_db, sizeof(resolved_active_db), "%s", active_db_hint) > 0 &&
        strlen(resolved_active_db) < sizeof(resolved_active_db)) {
        /* Keep caller-provided active DB path. */
    } else if (resolve_default_db_path(default_db_path, sizeof(default_db_path))) {
        (void)snprintf(resolved_active_db, sizeof(resolved_active_db), "%s", default_db_path);
    }
    if (!resolved_active_db[0]) {
        written = snprintf(resolved_active_db,
                           sizeof(resolved_active_db),
                           "%s/data/default.sqlite",
                           resolved_output_root);
        if (written <= 0 || (size_t)written >= sizeof(resolved_active_db)) {
            return 0;
        }
    }
    if (!mem_console_ensure_parent_directory(resolved_active_db)) {
        return 0;
    }

    if (input_root_hint && input_root_hint[0] &&
        snprintf(resolved_input_root, sizeof(resolved_input_root), "%s", input_root_hint) > 0 &&
        strlen(resolved_input_root) < sizeof(resolved_input_root)) {
        /* Keep caller-provided input root. */
    } else if (mem_console_path_parent(resolved_active_db, active_parent, sizeof(active_parent))) {
        (void)snprintf(resolved_input_root, sizeof(resolved_input_root), "%s", active_parent);
    } else {
        (void)snprintf(resolved_input_root, sizeof(resolved_input_root), "%s", resolved_output_root);
    }
    if (!resolved_input_root[0]) {
        (void)snprintf(resolved_input_root, sizeof(resolved_input_root), "%s", resolved_output_root);
    }
    if (!mem_console_path_is_directory(resolved_input_root)) {
        if (!mem_console_ensure_directory(resolved_input_root)) {
            (void)snprintf(resolved_input_root, sizeof(resolved_input_root), "%s", resolved_output_root);
        }
    }

    if (snprintf(out_output_root, out_output_root_cap, "%s", resolved_output_root) <= 0 ||
        strlen(resolved_output_root) >= out_output_root_cap) {
        out_output_root[0] = '\0';
        return 0;
    }
    if (snprintf(out_input_root, out_input_root_cap, "%s", resolved_input_root) <= 0 ||
        strlen(resolved_input_root) >= out_input_root_cap) {
        out_input_root[0] = '\0';
        return 0;
    }
    if (snprintf(out_active_db_path, out_active_db_path_cap, "%s", resolved_active_db) <= 0 ||
        strlen(resolved_active_db) >= out_active_db_path_cap) {
        out_active_db_path[0] = '\0';
        return 0;
    }
    return 1;
}

void print_usage(const char *argv0) {
    fprintf(stderr, "usage: %s [--db <path>] [--kernel-bridge]\n", argv0);
}

const char *find_flag_value(int argc, char **argv, const char *flag) {
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            if ((i + 1) >= argc) {
                return 0;
            }
            return argv[i + 1];
        }
    }

    return 0;
}

int has_flag(int argc, char **argv, const char *flag) {
    int i;

    if (!flag) {
        return 0;
    }

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            return 1;
        }
    }

    return 0;
}

int has_unknown_flag(int argc, char **argv) {
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--db") == 0) {
            if ((i + 1) < argc) {
                i += 1;
                continue;
            }
            return 1;
        }
        if (strcmp(argv[i], "--kernel-bridge") == 0) {
            continue;
        }
        return 1;
    }

    return 0;
}
