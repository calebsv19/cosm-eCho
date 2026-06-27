#include "runtime/mem_console_runtime_internal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int mem_console_browse_kind_index_clamp(int index) {
    if (index < 0 || index >= MEM_CONSOLE_BROWSE_KIND_COUNT) {
        return 0;
    }
    return index;
}

uint32_t mem_console_graph_kind_filter_all_mask(void) {
    return 0xffu;
}

uint32_t mem_console_graph_node_kind_filter_all_mask(void) {
    return 0x7fu;
}

int mem_console_graph_edge_limit_clamp(int value) {
    if (value < MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN) {
        return MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN;
    }
    if (value > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        return MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }
    return value;
}

void mem_console_graph_edge_limit_set(MemConsoleState *state, int value) {
    if (!state) {
        return;
    }
    state->graph_query_edge_limit = mem_console_graph_edge_limit_clamp(value);
}

int mem_console_graph_hops_clamp(int value) {
    if (value < MEM_CONSOLE_GRAPH_HOPS_MIN) {
        return MEM_CONSOLE_GRAPH_HOPS_MIN;
    }
    if (value > MEM_CONSOLE_GRAPH_HOPS_MAX) {
        return MEM_CONSOLE_GRAPH_HOPS_MAX;
    }
    return value;
}

int mem_console_graph_layout_mode_clamp(int value) {
    return value == MEM_CONSOLE_GRAPH_LAYOUT_TREE ? MEM_CONSOLE_GRAPH_LAYOUT_TREE : MEM_CONSOLE_GRAPH_LAYOUT_DAG;
}

int mem_console_graph_sort_mode_clamp(int value) {
    return value == MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST ? MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST : MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST;
}

void mem_console_graph_kind_sync_text_filter(MemConsoleState *state) {
    if (!state) {
        return;
    }
    if (state->graph_kind_filter_all_override) {
        state->graph_kind_filter[0] = '\0';
    }
}

void mem_console_selection_set(MemConsoleState *state, int64_t item_id) {
    if (state) {
        state->selected_item_id = item_id;
    }
}

void mem_console_graph_center_set(MemConsoleState *state, int64_t item_id) {
    if (state) {
        state->graph_center_item_id = item_id;
    }
}

void mem_console_selection_apply_refreshed(MemConsoleState *state, const MemConsoleState *refreshed) {
    if (!state || !refreshed) {
        return;
    }
    state->selected_item_id = refreshed->selected_item_id;
    state->graph_center_item_id = refreshed->graph_center_item_id;
}

void seed_state(MemConsoleState *state, const char *db_path) {
    if (!state) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->db_path = db_path;
}

CoreResult refresh_state_from_db(CoreMemDb *db, MemConsoleState *state) {
    (void)db;
    (void)state;
    return core_result_ok();
}

CoreResult core_memdb_open(const char *path, CoreMemDb *out_db) {
    (void)path;
    if (out_db) {
        out_db->native_db = 0;
    }
    return core_result_ok();
}

CoreResult core_memdb_close(CoreMemDb *db) {
    (void)db;
    return core_result_ok();
}

bool core_wake_signal(CoreWake *wake) {
    (void)wake;
    return true;
}

bool core_workers_submit(CoreWorkers *workers, CoreWorkerTaskFn fn, void *task_ctx) {
    (void)workers;
    (void)fn;
    (void)task_ctx;
    return false;
}

static void seed_intent_state(MemConsoleState *state) {
    memset(state, 0, sizeof(*state));
    (void)snprintf(state->search_text, sizeof(state->search_text), "alpha");
    state->selected_item_id = 42;
    state->graph_center_item_id = 77;
    state->list_query_offset = 5;
    state->browse_pinned_only = 1;
    state->browse_canonical_only = 0;
    state->browse_kind_index = 2;
    state->selected_project_count = 2;
    (void)snprintf(state->selected_project_keys[0], sizeof(state->selected_project_keys[0]), "memory_console");
    (void)snprintf(state->selected_project_keys[1], sizeof(state->selected_project_keys[1]), "shared");
    (void)snprintf(state->graph_kind_filter, sizeof(state->graph_kind_filter), "supports");
    state->graph_kind_filter_mask = 0x03u;
    state->graph_kind_filter_all_override = 0;
    state->graph_query_edge_limit = MEM_CONSOLE_GRAPH_EDGE_LIMIT_DEFAULT;
    state->graph_query_hops = 3;
    state->graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_TREE;
    state->graph_sort_mode = MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST;
    state->graph_scope_full_mode_enabled = 1;
    state->graph_node_kind_filter_mask = 0x05u;
    state->graph_node_kind_filter_all_override = 0;
}

static int captured_matches_state(const MemConsoleState *state,
                                  const char *search_text,
                                  int64_t selected_item_id,
                                  int64_t graph_center_item_id,
                                  int list_query_offset,
                                  int browse_pinned_only,
                                  int browse_canonical_only,
                                  int browse_kind_index,
                                  const char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
                                  int selected_project_count,
                                  const char *graph_kind_filter,
                                  uint32_t graph_kind_filter_mask,
                                  int graph_kind_filter_all_override,
                                  int graph_edge_limit,
                                  int graph_hops,
                                  int graph_layout_mode,
                                  int graph_sort_mode,
                                  int graph_scope_full_mode_enabled,
                                  uint32_t graph_node_kind_filter_mask,
                                  int graph_node_kind_filter_all_override) {
    return mem_console_runtime_intent_matches(search_text,
                                              selected_item_id,
                                              graph_center_item_id,
                                              list_query_offset,
                                              browse_pinned_only,
                                              browse_canonical_only,
                                              browse_kind_index,
                                              selected_project_keys,
                                              selected_project_count,
                                              graph_kind_filter,
                                              graph_kind_filter_mask,
                                              graph_kind_filter_all_override,
                                              graph_edge_limit,
                                              graph_hops,
                                              graph_layout_mode,
                                              graph_sort_mode,
                                              graph_scope_full_mode_enabled,
                                              graph_node_kind_filter_mask,
                                              graph_node_kind_filter_all_override,
                                              state->search_text,
                                              state->selected_item_id,
                                              state->graph_center_item_id,
                                              state->list_query_offset,
                                              state->browse_pinned_only,
                                              state->browse_canonical_only,
                                              state->browse_kind_index,
                                              state->selected_project_keys,
                                              state->selected_project_count,
                                              state->graph_kind_filter,
                                              state->graph_kind_filter_mask,
                                              state->graph_kind_filter_all_override,
                                              state->graph_query_edge_limit,
                                              state->graph_query_hops,
                                              state->graph_layout_mode,
                                              state->graph_sort_mode,
                                              state->graph_scope_full_mode_enabled,
                                              state->graph_node_kind_filter_mask,
                                              state->graph_node_kind_filter_all_override);
}

static void capture_intent(const MemConsoleState *state,
                           char *search_text,
                           int64_t *selected_item_id,
                           int64_t *graph_center_item_id,
                           int *list_query_offset,
                           int *browse_pinned_only,
                           int *browse_canonical_only,
                           int *browse_kind_index,
                           char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
                           int *selected_project_count,
                           char *graph_kind_filter,
                           uint32_t *graph_kind_filter_mask,
                           int *graph_kind_filter_all_override,
                           int *graph_edge_limit,
                           int *graph_hops,
                           int *graph_layout_mode,
                           int *graph_sort_mode,
                           int *graph_scope_full_mode_enabled,
                           uint32_t *graph_node_kind_filter_mask,
                           int *graph_node_kind_filter_all_override) {
    mem_console_runtime_capture_intent_from_state(state,
                                                  search_text,
                                                  256,
                                                  selected_item_id,
                                                  graph_center_item_id,
                                                  list_query_offset,
                                                  browse_pinned_only,
                                                  browse_canonical_only,
                                                  browse_kind_index,
                                                  selected_project_keys,
                                                  selected_project_count,
                                                  graph_kind_filter,
                                                  32,
                                                  graph_kind_filter_mask,
                                                  graph_kind_filter_all_override,
                                                  graph_edge_limit,
                                                  graph_hops,
                                                  graph_layout_mode,
                                                  graph_sort_mode,
                                                  graph_scope_full_mode_enabled,
                                                  graph_node_kind_filter_mask,
                                                  graph_node_kind_filter_all_override);
}

static void test_capture_and_match_browse_intent(void) {
    MemConsoleState state;
    char search_text[256];
    int64_t selected_item_id = 0;
    int64_t graph_center_item_id = 0;
    int list_query_offset = 0;
    int browse_pinned_only = 0;
    int browse_canonical_only = 0;
    int browse_kind_index = 0;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    int selected_project_count = 0;
    char graph_kind_filter[32];
    uint32_t graph_kind_filter_mask = 0u;
    int graph_kind_filter_all_override = 0;
    int graph_edge_limit = 0;
    int graph_hops = 0;
    int graph_layout_mode = 0;
    int graph_sort_mode = 0;
    int graph_scope_full_mode_enabled = 0;
    uint32_t graph_node_kind_filter_mask = 0u;
    int graph_node_kind_filter_all_override = 0;

    seed_intent_state(&state);
    capture_intent(&state,
                   search_text,
                   &selected_item_id,
                   &graph_center_item_id,
                   &list_query_offset,
                   &browse_pinned_only,
                   &browse_canonical_only,
                   &browse_kind_index,
                   selected_project_keys,
                   &selected_project_count,
                   graph_kind_filter,
                   &graph_kind_filter_mask,
                   &graph_kind_filter_all_override,
                   &graph_edge_limit,
                   &graph_hops,
                   &graph_layout_mode,
                   &graph_sort_mode,
                   &graph_scope_full_mode_enabled,
                   &graph_node_kind_filter_mask,
                   &graph_node_kind_filter_all_override);

    assert(strcmp(search_text, "alpha") == 0);
    assert(selected_item_id == 42);
    assert(graph_center_item_id == 77);
    assert(list_query_offset == 5);
    assert(browse_pinned_only == 1);
    assert(browse_canonical_only == 0);
    assert(browse_kind_index == 2);
    assert(selected_project_count == 2);
    assert(strcmp(selected_project_keys[0], "memory_console") == 0);
    assert(strcmp(selected_project_keys[1], "shared") == 0);
    assert(captured_matches_state(&state,
                                  search_text,
                                  selected_item_id,
                                  graph_center_item_id,
                                  list_query_offset,
                                  browse_pinned_only,
                                  browse_canonical_only,
                                  browse_kind_index,
                                  selected_project_keys,
                                  selected_project_count,
                                  graph_kind_filter,
                                  graph_kind_filter_mask,
                                  graph_kind_filter_all_override,
                                  graph_edge_limit,
                                  graph_hops,
                                  graph_layout_mode,
                                  graph_sort_mode,
                                  graph_scope_full_mode_enabled,
                                  graph_node_kind_filter_mask,
                                  graph_node_kind_filter_all_override));

    state.browse_pinned_only = 0;
    assert(!captured_matches_state(&state,
                                   search_text,
                                   selected_item_id,
                                   graph_center_item_id,
                                   list_query_offset,
                                   browse_pinned_only,
                                   browse_canonical_only,
                                   browse_kind_index,
                                   selected_project_keys,
                                   selected_project_count,
                                   graph_kind_filter,
                                   graph_kind_filter_mask,
                                   graph_kind_filter_all_override,
                                   graph_edge_limit,
                                   graph_hops,
                                   graph_layout_mode,
                                   graph_sort_mode,
                                   graph_scope_full_mode_enabled,
                                   graph_node_kind_filter_mask,
                                   graph_node_kind_filter_all_override));

    state.browse_pinned_only = 1;
    state.browse_canonical_only = 1;
    assert(!captured_matches_state(&state,
                                   search_text,
                                   selected_item_id,
                                   graph_center_item_id,
                                   list_query_offset,
                                   browse_pinned_only,
                                   browse_canonical_only,
                                   browse_kind_index,
                                   selected_project_keys,
                                   selected_project_count,
                                   graph_kind_filter,
                                   graph_kind_filter_mask,
                                   graph_kind_filter_all_override,
                                   graph_edge_limit,
                                   graph_hops,
                                   graph_layout_mode,
                                   graph_sort_mode,
                                   graph_scope_full_mode_enabled,
                                   graph_node_kind_filter_mask,
                                   graph_node_kind_filter_all_override));

    state.browse_canonical_only = 0;
    state.browse_kind_index = 3;
    assert(!captured_matches_state(&state,
                                   search_text,
                                   selected_item_id,
                                   graph_center_item_id,
                                   list_query_offset,
                                   browse_pinned_only,
                                   browse_canonical_only,
                                   browse_kind_index,
                                   selected_project_keys,
                                   selected_project_count,
                                   graph_kind_filter,
                                   graph_kind_filter_mask,
                                   graph_kind_filter_all_override,
                                   graph_edge_limit,
                                   graph_hops,
                                   graph_layout_mode,
                                   graph_sort_mode,
                                   graph_scope_full_mode_enabled,
                                   graph_node_kind_filter_mask,
                                   graph_node_kind_filter_all_override));
}

static void test_refreshed_state_applies_browse_intent(void) {
    MemConsoleState current;
    MemConsoleState refreshed;

    memset(&current, 0, sizeof(current));
    memset(&refreshed, 0, sizeof(refreshed));
    current.browse_pinned_only = 0;
    current.browse_canonical_only = 0;
    current.browse_kind_index = 0;
    current.graph_kind_filter_all_override = 1;
    current.graph_node_kind_filter_all_override = 1;
    refreshed.browse_pinned_only = 1;
    refreshed.browse_canonical_only = 1;
    refreshed.browse_kind_index = 999;
    refreshed.visible_start_index = 4;
    refreshed.visible_count = 1;
    refreshed.selected_project_count = 1;
    (void)snprintf(refreshed.selected_project_keys[0], sizeof(refreshed.selected_project_keys[0]), "memory_console");
    refreshed.graph_query_edge_limit = MEM_CONSOLE_GRAPH_EDGE_LIMIT + 100;
    refreshed.graph_query_hops = MEM_CONSOLE_GRAPH_HOPS_MAX + 10;
    refreshed.graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_TREE;
    refreshed.graph_scope_full_mode_enabled = 1;

    mem_console_runtime_apply_refreshed_state(&current, &refreshed);

    assert(current.browse_pinned_only == 1);
    assert(current.browse_canonical_only == 1);
    assert(current.browse_kind_index == 0);
    assert(current.visible_start_index == 4);
    assert(current.visible_count == 1);
    assert(current.selected_project_count == 1);
    assert(strcmp(current.selected_project_keys[0], "memory_console") == 0);
    assert(current.graph_query_edge_limit == MEM_CONSOLE_GRAPH_EDGE_LIMIT);
    assert(current.graph_query_hops == MEM_CONSOLE_GRAPH_HOPS_MAX);
    assert(current.graph_scope_full_mode_enabled == 1);
}

int main(void) {
    test_capture_and_match_browse_intent();
    test_refreshed_state_applies_browse_intent();
    puts("mem_console_runtime_refresh_intent_test: success");
    return 0;
}
