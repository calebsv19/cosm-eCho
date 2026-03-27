#include "mem_console_db.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

static void normalize_ascii_in_place(char *text) {
    size_t r = 0u;
    size_t w = 0u;
    int last_was_space = 0;

    if (!text) {
        return;
    }

    while (text[r] != '\0') {
        unsigned char c = (unsigned char)text[r];
        char out_ch = 0;

        if (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
            out_ch = ' ';
        } else if (c >= 32u && c <= 126u) {
            out_ch = (char)c;
        }

        if (out_ch == ' ') {
            if (!last_was_space) {
                text[w++] = out_ch;
                last_was_space = 1;
            }
        } else if (out_ch != 0) {
            text[w++] = out_ch;
            last_was_space = 0;
        }

        r += 1u;
    }
    while (w > 0u && text[w - 1u] == ' ') {
        w -= 1u;
    }
    text[w] = '\0';
}

static int find_graph_node_index(const MemConsoleState *state, int64_t item_id) {
    int i;

    if (!state || item_id == 0) {
        return -1;
    }

    for (i = 0; i < state->graph_node_count; ++i) {
        if (state->graph_nodes[i].item_id == item_id) {
            return i;
        }
    }

    return -1;
}

static CoreResult load_graph_node_detail(CoreMemDb *db, int64_t item_id, MemConsoleGraphNode *out_node) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    CoreStr title = {0};
    CoreStr project_key = {0};
    CoreStr kind = {0};
    CoreStr stable_id = {0};
    int64_t flag_value = 0;

    if (!db || !out_node || item_id == 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph node request" };
    }

    memset(out_node, 0, sizeof(*out_node));
    out_node->item_id = item_id;

    result = core_memdb_prepare(db,
                                "SELECT title, body, pinned, canonical, created_ns, project_key, kind, stable_id "
                                "FROM mem_item "
                                "WHERE id = ?1 AND archived_ns IS NULL;",
                                &stmt);
    if (result.code != CORE_OK) {
        result = core_memdb_prepare(db,
                                    "SELECT title, body, pinned, canonical, created_ns, project_key, kind, '' "
                                    "FROM mem_item "
                                    "WHERE id = ?1 AND archived_ns IS NULL;",
                                    &stmt);
        if (result.code != CORE_OK) {
            result = core_memdb_prepare(db,
                                    "SELECT title, body, pinned, canonical, created_ns, project_key, '', '' "
                                    "FROM mem_item "
                                    "WHERE id = ?1 AND archived_ns IS NULL;",
                                    &stmt);
        }
        if (result.code != CORE_OK) {
            result = core_memdb_prepare(db,
                                        "SELECT title, body, pinned, canonical, created_ns, '', '', '' "
                                        "FROM mem_item "
                                        "WHERE id = ?1 AND archived_ns IS NULL;",
                                        &stmt);
        }
        if (result.code != CORE_OK) {
            return result;
        }
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
        result = (CoreResult){ CORE_ERR_NOT_FOUND, "graph node item not found" };
        goto cleanup;
    }

    result = core_memdb_stmt_column_text(&stmt, 0, &title);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    copy_core_str(title, out_node->title, sizeof(out_node->title));
    normalize_ascii_in_place(out_node->title);
    if (out_node->title[0] == '\0') {
        (void)snprintf(out_node->title, sizeof(out_node->title), "ITEM %lld", (long long)item_id);
    }

    {
        CoreStr body = {0};
        result = core_memdb_stmt_column_text(&stmt, 1, &body);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        copy_core_str(body, out_node->body_preview, sizeof(out_node->body_preview));
        normalize_ascii_in_place(out_node->body_preview);
    }

    result = core_memdb_stmt_column_i64(&stmt, 2, &flag_value);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    out_node->pinned = flag_value ? 1 : 0;

    result = core_memdb_stmt_column_i64(&stmt, 3, &flag_value);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    out_node->canonical = flag_value ? 1 : 0;

    result = core_memdb_stmt_column_i64(&stmt, 4, &out_node->created_ns);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_column_text(&stmt, 5, &project_key);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    copy_core_str(project_key, out_node->project_key, sizeof(out_node->project_key));
    normalize_ascii_in_place(out_node->project_key);
    result = core_memdb_stmt_column_text(&stmt, 6, &kind);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    copy_core_str(kind, out_node->kind, sizeof(out_node->kind));
    normalize_ascii_in_place(out_node->kind);
    result = core_memdb_stmt_column_text(&stmt, 7, &stable_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    copy_core_str(stable_id, out_node->stable_id, sizeof(out_node->stable_id));
    normalize_ascii_in_place(out_node->stable_id);
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

static CoreResult ensure_graph_node(CoreMemDb *db,
                                    MemConsoleState *state,
                                    int64_t item_id,
                                    int *out_index) {
    int existing_index;
    CoreResult result;
    MemConsoleGraphNode node;

    if (!db || !state || !out_index || item_id == 0) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph node ensure request" };
    }

    existing_index = find_graph_node_index(state, item_id);
    if (existing_index >= 0) {
        *out_index = existing_index;
        return core_result_ok();
    }

    if (state->graph_node_count >= MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "graph node limit reached" };
    }

    result = load_graph_node_detail(db, item_id, &node);
    if (result.code != CORE_OK) {
        return result;
    }

    state->graph_nodes[state->graph_node_count] = node;
    *out_index = state->graph_node_count;
    state->graph_node_count += 1;
    return core_result_ok();
}

static int is_graph_node_limit_result(CoreResult result) {
    return result.code == CORE_ERR_NOT_FOUND &&
           result.message &&
           strcmp(result.message, "graph node limit reached") == 0;
}

static int graph_sort_nodes_should_swap(const MemConsoleState *state,
                                        const MemConsoleGraphNode *left,
                                        const MemConsoleGraphNode *right) {
    if (!state || !left || !right) {
        return 0;
    }
    if (state->graph_sort_mode == MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST) {
        if (left->created_ns > right->created_ns) {
            return 1;
        }
        if (left->created_ns < right->created_ns) {
            return 0;
        }
        return left->item_id > right->item_id ? 1 : 0;
    }

    if (left->created_ns < right->created_ns) {
        return 1;
    }
    if (left->created_ns > right->created_ns) {
        return 0;
    }
    return left->item_id < right->item_id ? 1 : 0;
}

static void apply_graph_node_sort(MemConsoleState *state) {
    MemConsoleGraphNode sorted_nodes[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int order[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int old_to_new[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int node_count = 0;
    int i;
    int j;
    int changed = 0;

    if (!state) {
        return;
    }

    node_count = state->graph_node_count;
    if (node_count <= 1) {
        return;
    }
    if (node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }

    for (i = 0; i < node_count; ++i) {
        order[i] = i;
    }

    for (i = 1; i < node_count; ++i) {
        int key = order[i];
        j = i - 1;
        while (j >= 0 &&
               graph_sort_nodes_should_swap(state,
                                            &state->graph_nodes[order[j]],
                                            &state->graph_nodes[key])) {
            order[j + 1] = order[j];
            j -= 1;
        }
        order[j + 1] = key;
    }

    for (i = 0; i < node_count; ++i) {
        if (order[i] != i) {
            changed = 1;
            break;
        }
    }
    if (!changed) {
        return;
    }

    for (i = 0; i < node_count; ++i) {
        sorted_nodes[i] = state->graph_nodes[order[i]];
        old_to_new[order[i]] = i;
    }
    for (i = 0; i < node_count; ++i) {
        state->graph_nodes[i] = sorted_nodes[i];
    }

    for (i = 0; i < state->graph_edge_count; ++i) {
        int from_index = state->graph_edges[i].from_index;
        int to_index = state->graph_edges[i].to_index;
        if (from_index >= 0 && from_index < node_count) {
            state->graph_edges[i].from_index = old_to_new[from_index];
        }
        if (to_index >= 0 && to_index < node_count) {
            state->graph_edges[i].to_index = old_to_new[to_index];
        }
    }
}

static void compact_graph_by_node_kind(MemConsoleState *state) {
    MemConsoleGraphNode kept_nodes[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    MemConsoleGraphEdge kept_edges[MEM_CONSOLE_GRAPH_EDGE_LIMIT];
    int old_to_new[MEM_CONSOLE_GRAPH_NODE_LIMIT];
    int old_node_count;
    int old_edge_count;
    int kept_node_count = 0;
    int kept_edge_count = 0;
    int i;

    if (!state) {
        return;
    }

    old_node_count = state->graph_node_count;
    old_edge_count = state->graph_edge_count;
    if (old_node_count < 0) {
        old_node_count = 0;
    }
    if (old_node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        old_node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }
    if (old_edge_count < 0) {
        old_edge_count = 0;
    }
    if (old_edge_count > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        old_edge_count = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }

    for (i = 0; i < old_node_count; ++i) {
        old_to_new[i] = -1;
        if (!mem_console_graph_node_kind_is_enabled(state, state->graph_nodes[i].kind)) {
            continue;
        }
        if (kept_node_count >= MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            continue;
        }
        kept_nodes[kept_node_count] = state->graph_nodes[i];
        old_to_new[i] = kept_node_count;
        kept_node_count += 1;
    }

    for (i = 0; i < old_edge_count; ++i) {
        int old_from = state->graph_edges[i].from_index;
        int old_to = state->graph_edges[i].to_index;
        int new_from;
        int new_to;

        if (kept_edge_count >= MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
            break;
        }
        if (old_from < 0 || old_to < 0 || old_from >= old_node_count || old_to >= old_node_count) {
            continue;
        }
        new_from = old_to_new[old_from];
        new_to = old_to_new[old_to];
        if (new_from < 0 || new_to < 0) {
            continue;
        }
        kept_edges[kept_edge_count] = state->graph_edges[i];
        kept_edges[kept_edge_count].from_index = new_from;
        kept_edges[kept_edge_count].to_index = new_to;
        kept_edge_count += 1;
    }

    for (i = 0; i < kept_node_count; ++i) {
        state->graph_nodes[i] = kept_nodes[i];
    }
    for (i = 0; i < kept_edge_count; ++i) {
        state->graph_edges[i] = kept_edges[i];
    }
    state->graph_node_count = kept_node_count;
    state->graph_edge_count = kept_edge_count;
}

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

static CoreResult bind_project_filters(CoreMemStmt *stmt,
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

static CoreResult read_project_filter_options(CoreMemDb *db, MemConsoleState *state) {
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

static CoreResult read_schema_version(CoreMemDb *db, MemConsoleState *state) {
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

static CoreResult read_active_count(CoreMemDb *db, MemConsoleState *state) {
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

static CoreResult read_matching_count(CoreMemDb *db, MemConsoleState *state) {
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
                                "AND (?3 = 0 OR project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11));",
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

static CoreResult read_visible_items(CoreMemDb *db, MemConsoleState *state) {
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
                                "AND (?3 = 0 OR project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)) "
                                "ORDER BY pinned DESC, updated_ns DESC, id ASC "
                                "LIMIT ?12 OFFSET ?13;",
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
    result = core_memdb_stmt_bind_i64(&stmt, 12, MEM_CONSOLE_LIST_FETCH_LIMIT);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 13, state->list_query_offset);
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

static void clamp_list_query_offset(MemConsoleState *state) {
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

static CoreResult read_selected_detail(CoreMemDb *db, MemConsoleState *state) {
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

CoreResult load_graph_neighborhood(CoreMemDb *db, MemConsoleState *state) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    int edge_limit = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    int graph_hops = MEM_CONSOLE_GRAPH_HOPS_MIN;
    int use_full_scope = 0;
    int64_t center_item_id = 0;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph neighborhood request" };
    }

    edge_limit = mem_console_graph_edge_limit_clamp(state->graph_query_edge_limit);
    state->graph_query_edge_limit = edge_limit;
    graph_hops = mem_console_graph_hops_clamp(state->graph_query_hops);
    state->graph_query_hops = graph_hops;
    use_full_scope = state->graph_scope_full_mode_enabled ? 1 : 0;

    state->graph_node_count = 0;
    state->graph_edge_count = 0;

    if (use_full_scope) {
        result = core_memdb_prepare(db,
                                    "SELECT l.from_item_id, l.to_item_id, l.kind "
                                    "FROM mem_link l "
                                    "JOIN mem_item src ON src.id = l.from_item_id AND src.archived_ns IS NULL "
                                    "                  AND (?3 = 0 OR src.project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)) "
                                    "JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                    "                  AND (?3 = 0 OR dst.project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)) "
                                    "WHERE (?1 = '' OR l.kind = ?1) "
                                    "ORDER BY l.id DESC "
                                    "LIMIT ?2;",
                                    &stmt);
        if (result.code != CORE_OK) {
            return result;
        }

        result = core_memdb_stmt_bind_text(&stmt, 1, state->graph_kind_filter);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        result = core_memdb_stmt_bind_i64(&stmt, 2, edge_limit);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        result = bind_project_filters(&stmt, 3, state);
        if (result.code != CORE_OK) {
            goto cleanup;
        }

        for (;;) {
            int64_t from_item_id = 0;
            int64_t to_item_id = 0;
            CoreStr kind = {0};
            char edge_kind[32];
            int from_index = -1;
            int to_index = -1;

            result = core_memdb_stmt_step(&stmt, &has_row);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            if (!has_row) {
                break;
            }
            if (state->graph_edge_count >= MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
                break;
            }

            result = core_memdb_stmt_column_i64(&stmt, 0, &from_item_id);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            result = core_memdb_stmt_column_i64(&stmt, 1, &to_item_id);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            result = core_memdb_stmt_column_text(&stmt, 2, &kind);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            copy_core_str(kind, edge_kind, sizeof(edge_kind));
            normalize_ascii_in_place(edge_kind);
            if (!mem_console_graph_kind_is_enabled(state, edge_kind)) {
                continue;
            }
            result = ensure_graph_node(db, state, from_item_id, &from_index);
            if (result.code != CORE_OK) {
                if (is_graph_node_limit_result(result)) {
                    continue;
                }
                goto cleanup;
            }
            result = ensure_graph_node(db, state, to_item_id, &to_index);
            if (result.code != CORE_OK) {
                if (is_graph_node_limit_result(result)) {
                    continue;
                }
                goto cleanup;
            }

            state->graph_edges[state->graph_edge_count].from_index = from_index;
            state->graph_edges[state->graph_edge_count].to_index = to_index;
            (void)snprintf(state->graph_edges[state->graph_edge_count].kind,
                           sizeof(state->graph_edges[state->graph_edge_count].kind),
                           "%s",
                           edge_kind);
            state->graph_edge_count += 1;
        }

        {
            CoreResult finalize_result = core_memdb_stmt_finalize(&stmt);
            memset(&stmt, 0, sizeof(stmt));
            if (result.code == CORE_OK && finalize_result.code != CORE_OK) {
                result = finalize_result;
            }
            if (result.code != CORE_OK) {
                return result;
            }
        }

        result = core_memdb_prepare(db,
                                    "SELECT i.id, i.kind "
                                    "FROM mem_item i "
                                    "WHERE i.archived_ns IS NULL "
                                    "  AND (?1 = 0 OR i.project_key IN (?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)) "
                                    "ORDER BY i.updated_ns DESC, i.id DESC "
                                    "LIMIT ?10;",
                                    &stmt);
        if (result.code != CORE_OK) {
            result = core_memdb_prepare(db,
                                        "SELECT i.id, '' "
                                        "FROM mem_item i "
                                        "WHERE i.archived_ns IS NULL "
                                        "  AND (?1 = 0 OR i.project_key IN (?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)) "
                                        "ORDER BY i.updated_ns DESC, i.id DESC "
                                        "LIMIT ?10;",
                                        &stmt);
            if (result.code != CORE_OK) {
                return result;
            }
        }

        result = bind_project_filters(&stmt, 1, state);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        result = core_memdb_stmt_bind_i64(&stmt, 10, MEM_CONSOLE_GRAPH_NODE_LIMIT);
        if (result.code != CORE_OK) {
            goto cleanup;
        }

        for (;;) {
            int64_t item_id = 0;
            CoreStr kind = {0};
            char node_kind[32];
            int node_index = -1;

            result = core_memdb_stmt_step(&stmt, &has_row);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            if (!has_row) {
                break;
            }
            result = core_memdb_stmt_column_i64(&stmt, 0, &item_id);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            result = core_memdb_stmt_column_text(&stmt, 1, &kind);
            if (result.code != CORE_OK) {
                goto cleanup;
            }
            copy_core_str(kind, node_kind, sizeof(node_kind));
            normalize_ascii_in_place(node_kind);
            if (!mem_console_graph_node_kind_is_enabled(state, node_kind)) {
                continue;
            }
            result = ensure_graph_node(db, state, item_id, &node_index);
            if (result.code != CORE_OK && !is_graph_node_limit_result(result)) {
                goto cleanup;
            }
        }

        {
            CoreResult finalize_result = core_memdb_stmt_finalize(&stmt);
            memset(&stmt, 0, sizeof(stmt));
            if (result.code == CORE_OK && finalize_result.code != CORE_OK) {
                return finalize_result;
            }
            if (result.code != CORE_OK) {
                return result;
            }
        }

        compact_graph_by_node_kind(state);
        apply_graph_node_sort(state);

        if (state->selected_item_id != 0 &&
            find_graph_node_index(state, state->selected_item_id) >= 0) {
            state->graph_center_item_id = state->selected_item_id;
        } else if (state->graph_center_item_id != 0 &&
                   find_graph_node_index(state, state->graph_center_item_id) >= 0) {
            /* keep existing center */
        } else if (state->graph_node_count > 0) {
            state->graph_center_item_id = state->graph_nodes[0].item_id;
        } else {
            state->graph_center_item_id = 0;
        }

        return core_result_ok();
    }

    center_item_id = state->graph_center_item_id;
    if (center_item_id == 0) {
        center_item_id = state->selected_item_id;
    }
    if (center_item_id == 0) {
        return core_result_ok();
    }

    {
        int selected_index = -1;
        result = ensure_graph_node(db, state, center_item_id, &selected_index);
        if (result.code != CORE_OK &&
            state->selected_item_id != 0 &&
            state->selected_item_id != center_item_id) {
            center_item_id = state->selected_item_id;
            result = ensure_graph_node(db, state, center_item_id, &selected_index);
        }
        if (result.code != CORE_OK) {
            state->graph_center_item_id = 0;
            return core_result_ok();
        }
        state->graph_center_item_id = center_item_id;
    }

    result = core_memdb_prepare(db,
                                "WITH RECURSIVE walk(node_id, depth) AS ("
                                "  SELECT ?1, 0 "
                                "  UNION "
                                "  SELECT CASE WHEN l.from_item_id = walk.node_id THEN l.to_item_id ELSE l.from_item_id END, "
                                "         walk.depth + 1 "
                                "  FROM walk "
                                "  JOIN mem_link l ON (l.from_item_id = walk.node_id OR l.to_item_id = walk.node_id) "
                                "  JOIN mem_item src ON src.id = l.from_item_id AND src.archived_ns IS NULL "
                                "                    AND (?5 = 0 OR src.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13)) "
                                "  JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                "                    AND (?5 = 0 OR dst.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13)) "
                                "  WHERE walk.depth < ?2 "
                                "    AND (?3 = '' OR l.kind = ?3)"
                                "), nodes AS ("
                                "  SELECT DISTINCT node_id FROM walk"
                                "), edges AS ("
                                "  SELECT l.from_item_id, l.to_item_id, l.kind, l.id "
                                "  FROM mem_link l "
                                "  JOIN nodes a ON a.node_id = l.from_item_id "
                                "  JOIN nodes b ON b.node_id = l.to_item_id "
                                "  JOIN mem_item src ON src.id = l.from_item_id AND src.archived_ns IS NULL "
                                "                    AND (?5 = 0 OR src.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13)) "
                                "  JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                "                    AND (?5 = 0 OR dst.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13)) "
                                "  WHERE (?3 = '' OR l.kind = ?3) "
                                "  ORDER BY l.id ASC "
                                "  LIMIT ?4"
                                ") "
                                "SELECT from_item_id, to_item_id, kind "
                                "FROM edges "
                                "ORDER BY id ASC;",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_stmt_bind_i64(&stmt, 1, center_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 2, graph_hops);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 3, state->graph_kind_filter);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 4, edge_limit);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = bind_project_filters(&stmt, 5, state);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    for (;;) {
        int64_t from_item_id = 0;
        int64_t to_item_id = 0;
        CoreStr kind = {0};
        int from_index = -1;
        int to_index = -1;

        result = core_memdb_stmt_step(&stmt, &has_row);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        if (!has_row) {
            break;
        }
        if (state->graph_edge_count >= MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
            break;
        }

        result = core_memdb_stmt_column_i64(&stmt, 0, &from_item_id);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        result = core_memdb_stmt_column_i64(&stmt, 1, &to_item_id);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        result = core_memdb_stmt_column_text(&stmt, 2, &kind);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        {
            char edge_kind[32];
            copy_core_str(kind, edge_kind, sizeof(edge_kind));
            normalize_ascii_in_place(edge_kind);
            if (!mem_console_graph_kind_is_enabled(state, edge_kind)) {
                continue;
            }
        }
        result = ensure_graph_node(db, state, from_item_id, &from_index);
        if (result.code != CORE_OK) {
            if (is_graph_node_limit_result(result)) {
                continue;
            }
            goto cleanup;
        }
        result = ensure_graph_node(db, state, to_item_id, &to_index);
        if (result.code != CORE_OK) {
            if (is_graph_node_limit_result(result)) {
                continue;
            }
            goto cleanup;
        }
        if (!mem_console_graph_node_kind_is_enabled(state, state->graph_nodes[from_index].kind) ||
            !mem_console_graph_node_kind_is_enabled(state, state->graph_nodes[to_index].kind)) {
            continue;
        }

        state->graph_edges[state->graph_edge_count].from_index = from_index;
        state->graph_edges[state->graph_edge_count].to_index = to_index;
        copy_core_str(kind,
                      state->graph_edges[state->graph_edge_count].kind,
                      sizeof(state->graph_edges[state->graph_edge_count].kind));
        state->graph_edge_count += 1;
    }

    compact_graph_by_node_kind(state);
    apply_graph_node_sort(state);

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

CoreResult refresh_state_from_db(CoreMemDb *db, MemConsoleState *state) {
    CoreResult result;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }

    result = read_schema_version(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = read_active_count(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = read_project_filter_options(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = read_matching_count(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    clamp_list_query_offset(state);

    result = read_visible_items(db, state);
    if (result.code != CORE_OK) {
        return result;
    }
    if (state->matching_count > 0 && state->visible_count == 0 && state->list_query_offset > 0) {
        state->list_query_offset = 0;
        state->visible_start_index = 0;
        result = read_visible_items(db, state);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    if (state->matching_count == 0) {
        state->selected_item_id = 0;
        state->graph_center_item_id = 0;
        state->list_query_offset = 0;
        state->visible_start_index = 0;
        state->list_scroll = 0.0f;
        set_default_detail(state);
    } else if (state->selected_item_id == 0 && state->visible_count > 0) {
        state->selected_item_id = state->visible_items[0].id;
    }

    if (state->graph_center_item_id == 0 && state->selected_item_id != 0) {
        state->graph_center_item_id = state->selected_item_id;
    }

    result = read_selected_detail(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    result = load_graph_neighborhood(db, state);
    if (result.code != CORE_OK) {
        return result;
    }

    if (state->selected_project_count > 0) {
        (void)snprintf(state->project_filter_summary_line,
                       sizeof(state->project_filter_summary_line),
                       "Projects: %d selected",
                       state->selected_project_count);
    } else if (state->project_filter_option_count > 0) {
        (void)snprintf(state->project_filter_summary_line,
                       sizeof(state->project_filter_summary_line),
                       "Projects: all (%d options)",
                       state->project_filter_option_count);
    } else {
        (void)snprintf(state->project_filter_summary_line,
                       sizeof(state->project_filter_summary_line),
                       "Projects: none");
    }

    if (state->search_text[0] != '\0') {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Active %lld | Match %lld | Proj %d | %s",
                       (long long)state->active_count,
                       (long long)state->matching_count,
                       state->selected_project_count,
                       state->search_text);
    } else {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Active %lld | Match %lld | Proj %d",
                       (long long)state->active_count,
                       (long long)state->matching_count,
                       state->selected_project_count);
    }

    return core_result_ok();
}
