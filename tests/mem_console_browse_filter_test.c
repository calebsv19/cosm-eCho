#include "db/mem_console_db_internal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *kTestDbPath = "build/targets/macOS-arm64/tests/mem_console_browse_filter_test.sqlite";

void copy_core_str(CoreStr value, char *out_text, size_t out_cap) {
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

void build_like_pattern(const char *search_text, char *out_pattern, size_t out_cap) {
    if (!out_pattern || out_cap == 0u) {
        return;
    }
    if (!search_text || search_text[0] == '\0') {
        out_pattern[0] = '\0';
        return;
    }
    (void)snprintf(out_pattern, out_cap, "%%%s%%", search_text);
}

int mem_console_browse_kind_index_clamp(int index) {
    if (index < 0 || index >= MEM_CONSOLE_BROWSE_KIND_COUNT) {
        return 0;
    }
    return index;
}

const char *mem_console_browse_kind_for_index(int index) {
    static const char *k_browse_kind_order[MEM_CONSOLE_BROWSE_KIND_COUNT] = {
        "",
        "plan",
        "decision",
        "issue",
        "scope",
        "summary",
        "policy",
        "runtime"
    };

    return k_browse_kind_order[mem_console_browse_kind_index_clamp(index)];
}

void mem_console_project_filter_prune_to_options(MemConsoleState *state) {
    (void)state;
}

void set_default_detail(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->selected_item_id = 0;
    (void)snprintf(state->selected_title, sizeof(state->selected_title), "No Matching Memory");
    (void)snprintf(state->selected_body, sizeof(state->selected_body), "No matching memory.");
}

static void seed_items(CoreMemDb *db) {
    CoreResult result = core_memdb_exec(
        db,
        "INSERT INTO mem_item (id, stable_id, title, body, fingerprint, workspace_key, project_key, kind, created_ns, updated_ns, pinned, canonical, ttl_until_ns, archived_ns) "
        "VALUES "
        "(1, 'one', 'One Plan', 'alpha body', 'f1', 'codework', 'memory_console', 'plan', 1, 100, 1, 0, NULL, NULL),"
        "(2, 'two', 'Two Decision', 'bravo body', 'f2', 'codework', 'memory_console', 'decision', 2, 90, 0, 1, NULL, NULL),"
        "(3, 'three', 'Three Issue', 'charlie body', 'f3', 'codework', 'memory_console', 'issue', 3, 80, 1, 1, NULL, NULL),"
        "(4, 'four', 'Four Summary', 'delta body', 'f4', 'codework', 'memory_console', 'summary', 4, 70, 0, 0, NULL, NULL),"
        "(5, 'archived', 'Archived Plan', 'archived body', 'f5', 'codework', 'memory_console', 'plan', 5, 200, 1, 1, NULL, 201),"
        "(6, 'six', 'Six Runtime', 'echo body', 'f6', 'codework', 'memory_console', 'runtime', 6, 60, 0, 0, NULL, NULL);");
    assert(result.code == CORE_OK);
}

static void refresh_list(CoreMemDb *db, MemConsoleState *state) {
    CoreResult result = read_matching_count(db, state);
    assert(result.code == CORE_OK);
    result = read_visible_items(db, state);
    assert(result.code == CORE_OK);
}

static void expect_ids(const MemConsoleState *state,
                       const int64_t *expected_ids,
                       int expected_count) {
    int i;

    assert(state->visible_count == expected_count);
    for (i = 0; i < expected_count; ++i) {
        assert(state->visible_items[i].id == expected_ids[i]);
    }
}

static void test_unfiltered_window(CoreMemDb *db) {
    MemConsoleState state;
    const int64_t expected_ids[] = {1, 3, 2, 4, 6};

    memset(&state, 0, sizeof(state));
    refresh_list(db, &state);
    assert(state.matching_count == 5);
    assert(state.visible_start_index == 0);
    expect_ids(&state, expected_ids, 5);
}

static void test_pinned_only(CoreMemDb *db) {
    MemConsoleState state;
    const int64_t expected_ids[] = {1, 3};

    memset(&state, 0, sizeof(state));
    state.browse_pinned_only = 1;
    refresh_list(db, &state);
    assert(state.matching_count == 2);
    expect_ids(&state, expected_ids, 2);
    assert(state.visible_items[0].pinned == 1);
    assert(state.visible_items[1].pinned == 1);
}

static void test_canonical_only(CoreMemDb *db) {
    MemConsoleState state;
    const int64_t expected_ids[] = {3, 2};

    memset(&state, 0, sizeof(state));
    state.browse_canonical_only = 1;
    refresh_list(db, &state);
    assert(state.matching_count == 2);
    expect_ids(&state, expected_ids, 2);
    assert(state.visible_items[0].canonical == 1);
    assert(state.visible_items[1].canonical == 1);
}

static void test_kind_cycle_mapping(CoreMemDb *db) {
    MemConsoleState state;
    const int64_t plan_ids[] = {1};
    const int64_t decision_ids[] = {2};

    memset(&state, 0, sizeof(state));
    state.browse_kind_index = 1;
    refresh_list(db, &state);
    assert(state.matching_count == 1);
    expect_ids(&state, plan_ids, 1);
    assert(strcmp(state.visible_items[0].kind, "plan") == 0);

    memset(&state, 0, sizeof(state));
    state.browse_kind_index = 2;
    refresh_list(db, &state);
    assert(state.matching_count == 1);
    expect_ids(&state, decision_ids, 1);
    assert(strcmp(state.visible_items[0].kind, "decision") == 0);
}

static void test_pagination_offset(CoreMemDb *db) {
    MemConsoleState state;
    const int64_t expected_ids[] = {2, 4, 6};

    memset(&state, 0, sizeof(state));
    state.list_query_offset = 2;
    refresh_list(db, &state);
    assert(state.matching_count == 5);
    assert(state.visible_start_index == 2);
    expect_ids(&state, expected_ids, 3);
}

int main(void) {
    CoreMemDb db = {0};
    CoreResult result;

    (void)unlink(kTestDbPath);
    result = core_memdb_open(kTestDbPath, &db);
    if (result.code != CORE_OK) {
        fprintf(stderr, "core_memdb_open failed: %d (%s)\n", (int)result.code, result.message ? result.message : "");
    }
    assert(result.code == CORE_OK);
    seed_items(&db);

    test_unfiltered_window(&db);
    test_pinned_only(&db);
    test_canonical_only(&db);
    test_kind_cycle_mapping(&db);
    test_pagination_offset(&db);

    result = core_memdb_close(&db);
    assert(result.code == CORE_OK);
    (void)unlink(kTestDbPath);

    printf("mem_console_browse_filter_test: success\n");
    return 0;
}
