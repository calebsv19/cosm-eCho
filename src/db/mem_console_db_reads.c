#include "mem_console_db_internal.h"

#include <stdio.h>
#include <string.h>

CoreResult read_schema_version(CoreMemDb *db, MemConsoleState *state) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    CoreStr schema_value = {0};

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }

    state->schema_version[0] = '\0';

    result = core_memdb_prepare(db,
                                "SELECT value FROM mem_meta WHERE key = 'schema_version';",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (!has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "schema version row missing" };
        goto cleanup;
    }

    result = core_memdb_stmt_column_text(&stmt, 0, &schema_value);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    copy_core_str(schema_value, state->schema_version, sizeof(state->schema_version));
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

CoreResult read_active_count(CoreMemDb *db, MemConsoleState *state) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }

    state->active_count = 0;

    result = core_memdb_prepare(db,
                                "SELECT COUNT(*) "
                                "FROM mem_item "
                                "WHERE archived_ns IS NULL;",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (!has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "count query returned no row" };
        goto cleanup;
    }

    result = core_memdb_stmt_column_i64(&stmt, 0, &state->active_count);
    if (result.code != CORE_OK) {
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

CoreResult read_matching_count(CoreMemDb *db, MemConsoleState *state) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    char like_pattern[512];

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }

    state->matching_count = 0;
    build_like_pattern(state->search_text, like_pattern, sizeof(like_pattern));

    result = core_memdb_prepare(db,
                                "SELECT COUNT(*) "
                                "FROM mem_item "
                                "WHERE archived_ns IS NULL "
                                "AND (?1 = '' OR title LIKE ?2 OR body LIKE ?2) "
                                "AND (?3 = 0 OR project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19));",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_stmt_bind_text(&stmt, 1, state->search_text);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 2, like_pattern);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = bind_project_filters(&stmt, 3, state);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (!has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "matching count query returned no row" };
        goto cleanup;
    }

    result = core_memdb_stmt_column_i64(&stmt, 0, &state->matching_count);
    if (result.code != CORE_OK) {
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

CoreResult read_visible_items(CoreMemDb *db, MemConsoleState *state) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    char like_pattern[512];

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }

    state->visible_count = 0;
    state->visible_start_index = state->list_query_offset;
    build_like_pattern(state->search_text, like_pattern, sizeof(like_pattern));

    result = core_memdb_prepare(db,
                                "SELECT id, title, pinned, canonical, workspace_key, project_key, kind "
                                "FROM mem_item "
                                "WHERE archived_ns IS NULL "
                                "AND (?1 = '' OR title LIKE ?2 OR body LIKE ?2) "
                                "AND (?3 = 0 OR project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19)) "
                                "ORDER BY pinned DESC, updated_ns DESC, id ASC "
                                "LIMIT ?20 OFFSET ?21;",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_stmt_bind_text(&stmt, 1, state->search_text);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 2, like_pattern);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = bind_project_filters(&stmt, 3, state);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 20, MEM_CONSOLE_LIST_FETCH_LIMIT);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 21, state->list_query_offset);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    for (;;) {
        MemConsoleListItem *item;
        CoreStr title = {0};
        int64_t flag_value = 0;

        result = core_memdb_stmt_step(&stmt, &has_row);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        if (!has_row) {
            break;
        }
        if (state->visible_count >= MEM_CONSOLE_LIST_FETCH_LIMIT) {
            break;
        }

        item = &state->visible_items[state->visible_count];
        memset(item, 0, sizeof(*item));

        result = core_memdb_stmt_column_i64(&stmt, 0, &item->id);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        result = core_memdb_stmt_column_text(&stmt, 1, &title);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        copy_core_str(title, item->title, sizeof(item->title));

        result = core_memdb_stmt_column_i64(&stmt, 2, &flag_value);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        item->pinned = flag_value ? 1 : 0;

        result = core_memdb_stmt_column_i64(&stmt, 3, &flag_value);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        item->canonical = flag_value ? 1 : 0;

        {
            CoreStr value = {0};

            result = core_memdb_stmt_column_text(&stmt, 4, &value);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            copy_core_str(value, item->workspace_key, sizeof(item->workspace_key));

            result = core_memdb_stmt_column_text(&stmt, 5, &value);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            copy_core_str(value, item->project_key, sizeof(item->project_key));

            result = core_memdb_stmt_column_text(&stmt, 6, &value);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            copy_core_str(value, item->kind, sizeof(item->kind));
        }

        state->visible_count += 1;
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

void clamp_list_query_offset(MemConsoleState *state) {
    int64_t max_index;

    if (!state) {
        return;
    }

    if (state->matching_count <= 0) {
        state->list_query_offset = 0;
        state->visible_start_index = 0;
        return;
    }

    if (state->list_query_offset < 0) {
        state->list_query_offset = 0;
    }

    max_index = state->matching_count - 1;
    if ((int64_t)state->list_query_offset > max_index) {
        state->list_query_offset = (int)max_index;
    }
}

CoreResult read_selected_detail(CoreMemDb *db, MemConsoleState *state) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    if (state->selected_item_id == 0) {
        set_default_detail(state);
        return core_result_ok();
    }

    result = core_memdb_prepare(db,
                                "SELECT title, body, pinned, canonical, created_ns "
                                "FROM mem_item "
                                "WHERE id = ?1 AND archived_ns IS NULL;",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_stmt_bind_i64(&stmt, 1, state->selected_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (!has_row) {
        set_default_detail(state);
        result = core_result_ok();
        goto cleanup;
    }

    {
        CoreStr title = {0};
        CoreStr body = {0};
        int64_t flag_value = 0;

        result = core_memdb_stmt_column_text(&stmt, 0, &title);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        result = core_memdb_stmt_column_text(&stmt, 1, &body);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        copy_core_str(title, state->selected_title, sizeof(state->selected_title));
        copy_core_str(body, state->selected_body, sizeof(state->selected_body));

        result = core_memdb_stmt_column_i64(&stmt, 2, &flag_value);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        state->selected_pinned = flag_value ? 1 : 0;

        result = core_memdb_stmt_column_i64(&stmt, 3, &flag_value);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        state->selected_canonical = flag_value ? 1 : 0;

        result = core_memdb_stmt_column_i64(&stmt, 4, &state->selected_created_ns);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
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
