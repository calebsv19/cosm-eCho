#include "mem_console_db.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *kTestDbPath = "build/targets/macOS-arm64/tests/mem_console_relationship_mutation_test.sqlite";

static int64_t query_i64(CoreMemDb *db, const char *sql) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    int64_t value = -1;

    result = core_memdb_prepare(db, sql, &stmt);
    assert(result.code == CORE_OK);
    result = core_memdb_stmt_step(&stmt, &has_row);
    assert(result.code == CORE_OK);
    assert(has_row == 1);
    result = core_memdb_stmt_column_i64(&stmt, 0, &value);
    assert(result.code == CORE_OK);
    result = core_memdb_stmt_finalize(&stmt);
    assert(result.code == CORE_OK);
    return value;
}

static void seed_items(CoreMemDb *db) {
    CoreResult result = core_memdb_exec(
        db,
        "INSERT INTO mem_item (id, stable_id, title, body, fingerprint, workspace_key, project_key, kind, created_ns, updated_ns, pinned, canonical, ttl_until_ns, archived_ns) "
        "VALUES "
        "(1, 'one', 'One', 'body one', 'f1', 'codework', 'memory_console', 'note', 1, 1, 0, 0, NULL, NULL),"
        "(2, 'two', 'Two', 'body two', 'f2', 'codework', 'memory_console', 'note', 2, 2, 0, 0, NULL, NULL),"
        "(3, 'archived', 'Archived', 'archived body', 'f3', 'codework', 'memory_console', 'note', 3, 3, 0, 0, NULL, 4);");
    assert(result.code == CORE_OK);
}

int main(void) {
    CoreMemDb db = {0};
    CoreResult result;
    MemConsoleState state;
    int64_t link_id = 0;

    (void)unlink(kTestDbPath);
    result = core_memdb_open(kTestDbPath, &db);
    if (result.code != CORE_OK) {
        fprintf(stderr, "core_memdb_open failed: %d (%s)\n", (int)result.code, result.message ? result.message : "");
    }
    assert(result.code == CORE_OK);
    seed_items(&db);

    memset(&state, 0, sizeof(state));
    state.selected_item_id = 1;
    snprintf(state.relationship_target_text, sizeof(state.relationship_target_text), "2");

    result = create_selected_relationship_to_target(&db, &state, &link_id);
    assert(result.code == CORE_OK);
    assert(link_id > 0);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_link WHERE from_item_id = 1 AND to_item_id = 2 AND kind = 'related';") == 1);

    state.relationship_action_link_id = link_id;
    result = cycle_selected_relationship_kind(&db, &state, &link_id);
    assert(result.code == CORE_OK);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_link WHERE from_item_id = 1 AND to_item_id = 2 AND kind = 'supports';") == 1);

    result = remove_selected_relationship(&db, &state, &link_id);
    assert(result.code == CORE_OK);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_link;") == 0);

    snprintf(state.relationship_target_text, sizeof(state.relationship_target_text), "1");
    result = create_selected_relationship_to_target(&db, &state, &link_id);
    assert(result.code == CORE_ERR_INVALID_ARG);

    snprintf(state.relationship_target_text, sizeof(state.relationship_target_text), "3");
    result = create_selected_relationship_to_target(&db, &state, &link_id);
    assert(result.code == CORE_ERR_NOT_FOUND);

    state.relationship_action_link_id = 9999;
    result = remove_selected_relationship(&db, &state, &link_id);
    assert(result.code == CORE_ERR_NOT_FOUND);

    result = core_memdb_close(&db);
    assert(result.code == CORE_OK);
    (void)unlink(kTestDbPath);

    printf("mem_console_relationship_mutation_test: success\n");
    return 0;
}
