#include "mem_console_db.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

static int64_t current_time_ns(void) {
    Uint64 ticks = SDL_GetTicks64();
    return (int64_t)ticks * 1000000LL;
}

static int build_fingerprint(const char *title,
                             const char *body,
                             char *out_fingerprint,
                             size_t out_cap) {
    size_t title_len;
    size_t body_len;
    size_t buffer_len;
    char *buffer;
    uint64_t hash_value;
    int written;

    if (!title || !body || !out_fingerprint || out_cap < 17u) {
        return 0;
    }

    title_len = strlen(title);
    body_len = strlen(body);
    buffer_len = title_len + 1u + body_len;
    buffer = (char *)core_alloc(buffer_len);
    if (!buffer) {
        return 0;
    }

    memcpy(buffer, title, title_len);
    buffer[title_len] = '\n';
    memcpy(buffer + title_len + 1u, body, body_len);

    hash_value = core_hash64_fnv1a(buffer, buffer_len);
    core_free(buffer);

    written = snprintf(out_fingerprint, out_cap, "%016llx", (unsigned long long)hash_value);
    return written > 0 && (size_t)written < out_cap;
}

static CoreResult sync_fts_row(CoreMemDb *db, int64_t item_id, const char *title, const char *body) {
    CoreMemStmt delete_stmt = {0};
    CoreMemStmt insert_stmt = {0};
    CoreResult result;
    int has_row = 0;

    if (!db || !title || !body) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }

    result = core_memdb_prepare(db, "DELETE FROM mem_item_fts WHERE rowid = ?1;", &delete_stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&delete_stmt, 1, item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&delete_stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = core_memdb_stmt_finalize(&delete_stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = core_memdb_prepare(db,
                                "INSERT INTO mem_item_fts (rowid, title, body) VALUES (?1, ?2, ?3);",
                                &insert_stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&insert_stmt, 1, item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&insert_stmt, 2, title);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&insert_stmt, 3, body);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&insert_stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "fts write returned unexpected row" };
        goto cleanup;
    }

    result = core_result_ok();

cleanup:
    {
        CoreResult finalize_result = core_memdb_stmt_finalize(&insert_stmt);
        if (result.code == CORE_OK && finalize_result.code != CORE_OK) {
            result = finalize_result;
        }
    }
    {
        CoreResult finalize_result = core_memdb_stmt_finalize(&delete_stmt);
        if (result.code == CORE_OK && finalize_result.code != CORE_OK) {
            result = finalize_result;
        }
    }
    return result;
}

CoreResult create_item_from_search(CoreMemDb *db,
                                   MemConsoleState *state,
                                   int64_t *out_item_id) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    int tx_started = 0;
    int64_t now_ns;
    const char *create_project_key = "";
    char body[384];
    char fingerprint[17];

    if (!db || !state || !out_item_id) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    if (state->search_text[0] == '\0') {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "search text is empty" };
    }

    *out_item_id = 0;
    if (state->selected_project_count == 1 && state->selected_project_keys[0][0] != '\0') {
        create_project_key = state->selected_project_keys[0];
    }
    now_ns = current_time_ns();
    (void)snprintf(body,
                   sizeof(body),
                   "Created from mem_console search buffer.\nSeed text: %s",
                   state->search_text);
    if (!build_fingerprint(state->search_text, body, fingerprint, sizeof(fingerprint))) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to build fingerprint" };
    }

    result = core_memdb_tx_begin(db);
    if (result.code != CORE_OK) {
        return result;
    }
    tx_started = 1;

    result = core_memdb_prepare(db,
                                "INSERT INTO mem_item ("
                                "title, body, fingerprint, workspace_key, project_key, kind, created_ns, updated_ns, pinned, canonical, ttl_until_ns, archived_ns"
                                ") VALUES (?1, ?2, ?3, '', COALESCE(?4, ''), 'note', ?5, ?6, 0, 0, NULL, NULL) RETURNING id;",
                                &stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 1, state->search_text);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 2, body);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 3, fingerprint);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 4, create_project_key);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 5, now_ns);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 6, now_ns);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (!has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "insert did not return id" };
        goto cleanup;
    }
    result = core_memdb_stmt_column_i64(&stmt, 0, out_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = core_memdb_stmt_finalize(&stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = sync_fts_row(db, *out_item_id, state->search_text, body);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = core_memdb_tx_commit(db);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    tx_started = 0;
    return core_result_ok();

cleanup:
    (void)core_memdb_stmt_finalize(&stmt);
    if (tx_started) {
        (void)core_memdb_tx_rollback(db);
    }
    return result;
}

CoreResult rename_selected_from_title_buffer(CoreMemDb *db, MemConsoleState *state) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    int tx_started = 0;
    int64_t now_ns;
    char fingerprint[17];

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    if (state->selected_item_id == 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "no selected item" };
    }
    if (state->title_edit_text[0] == '\0') {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "title edit text is empty" };
    }
    if (!build_fingerprint(state->title_edit_text,
                           state->selected_body,
                           fingerprint,
                           sizeof(fingerprint))) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to build fingerprint" };
    }

    now_ns = current_time_ns();
    result = core_memdb_tx_begin(db);
    if (result.code != CORE_OK) {
        return result;
    }
    tx_started = 1;

    result = core_memdb_prepare(db,
                                "UPDATE mem_item "
                                "SET title = ?1, fingerprint = ?2, updated_ns = ?3 "
                                "WHERE id = ?4 AND archived_ns IS NULL;",
                                &stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 1, state->title_edit_text);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 2, fingerprint);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 3, now_ns);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 4, state->selected_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "update returned unexpected row" };
        goto cleanup;
    }

    result = core_memdb_stmt_finalize(&stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = sync_fts_row(db, state->selected_item_id, state->title_edit_text, state->selected_body);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = core_memdb_tx_commit(db);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    tx_started = 0;
    return core_result_ok();

cleanup:
    (void)core_memdb_stmt_finalize(&stmt);
    if (tx_started) {
        (void)core_memdb_tx_rollback(db);
    }
    return result;
}

CoreResult replace_selected_body_from_body_buffer(CoreMemDb *db, MemConsoleState *state) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    int tx_started = 0;
    int64_t now_ns;
    char fingerprint[17];

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    if (state->selected_item_id == 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "no selected item" };
    }
    if (state->body_edit_text[0] == '\0') {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "body edit text is empty" };
    }
    if (!build_fingerprint(state->selected_title,
                           state->body_edit_text,
                           fingerprint,
                           sizeof(fingerprint))) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to build fingerprint" };
    }

    now_ns = current_time_ns();
    result = core_memdb_tx_begin(db);
    if (result.code != CORE_OK) {
        return result;
    }
    tx_started = 1;

    result = core_memdb_prepare(db,
                                "UPDATE mem_item "
                                "SET body = ?1, fingerprint = ?2, updated_ns = ?3 "
                                "WHERE id = ?4 AND archived_ns IS NULL;",
                                &stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 1, state->body_edit_text);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 2, fingerprint);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 3, now_ns);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 4, state->selected_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "update returned unexpected row" };
        goto cleanup;
    }

    result = core_memdb_stmt_finalize(&stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = sync_fts_row(db, state->selected_item_id, state->selected_title, state->body_edit_text);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = core_memdb_tx_commit(db);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    tx_started = 0;
    return core_result_ok();

cleanup:
    (void)core_memdb_stmt_finalize(&stmt);
    if (tx_started) {
        (void)core_memdb_tx_rollback(db);
    }
    return result;
}

CoreResult set_selected_flag(CoreMemDb *db,
                             const MemConsoleState *state,
                             const char *field_name,
                             int field_value) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    char sql[160];

    if (!db || !state || !field_name) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    if (state->selected_item_id == 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "no selected item" };
    }

    (void)snprintf(sql,
                   sizeof(sql),
                   "UPDATE mem_item SET %s = ?1 WHERE id = ?2 AND archived_ns IS NULL;",
                   field_name);

    result = core_memdb_prepare(db, sql, &stmt);
    if (result.code != CORE_OK) {
        return result;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 1, field_value ? 1 : 0);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 2, state->selected_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "update returned unexpected row" };
        goto cleanup;
    }

    result = core_result_ok();

cleanup:
    {
        CoreResult finalize_result = core_memdb_stmt_finalize(&stmt);
        if (result.code == CORE_OK && finalize_result.code != CORE_OK) {
            result = finalize_result;
        }
    }
    return result;
}
