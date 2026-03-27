#include "mem_console_db_internal.h"

#include <stdio.h>
#include <string.h>

CoreResult bind_project_filters(CoreMemStmt *stmt,
                                       int start_index,
                                       const MemConsoleState *state) {
    static const char *k_unused_filter_key = "__mem_console_unused_filter__";
    CoreResult result;
    int i;
    int filter_count = 0;

    if (!stmt || !state || start_index <= 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid project filter bind request" };
    }

    filter_count = state->selected_project_count;
    if (filter_count < 0) {
        filter_count = 0;
    }
    if (filter_count > MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
        filter_count = MEM_CONSOLE_SCOPE_FILTER_LIMIT;
    }

    result = core_memdb_stmt_bind_i64(stmt, start_index, filter_count);
    if (result.code != CORE_OK) {
        return result;
    }

    for (i = 0; i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        const char *key = k_unused_filter_key;
        if (i < filter_count && state->selected_project_keys[i][0] != '\0') {
            key = state->selected_project_keys[i];
        }
        result = core_memdb_stmt_bind_text(stmt, start_index + 1 + i, key);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    return core_result_ok();
}

CoreResult read_project_filter_options(CoreMemDb *db, MemConsoleState *state) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }

    state->project_filter_option_count = 0;
    memset(state->project_filter_keys, 0, sizeof(state->project_filter_keys));
    memset(state->project_filter_labels, 0, sizeof(state->project_filter_labels));
    memset(state->project_filter_counts, 0, sizeof(state->project_filter_counts));

    result = core_memdb_prepare(db,
                                "SELECT project_key, COUNT(*) "
                                "FROM mem_item "
                                "WHERE archived_ns IS NULL AND project_key <> '' "
                                "GROUP BY project_key "
                                "ORDER BY COUNT(*) DESC, project_key ASC "
                                "LIMIT ?1;",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_stmt_bind_i64(&stmt, 1, MEM_CONSOLE_SCOPE_FILTER_LIMIT);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    for (;;) {
        CoreStr project_key = {0};
        int64_t item_count = 0;
        int index = state->project_filter_option_count;

        result = core_memdb_stmt_step(&stmt, &has_row);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        if (!has_row) {
            break;
        }
        if (state->project_filter_option_count >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
            break;
        }

        result = core_memdb_stmt_column_text(&stmt, 0, &project_key);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        result = core_memdb_stmt_column_i64(&stmt, 1, &item_count);
        if (result.code != CORE_OK) {
            goto cleanup;
        }

        copy_core_str(project_key,
                      state->project_filter_keys[index],
                      sizeof(state->project_filter_keys[index]));
        state->project_filter_counts[index] = item_count;
        (void)snprintf(state->project_filter_labels[index],
                       sizeof(state->project_filter_labels[index]),
                       "%s (%lld)",
                       state->project_filter_keys[index],
                       (long long)item_count);
        state->project_filter_option_count += 1;
    }

    mem_console_project_filter_prune_to_options(state);
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
