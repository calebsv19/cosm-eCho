#include "mem_console_db.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *kTestDbPath = "build/targets/macOS-arm64/tests/mem_console_item_mutation_test.sqlite";

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

static void seed_state_for_item(MemConsoleState *state, int64_t item_id) {
    memset(state, 0, sizeof(*state));
    state->selected_item_id = item_id;
    snprintf(state->selected_title, sizeof(state->selected_title), "One");
    snprintf(state->selected_body, sizeof(state->selected_body), "body one");
    snprintf(state->title_edit_text, sizeof(state->title_edit_text), "Renamed");
    snprintf(state->body_edit_text, sizeof(state->body_edit_text), "updated body");
}

int main(void) {
    CoreMemDb db = {0};
    CoreResult result;
    MemConsoleState state;

    (void)unlink(kTestDbPath);
    result = core_memdb_open(kTestDbPath, &db);
    if (result.code != CORE_OK) {
        fprintf(stderr, "core_memdb_open failed: %d (%s)\n", (int)result.code, result.message ? result.message : "");
    }
    assert(result.code == CORE_OK);
    seed_items(&db);

    seed_state_for_item(&state, 1);
    result = rename_selected_from_title_buffer(&db, &state);
    assert(result.code == CORE_OK);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_item WHERE id = 1 AND title = 'Renamed';") == 1);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_item_fts WHERE rowid = 1 AND title = 'Renamed';") == 1);

    snprintf(state.selected_title, sizeof(state.selected_title), "Renamed");
    result = replace_selected_body_from_body_buffer(&db, &state);
    assert(result.code == CORE_OK);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_item WHERE id = 1 AND body = 'updated body';") == 1);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_item_fts WHERE rowid = 1 AND body = 'updated body';") == 1);

    result = set_selected_item_flag(&db, &state, MEM_CONSOLE_ITEM_FLAG_PINNED, 1);
    assert(result.code == CORE_OK);
    assert(query_i64(&db, "SELECT pinned FROM mem_item WHERE id = 1;") == 1);

    seed_state_for_item(&state, 3);
    result = rename_selected_from_title_buffer(&db, &state);
    assert(result.code == CORE_ERR_NOT_FOUND);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_item WHERE id = 3 AND title = 'Archived';") == 1);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_item_fts WHERE rowid = 3;") == 0);

    result = replace_selected_body_from_body_buffer(&db, &state);
    assert(result.code == CORE_ERR_NOT_FOUND);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_item WHERE id = 3 AND body = 'archived body';") == 1);
    assert(query_i64(&db, "SELECT COUNT(*) FROM mem_item_fts WHERE rowid = 3;") == 0);

    result = set_selected_item_flag(&db, &state, MEM_CONSOLE_ITEM_FLAG_CANONICAL, 1);
    assert(result.code == CORE_ERR_NOT_FOUND);
    assert(query_i64(&db, "SELECT canonical FROM mem_item WHERE id = 3;") == 0);

    seed_state_for_item(&state, 9999);
    result = set_selected_item_flag(&db, &state, MEM_CONSOLE_ITEM_FLAG_PINNED, 1);
    assert(result.code == CORE_ERR_NOT_FOUND);

    result = core_memdb_close(&db);
    assert(result.code == CORE_OK);
    (void)unlink(kTestDbPath);

    printf("mem_console_item_mutation_test: success\n");
    return 0;
}
