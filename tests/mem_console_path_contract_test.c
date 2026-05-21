#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mem_console_state.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void ensure_dir(const char* path) {
    if (!path || !path[0]) return;
    if (mkdir(path, 0755) == 0) return;
    if (errno == EEXIST) return;
    perror("mkdir");
    assert(0 && "mkdir failed");
}

static void ensure_dir_recursive(const char* path) {
    char buf[PATH_MAX];
    size_t len;
    if (!path || !path[0]) return;
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    len = strlen(buf);
    if (len == 0) return;
    if (buf[len - 1] == '/') {
        buf[len - 1] = '\0';
    }
    for (char* p = buf + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            ensure_dir(buf);
            *p = '/';
        }
    }
    ensure_dir(buf);
}

static void test_env_db_override_wins(void) {
    char path[PATH_MAX];
    setenv("CODEWORK_MEMDB_PATH", "/tmp/mem_console_contract_env/env.sqlite", 1);
    assert(resolve_default_db_path(path, sizeof(path)));
    assert(strcmp(path, "/tmp/mem_console_contract_env/env.sqlite") == 0);
    unsetenv("CODEWORK_MEMDB_PATH");
}

static void test_normalize_derives_input_root_from_active_db_parent(void) {
    char tmp_template[] = "/tmp/mem_console_path_contract.XXXXXX";
    char* root = mkdtemp(tmp_template);
    char output_root[PATH_MAX];
    char active_db[PATH_MAX];
    char input_root_out[PATH_MAX];
    char output_root_out[PATH_MAX];
    char active_db_out[PATH_MAX];

    assert(root && "mkdtemp failed");
    snprintf(output_root, sizeof(output_root), "%s/runtime", root);
    ensure_dir_recursive(output_root);
    snprintf(active_db, sizeof(active_db), "%s/dbs/current.sqlite", output_root);

    assert(mem_console_path_contract_normalize(NULL,
                                               output_root,
                                               active_db,
                                               input_root_out,
                                               sizeof(input_root_out),
                                               output_root_out,
                                               sizeof(output_root_out),
                                               active_db_out,
                                               sizeof(active_db_out)));
    assert(strcmp(output_root_out, output_root) == 0);
    assert(strcmp(active_db_out, active_db) == 0);

    {
        char expected_input_root[PATH_MAX];
        snprintf(expected_input_root, sizeof(expected_input_root), "%s/dbs", output_root);
        assert(strcmp(input_root_out, expected_input_root) == 0);
    }
}

static void test_normalize_preserves_explicit_input_root_hint(void) {
    char tmp_template[] = "/tmp/mem_console_path_contract_hint.XXXXXX";
    char* root = mkdtemp(tmp_template);
    char output_root[PATH_MAX];
    char input_root[PATH_MAX];
    char active_db[PATH_MAX];
    char input_root_out[PATH_MAX];
    char output_root_out[PATH_MAX];
    char active_db_out[PATH_MAX];

    assert(root && "mkdtemp failed");
    snprintf(output_root, sizeof(output_root), "%s/runtime", root);
    snprintf(input_root, sizeof(input_root), "%s/incoming", root);
    ensure_dir_recursive(output_root);
    ensure_dir_recursive(input_root);
    snprintf(active_db, sizeof(active_db), "%s/current.sqlite", output_root);

    assert(mem_console_path_contract_normalize(input_root,
                                               output_root,
                                               active_db,
                                               input_root_out,
                                               sizeof(input_root_out),
                                               output_root_out,
                                               sizeof(output_root_out),
                                               active_db_out,
                                               sizeof(active_db_out)));
    assert(strcmp(input_root_out, input_root) == 0);
    assert(strcmp(output_root_out, output_root) == 0);
    assert(strcmp(active_db_out, active_db) == 0);
}

int main(void) {
    test_env_db_override_wins();
    test_normalize_derives_input_root_from_active_db_parent();
    test_normalize_preserves_explicit_input_root_hint();
    puts("mem_console_path_contract_test: success");
    return 0;
}
