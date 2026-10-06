#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem_console_prefs.h"
#include "runtime/mem_console_startup_paths.h"

static void path_join(char *out, size_t cap, const char *root, const char *suffix) {
    int n = snprintf(out, cap, "%s/%s", root, suffix);
    assert(n > 0 && (size_t)n < cap);
}

static void save_prefs(const char *path, const char *db, const char *output) {
    assert(mem_console_ensure_parent_directory(path));
    assert(mem_console_app_prefs_save(path, db, "", output, db).code == CORE_OK);
}

static unsigned long file_hash(const char *path) {
    FILE *f = fopen(path, "rb");
    unsigned long hash = 5381;
    int c;
    assert(f);
    while ((c = fgetc(f)) != EOF) hash = hash * 33 + (unsigned char)c;
    assert(!ferror(f));
    fclose(f);
    return hash;
}

int main(int argc, char **argv) {
    char canonical[1024], canonical_prefs[1200], legacy[1200];
    char isolated[1024], isolated_prefs[1200], chosen[1024], cli_db[1024], seed[1024];
    char resolved[1024], namespace_root[1024];
    MemConsoleStartupPaths paths;
    assert(argc == 2);
    /* HOME is isolated only inside this test process; all bytes are synthetic. */
    assert(setenv("HOME", argv[1], 1) == 0);
    unsetenv("CODEWORK_WINDOW_LIFECYCLE_PROOF");
    unsetenv("MEM_CONSOLE_RUNTIME_DIR");
    unsetenv("MEM_CONSOLE_RUNTIME_NAMESPACE");
    unsetenv("MEM_CONSOLE_PACKAGE_PROFILE");
    unsetenv("CODEWORK_MEMDB_PATH");
    unsetenv("MEM_CONSOLE_LAUNCHER_DEFAULT_DB");
    assert(mem_console_resolve_app_data_dir(canonical, sizeof(canonical)));
    path_join(canonical_prefs, sizeof(canonical_prefs), canonical, "mem_console.app.pack");
    path_join(legacy, sizeof(legacy), argv[1], ".local/share/mem_console/mem_console.app.pack");
    path_join(isolated, sizeof(isolated), argv[1], "isolated-runtime");
    path_join(isolated_prefs, sizeof(isolated_prefs), isolated, "mem_console.app.pack");
    path_join(chosen, sizeof(chosen), argv[1], "selected/existing.sqlite");
    path_join(cli_db, sizeof(cli_db), argv[1], "selected/explicit.sqlite");
    path_join(seed, sizeof(seed), isolated, "data/default.sqlite");
    save_prefs(canonical_prefs, chosen, canonical);
    unsigned long canonical_hash = file_hash(canonical_prefs);
    assert(mem_console_startup_paths_resolve(NULL, &paths).code == CORE_OK);
    assert(strcmp(paths.db_path, chosen) == 0);
    assert(file_hash(canonical_prefs) == canonical_hash);

    save_prefs(legacy, chosen, canonical);
    unsigned long legacy_hash = file_hash(legacy);
    assert(setenv("MEM_CONSOLE_PACKAGE_PROFILE", "main-edit", 1) == 0);
    assert(setenv("MEM_CONSOLE_RUNTIME_NAMESPACE", "MemConsole-Main-Edit", 1) == 0);
    assert(setenv("MEM_CONSOLE_RUNTIME_DIR", isolated, 1) == 0);
    assert(setenv("CODEWORK_MEMDB_PATH", seed, 1) == 0);
    assert(setenv("MEM_CONSOLE_LAUNCHER_DEFAULT_DB", seed, 1) == 0);
    assert(mem_console_startup_paths_resolve(NULL, &paths).code == CORE_OK);
    assert(strcmp(paths.db_path, seed) == 0 && strcmp(paths.output_root, isolated) == 0);
    assert(mem_console_startup_paths_resolve(cli_db, &paths).code == CORE_OK);
    assert(strcmp(paths.db_path, cli_db) == 0);
    save_prefs(isolated_prefs, chosen, canonical);
    unsigned long isolated_hash = file_hash(isolated_prefs);
    assert(mem_console_startup_paths_resolve(NULL, &paths).code == CORE_OK);
    assert(strcmp(paths.db_path, chosen) == 0 && strcmp(paths.output_root, isolated) == 0);
    setenv("CODEWORK_MEMDB_PATH", cli_db, 1);
    assert(mem_console_startup_paths_resolve(NULL, &paths).code == CORE_OK);
    assert(strcmp(paths.db_path, cli_db) == 0 && strcmp(paths.output_root, isolated) == 0);
    setenv("CODEWORK_MEMDB_PATH", seed, 1);
    assert(file_hash(canonical_prefs) == canonical_hash && file_hash(legacy) == legacy_hash);
    assert(file_hash(isolated_prefs) == isolated_hash);
    assert(access(chosen, F_OK) != 0 && access(seed, F_OK) != 0);

    unsetenv("MEM_CONSOLE_RUNTIME_DIR");
    unsetenv("CODEWORK_MEMDB_PATH");
    assert(mem_console_resolve_app_data_dir(namespace_root, sizeof(namespace_root)));
    assert(strstr(namespace_root, "MemConsole-Main-Edit"));
    assert(mem_console_startup_paths_resolve(NULL, &paths).code == CORE_OK);
    assert(strncmp(paths.db_path, namespace_root, strlen(namespace_root)) == 0);
    assert(strcmp(paths.output_root, namespace_root) == 0);
    const char *invalid[] = {"relative-root", "/tmp/../escape", "/tmp/bad\nroot"};
    for (size_t i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        setenv("MEM_CONSOLE_RUNTIME_DIR", invalid[i], 1);
        assert(!mem_console_resolve_app_data_dir(resolved, sizeof(resolved)));
        assert(mem_console_startup_paths_resolve(NULL, &paths).code != CORE_OK);
    }
    /* Standard default keeps its legacy preference compatibility. */
    unsetenv("MEM_CONSOLE_RUNTIME_DIR");
    unsetenv("MEM_CONSOLE_RUNTIME_NAMESPACE");
    unsetenv("MEM_CONSOLE_PACKAGE_PROFILE");
    assert(unlink(canonical_prefs) == 0);
    assert(mem_console_startup_paths_resolve(NULL, &paths).code == CORE_OK);
    assert(strcmp(paths.db_path, chosen) == 0 && file_hash(legacy) == legacy_hash);
    puts("runtime isolation: canonical/legacy prefs untouched, explicit and profile-selected DB paths preserved, invalid roots rejected");
    return 0;
}
