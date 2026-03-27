#include "mem_console_state.h"

#include <SDL2/SDL.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

const char *k_mem_console_default_db_path =
    "mem_console/data/default.sqlite";

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
        written = snprintf(out_path, out_cap, "%s/.local/share/mem_console", home_path);
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
