#include "mem_console_db.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static const char *kRelationshipKinds[] = {
    "related",
    "supports",
    "depends_on",
    "references",
    "summarizes",
    "implements",
    "blocks",
    "contradicts"
};

static int parse_target_item_id(const char *text, int64_t *out_item_id) {
    char *end = 0;
    long long parsed;

    if (!text || !out_item_id || text[0] == '\0') {
        return 0;
    }

    errno = 0;
    parsed = strtoll(text, &end, 10);
    if (errno != 0 || end == text || !end || *end != '\0' || parsed <= 0) {
        return 0;
    }

    *out_item_id = (int64_t)parsed;
    return 1;
}

static int relationship_kind_index(const char *kind) {
    int i;

    if (!kind || !kind[0]) {
        return 0;
    }
    for (i = 0; i < (int)(sizeof(kRelationshipKinds) / sizeof(kRelationshipKinds[0])); ++i) {
        if (strcmp(kind, kRelationshipKinds[i]) == 0) {
            return i;
        }
    }
    return 0;
}

static const char *next_relationship_kind(const char *kind) {
    int index = relationship_kind_index(kind);
    int count = (int)(sizeof(kRelationshipKinds) / sizeof(kRelationshipKinds[0]));
    return kRelationshipKinds[(index + 1) % count];
}

static void relationship_copy_core_str(CoreStr value, char *out_text, size_t out_cap) {
    size_t copy_len;

    if (!out_text || out_cap == 0u) {
        return;
    }
    out_text[0] = '\0';
    if (!value.data || value.len == 0u) {
        return;
    }

    copy_len = value.len;
    if (copy_len >= out_cap) {
        copy_len = out_cap - 1u;
    }
    memcpy(out_text, value.data, copy_len);
    out_text[copy_len] = '\0';
}

static CoreResult ensure_active_item(CoreMemDb *db, int64_t item_id, const char *missing_message) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    int64_t count = 0;

    if (!db || item_id <= 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid item id" };
    }

    result = core_memdb_prepare(db,
                                "SELECT COUNT(*) FROM mem_item WHERE id = ?1 AND archived_ns IS NULL;",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 1, item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (!has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "active item check returned no row" };
        goto cleanup;
    }
    result = core_memdb_stmt_column_i64(&stmt, 0, &count);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (count != 1) {
        result = (CoreResult){ CORE_ERR_NOT_FOUND, missing_message };
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

CoreResult create_selected_relationship_to_target(CoreMemDb *db,
                                                  const MemConsoleState *state,
                                                  int64_t *out_link_id) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    int64_t target_item_id = 0;

    if (!db || !state || !out_link_id) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    *out_link_id = 0;
    if (state->selected_item_id <= 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "no selected item" };
    }
    if (!parse_target_item_id(state->relationship_target_text, &target_item_id)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "enter a numeric target memory id" };
    }
    if (target_item_id == state->selected_item_id) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "relationship target must differ from selected memory" };
    }

    result = ensure_active_item(db, state->selected_item_id, "selected memory not found");
    if (result.code != CORE_OK) {
        return result;
    }
    result = ensure_active_item(db, target_item_id, "target memory not found");
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_prepare(db,
                                "INSERT INTO mem_link (from_item_id, to_item_id, kind, weight, note) "
                                "VALUES (?1, ?2, 'related', 1.0, 'mem_console relationship editor') "
                                "RETURNING id;",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 1, state->selected_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 2, target_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (!has_row) {
        result = (CoreResult){ CORE_ERR_FORMAT, "relationship insert did not return id" };
        goto cleanup;
    }
    result = core_memdb_stmt_column_i64(&stmt, 0, out_link_id);
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

CoreResult cycle_selected_relationship_kind(CoreMemDb *db,
                                            const MemConsoleState *state,
                                            int64_t *out_link_id) {
    CoreMemStmt read_stmt = {0};
    CoreMemStmt update_stmt = {0};
    CoreResult result;
    int has_row = 0;
    CoreStr current_kind = {0};
    char kind_buffer[32];
    const char *next_kind;

    if (!db || !state || !out_link_id) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    *out_link_id = 0;
    if (state->selected_item_id <= 0 || state->relationship_action_link_id <= 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "select a relationship first" };
    }

    result = core_memdb_prepare(db,
                                "SELECT kind FROM mem_link "
                                "WHERE id = ?1 AND (from_item_id = ?2 OR to_item_id = ?2);",
                                &read_stmt);
    if (result.code != CORE_OK) {
        return result;
    }
    result = core_memdb_stmt_bind_i64(&read_stmt, 1, state->relationship_action_link_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&read_stmt, 2, state->selected_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&read_stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (!has_row) {
        result = (CoreResult){ CORE_ERR_NOT_FOUND, "relationship not found for selected memory" };
        goto cleanup;
    }
    result = core_memdb_stmt_column_text(&read_stmt, 0, &current_kind);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    relationship_copy_core_str(current_kind, kind_buffer, sizeof(kind_buffer));
    next_kind = next_relationship_kind(kind_buffer);

    result = core_memdb_stmt_finalize(&read_stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    memset(&read_stmt, 0, sizeof(read_stmt));

    result = core_memdb_prepare(db,
                                "UPDATE mem_link SET kind = ?1 "
                                "WHERE id = ?2 AND (from_item_id = ?3 OR to_item_id = ?3) "
                                "RETURNING id;",
                                &update_stmt);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&update_stmt, 1, next_kind);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&update_stmt, 2, state->relationship_action_link_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&update_stmt, 3, state->selected_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_step(&update_stmt, &has_row);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    if (!has_row) {
        result = (CoreResult){ CORE_ERR_NOT_FOUND, "relationship kind update did not affect a row" };
        goto cleanup;
    }
    result = core_memdb_stmt_column_i64(&update_stmt, 0, out_link_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    result = core_result_ok();

cleanup:
    {
        CoreResult finalize_result = core_memdb_stmt_finalize(&update_stmt);
        if (result.code == CORE_OK && finalize_result.code != CORE_OK) {
            result = finalize_result;
        }
    }
    {
        CoreResult finalize_result = core_memdb_stmt_finalize(&read_stmt);
        if (result.code == CORE_OK && finalize_result.code != CORE_OK) {
            result = finalize_result;
        }
    }
    return result;
}

CoreResult remove_selected_relationship(CoreMemDb *db,
                                        const MemConsoleState *state,
                                        int64_t *out_link_id) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;

    if (!db || !state || !out_link_id) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    *out_link_id = 0;
    if (state->selected_item_id <= 0 || state->relationship_action_link_id <= 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "select a relationship first" };
    }

    result = core_memdb_prepare(db,
                                "DELETE FROM mem_link "
                                "WHERE id = ?1 AND (from_item_id = ?2 OR to_item_id = ?2) "
                                "RETURNING id;",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 1, state->relationship_action_link_id);
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
    if (!has_row) {
        result = (CoreResult){ CORE_ERR_NOT_FOUND, "relationship not found for selected memory" };
        goto cleanup;
    }
    result = core_memdb_stmt_column_i64(&stmt, 0, out_link_id);
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
