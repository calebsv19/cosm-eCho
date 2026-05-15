#include "mem_console_db.h"
#include "mem_console_db_internal.h"
#include "mem_console_db_graph_sort.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
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
    out_node->render_anchor_item_id = item_id;

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
    out_node->render_anchor_created_ns = out_node->created_ns;
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

static int64_t graph_priority_root_item_id(const MemConsoleState *state) {
    if (!state) {
        return 0;
    }
    if (state->selected_item_id != 0) {
        return state->selected_item_id;
    }
    return state->graph_center_item_id;
}

static CoreResult load_priority_graph_nodes(CoreMemDb *db,
                                            MemConsoleState *state,
                                            int64_t root_item_id,
                                            int reserve_hops,
                                            int sort_oldest_first) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid priority graph node load request" };
    }
    if (root_item_id == 0 || reserve_hops <= 0) {
        return core_result_ok();
    }

    if (state->graph_node_count < MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        int node_index = -1;
        result = ensure_graph_node(db, state, root_item_id, &node_index);
        if (result.code != CORE_OK && !is_graph_node_limit_result(result)) {
            return result;
        }
    }

    result = core_memdb_prepare(db,
                                sort_oldest_first
                                    ? "WITH RECURSIVE walk(node_id, depth) AS ("
                                      "  SELECT ?1, 0 "
                                      "  UNION "
                                      "  SELECT CASE WHEN l.from_item_id = walk.node_id THEN l.to_item_id ELSE l.from_item_id END, "
                                      "         walk.depth + 1 "
                                      "  FROM walk "
                                      "  JOIN mem_link l ON (l.from_item_id = walk.node_id OR l.to_item_id = walk.node_id) "
                                      "  JOIN mem_item src ON src.id = l.from_item_id AND src.archived_ns IS NULL "
                                      "                    AND (?4 = 0 OR src.project_key IN (?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20)) "
                                      "  JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                      "                    AND (?4 = 0 OR dst.project_key IN (?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20)) "
                                      "  WHERE walk.depth < ?2 "
                                      "    AND (?3 = '' OR l.kind = ?3)"
                                      "), prioritized AS ("
                                      "  SELECT node_id, MIN(depth) AS min_depth "
                                      "  FROM walk "
                                      "  GROUP BY node_id"
                                      ") "
                                      "SELECT i.id, i.kind "
                                      "FROM prioritized p "
                                      "JOIN mem_item i ON i.id = p.node_id "
                                      "WHERE i.archived_ns IS NULL "
                                      "ORDER BY p.min_depth ASC, i.updated_ns ASC, i.id ASC;"
                                    : "WITH RECURSIVE walk(node_id, depth) AS ("
                                      "  SELECT ?1, 0 "
                                      "  UNION "
                                      "  SELECT CASE WHEN l.from_item_id = walk.node_id THEN l.to_item_id ELSE l.from_item_id END, "
                                      "         walk.depth + 1 "
                                      "  FROM walk "
                                      "  JOIN mem_link l ON (l.from_item_id = walk.node_id OR l.to_item_id = walk.node_id) "
                                      "  JOIN mem_item src ON src.id = l.from_item_id AND src.archived_ns IS NULL "
                                      "                    AND (?4 = 0 OR src.project_key IN (?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20)) "
                                      "  JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                      "                    AND (?4 = 0 OR dst.project_key IN (?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20)) "
                                      "  WHERE walk.depth < ?2 "
                                      "    AND (?3 = '' OR l.kind = ?3)"
                                      "), prioritized AS ("
                                      "  SELECT node_id, MIN(depth) AS min_depth "
                                      "  FROM walk "
                                      "  GROUP BY node_id"
                                      ") "
                                      "SELECT i.id, i.kind "
                                      "FROM prioritized p "
                                      "JOIN mem_item i ON i.id = p.node_id "
                                      "WHERE i.archived_ns IS NULL "
                                      "ORDER BY p.min_depth ASC, i.updated_ns DESC, i.id DESC;",
                                &stmt);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_stmt_bind_i64(&stmt, 1, root_item_id);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 2, reserve_hops);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_text(&stmt, 3, state->graph_kind_filter);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = bind_project_filters(&stmt, 4, state);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    for (;;) {
        int64_t item_id = 0;
        CoreStr kind = {0};
        char node_kind[32];
        int node_index = -1;

        if (state->graph_node_count >= MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            break;
        }

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
        if (result.code != CORE_OK) {
            if (is_graph_node_limit_result(result)) {
                break;
            }
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

static CoreResult load_full_scope_graph_nodes(CoreMemDb *db,
                                              MemConsoleState *state,
                                              int sort_oldest_first) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid full scope node load request" };
    }

    if (sort_oldest_first) {
        result = core_memdb_prepare(db,
                                    "SELECT i.id, i.kind "
                                    "FROM mem_item i "
                                    "WHERE i.archived_ns IS NULL "
                                    "  AND (?1 = 0 OR i.project_key IN (?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17)) "
                                    "ORDER BY i.updated_ns ASC, i.id ASC;",
                                    &stmt);
    } else {
        result = core_memdb_prepare(db,
                                    "SELECT i.id, i.kind "
                                    "FROM mem_item i "
                                    "WHERE i.archived_ns IS NULL "
                                    "  AND (?1 = 0 OR i.project_key IN (?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17)) "
                                    "ORDER BY i.updated_ns DESC, i.id DESC;",
                                    &stmt);
    }
    if (result.code != CORE_OK) {
        result = core_memdb_prepare(db,
                                    sort_oldest_first
                                        ? "SELECT i.id, '' "
                                          "FROM mem_item i "
                                          "WHERE i.archived_ns IS NULL "
                                          "  AND (?1 = 0 OR i.project_key IN (?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17)) "
                                          "ORDER BY i.updated_ns ASC, i.id ASC;"
                                        : "SELECT i.id, '' "
                                          "FROM mem_item i "
                                          "WHERE i.archived_ns IS NULL "
                                          "  AND (?1 = 0 OR i.project_key IN (?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17)) "
                                          "ORDER BY i.updated_ns DESC, i.id DESC;",
                                    &stmt);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    result = bind_project_filters(&stmt, 1, state);
    if (result.code != CORE_OK) {
        goto cleanup;
    }

    for (;;) {
        int64_t item_id = 0;
        CoreStr kind = {0};
        char node_kind[32];
        int node_index = -1;

        if (state->graph_node_count >= MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            break;
        }

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
        if (result.code != CORE_OK) {
            if (is_graph_node_limit_result(result)) {
                break;
            }
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

static CoreResult load_full_scope_graph_edges(CoreMemDb *db,
                                              MemConsoleState *state,
                                              int sort_oldest_first,
                                              int query_edge_limit) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid full scope edge load request" };
    }

    if (sort_oldest_first) {
        result = core_memdb_prepare(db,
                                    "SELECT l.from_item_id, l.to_item_id, l.kind "
                                    "FROM mem_link l "
                                    "JOIN mem_item src ON src.id = l.from_item_id AND src.archived_ns IS NULL "
                                    "                  AND (?3 = 0 OR src.project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19)) "
                                    "JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                    "                  AND (?3 = 0 OR dst.project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19)) "
                                    "WHERE (?1 = '' OR l.kind = ?1) "
                                    "ORDER BY l.id ASC "
                                    "LIMIT ?2;",
                                    &stmt);
    } else {
        result = core_memdb_prepare(db,
                                    "SELECT l.from_item_id, l.to_item_id, l.kind "
                                    "FROM mem_link l "
                                    "JOIN mem_item src ON src.id = l.from_item_id AND src.archived_ns IS NULL "
                                    "                  AND (?3 = 0 OR src.project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19)) "
                                    "JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                    "                  AND (?3 = 0 OR dst.project_key IN (?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19)) "
                                    "WHERE (?1 = '' OR l.kind = ?1) "
                                    "ORDER BY l.id DESC "
                                    "LIMIT ?2;",
                                    &stmt);
    }
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_memdb_stmt_bind_text(&stmt, 1, state->graph_kind_filter);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    result = core_memdb_stmt_bind_i64(&stmt, 2, query_edge_limit);
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

        if (state->graph_edge_count >= MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
            break;
        }

        result = core_memdb_stmt_step(&stmt, &has_row);
        if (result.code != CORE_OK) {
            goto cleanup;
        }
        if (!has_row) {
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

        from_index = find_graph_node_index(state, from_item_id);
        to_index = find_graph_node_index(state, to_item_id);
        if (from_index < 0 || to_index < 0) {
            continue;
        }

        copy_core_str(kind, edge_kind, sizeof(edge_kind));
        normalize_ascii_in_place(edge_kind);
        if (!mem_console_graph_kind_is_enabled(state, edge_kind)) {
            continue;
        }

        state->graph_edges[state->graph_edge_count].from_index = from_index;
        state->graph_edges[state->graph_edge_count].to_index = to_index;
        (void)snprintf(state->graph_edges[state->graph_edge_count].kind,
                       sizeof(state->graph_edges[state->graph_edge_count].kind),
                       "%s",
                       edge_kind);
        state->graph_edge_count += 1;
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
    int query_edge_limit = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    int graph_hops = MEM_CONSOLE_GRAPH_HOPS_MIN;
    int reserve_hops = 0;
    int use_full_scope = 0;
    int sort_oldest_first = 0;
    int64_t center_item_id = 0;
    int64_t priority_root_item_id = 0;

    if (!db || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph neighborhood request" };
    }

    edge_limit = mem_console_graph_edge_limit_clamp(state->graph_query_edge_limit);
    state->graph_query_edge_limit = edge_limit;
    query_edge_limit = edge_limit * 3;
    if (query_edge_limit < edge_limit) {
        query_edge_limit = edge_limit;
    }
    if (query_edge_limit > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        query_edge_limit = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }
    graph_hops = mem_console_graph_hops_clamp(state->graph_query_hops);
    state->graph_query_hops = graph_hops;
    reserve_hops = graph_hops;
    if (reserve_hops > 2) {
        reserve_hops = 2;
    }
    use_full_scope = state->graph_scope_full_mode_enabled ? 1 : 0;
    sort_oldest_first = state->graph_sort_mode == MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST ? 1 : 0;
    priority_root_item_id = graph_priority_root_item_id(state);

    state->graph_node_count = 0;
    state->graph_edge_count = 0;

    if (use_full_scope) {
        result = load_priority_graph_nodes(db,
                                           state,
                                           priority_root_item_id,
                                           reserve_hops,
                                           sort_oldest_first);
        if (result.code != CORE_OK) {
            return result;
        }
        result = load_full_scope_graph_nodes(db, state, sort_oldest_first);
        if (result.code != CORE_OK) {
            return result;
        }
        result = load_full_scope_graph_edges(db, state, sort_oldest_first, query_edge_limit);
        if (result.code != CORE_OK) {
            return result;
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

        mem_console_db_compact_graph_by_node_kind(state);
        mem_console_db_annotate_rollup_render_anchors(state);
        mem_console_db_apply_graph_node_sort(state);
        mem_console_db_apply_graph_edge_priority(state, edge_limit);

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

    if (sort_oldest_first) {
        result = core_memdb_prepare(db,
                                    "WITH RECURSIVE walk(node_id, depth) AS ("
                                    "  SELECT ?1, 0 "
                                    "  UNION "
                                    "  SELECT CASE WHEN l.from_item_id = walk.node_id THEN l.to_item_id ELSE l.from_item_id END, "
                                    "         walk.depth + 1 "
                                    "  FROM walk "
                                    "  JOIN mem_link l ON (l.from_item_id = walk.node_id OR l.to_item_id = walk.node_id) "
                                    "  JOIN mem_item src ON src.id = l.from_item_id AND src.archived_ns IS NULL "
                                    "                    AND (?5 = 0 OR src.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)) "
                                    "  JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                    "                    AND (?5 = 0 OR dst.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)) "
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
                                    "                    AND (?5 = 0 OR src.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)) "
                                    "  JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                    "                    AND (?5 = 0 OR dst.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)) "
                                    "  WHERE (?3 = '' OR l.kind = ?3) "
                                    "  ORDER BY l.id ASC "
                                    "  LIMIT ?4"
                                    ") "
                                    "SELECT from_item_id, to_item_id, kind "
                                    "FROM edges "
                                    "ORDER BY id ASC;",
                                    &stmt);
    } else {
        result = core_memdb_prepare(db,
                                    "WITH RECURSIVE walk(node_id, depth) AS ("
                                    "  SELECT ?1, 0 "
                                    "  UNION "
                                    "  SELECT CASE WHEN l.from_item_id = walk.node_id THEN l.to_item_id ELSE l.from_item_id END, "
                                    "         walk.depth + 1 "
                                    "  FROM walk "
                                    "  JOIN mem_link l ON (l.from_item_id = walk.node_id OR l.to_item_id = walk.node_id) "
                                    "  JOIN mem_item src ON src.id = l.from_item_id AND src.archived_ns IS NULL "
                                    "                    AND (?5 = 0 OR src.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)) "
                                    "  JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                    "                    AND (?5 = 0 OR dst.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)) "
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
                                    "                    AND (?5 = 0 OR src.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)) "
                                    "  JOIN mem_item dst ON dst.id = l.to_item_id AND dst.archived_ns IS NULL "
                                    "                    AND (?5 = 0 OR dst.project_key IN (?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)) "
                                    "  WHERE (?3 = '' OR l.kind = ?3) "
                                    "  ORDER BY l.id DESC "
                                    "  LIMIT ?4"
                                    ") "
                                    "SELECT from_item_id, to_item_id, kind "
                                    "FROM edges "
                                    "ORDER BY id DESC;",
                                    &stmt);
    }
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
    result = core_memdb_stmt_bind_i64(&stmt, 4, query_edge_limit);
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

    mem_console_db_compact_graph_by_node_kind(state);
    mem_console_db_annotate_rollup_render_anchors(state);
    mem_console_db_apply_graph_node_sort(state);
    mem_console_db_apply_graph_edge_priority(state, edge_limit);

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
