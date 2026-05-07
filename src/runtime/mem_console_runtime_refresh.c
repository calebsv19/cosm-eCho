#include "mem_console_runtime_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_base.h"
#include "mem_console_db.h"

typedef struct MemConsoleRefreshTask {
    char db_path[1024];
    char search_text[256];
    int64_t selected_item_id;
    int64_t graph_center_item_id;
    int list_query_offset;
    int selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    uint32_t graph_kind_filter_mask;
    int graph_kind_filter_all_override;
    int graph_edge_limit;
    int graph_hops;
    int graph_layout_mode;
    int graph_sort_mode;
    int graph_scope_full_mode_enabled;
    uint32_t graph_node_kind_filter_mask;
    int graph_node_kind_filter_all_override;
    uint64_t request_id;
    CoreWake *wake;
} MemConsoleRefreshTask;

MemConsoleRuntimeMetricsSnapshot
mem_console_runtime_metrics_snapshot_capture(const MemConsoleRuntime *runtime) {
    MemConsoleRuntimeMetricsSnapshot snapshot;

    memset(&snapshot, 0, sizeof(snapshot));
    if (!runtime) {
        return snapshot;
    }

    snapshot.refresh_submitted = runtime->stats_refresh_submitted;
    snapshot.refresh_applied = runtime->stats_refresh_applied;
    snapshot.refresh_dropped_stale = runtime->stats_refresh_dropped_stale;
    snapshot.refresh_dropped_mismatch = runtime->stats_refresh_dropped_mismatch;
    snapshot.refresh_dropped_editing = runtime->stats_refresh_dropped_editing;
    snapshot.refresh_errors = runtime->stats_refresh_errors;
    snapshot.refresh_coalesced = runtime->stats_refresh_coalesced;
    snapshot.refresh_in_flight = runtime->refresh_in_flight;
    snapshot.pending_intent_valid = runtime->pending_intent_valid;
    return snapshot;
}

int mem_console_runtime_metrics_snapshot_changed(
    const MemConsoleRuntimeMetricsSnapshot *before,
    const MemConsoleRuntimeMetricsSnapshot *after) {
    if (!before || !after) {
        return 0;
    }
    return before->refresh_submitted != after->refresh_submitted ||
           before->refresh_applied != after->refresh_applied ||
           before->refresh_dropped_stale != after->refresh_dropped_stale ||
           before->refresh_dropped_mismatch != after->refresh_dropped_mismatch ||
           before->refresh_dropped_editing != after->refresh_dropped_editing ||
           before->refresh_errors != after->refresh_errors ||
           before->refresh_coalesced != after->refresh_coalesced ||
           before->refresh_in_flight != after->refresh_in_flight ||
           before->pending_intent_valid != after->pending_intent_valid;
}

int mem_console_runtime_refreshed_graph_payload_changed(const MemConsoleState *current,
                                                        const MemConsoleState *refreshed) {
    if (!current || !refreshed) {
        return 0;
    }
    if (current->graph_node_count != refreshed->graph_node_count ||
        current->graph_edge_count != refreshed->graph_edge_count ||
        current->graph_query_edge_limit != refreshed->graph_query_edge_limit ||
        current->graph_query_hops != refreshed->graph_query_hops ||
        current->graph_scope_full_mode_enabled != refreshed->graph_scope_full_mode_enabled ||
        current->graph_node_kind_filter_mask != refreshed->graph_node_kind_filter_mask ||
        current->graph_node_kind_filter_all_override != refreshed->graph_node_kind_filter_all_override) {
        return 1;
    }
    if (memcmp(current->graph_nodes, refreshed->graph_nodes, sizeof(current->graph_nodes)) != 0) {
        return 1;
    }
    if (memcmp(current->graph_edges, refreshed->graph_edges, sizeof(current->graph_edges)) != 0) {
        return 1;
    }
    return 0;
}

int mem_console_runtime_refreshed_non_graph_payload_changed(const MemConsoleState *current,
                                                            const MemConsoleState *refreshed) {
    if (!current || !refreshed) {
        return 0;
    }
    if (strcmp(current->schema_version, refreshed->schema_version) != 0 ||
        current->active_count != refreshed->active_count ||
        current->matching_count != refreshed->matching_count ||
        current->visible_start_index != refreshed->visible_start_index ||
        current->visible_count != refreshed->visible_count ||
        current->project_filter_option_count != refreshed->project_filter_option_count ||
        current->selected_project_count != refreshed->selected_project_count ||
        current->selected_item_id != refreshed->selected_item_id ||
        current->graph_center_item_id != refreshed->graph_center_item_id ||
        current->selected_created_ns != refreshed->selected_created_ns ||
        current->selected_pinned != refreshed->selected_pinned ||
        current->selected_canonical != refreshed->selected_canonical) {
        return 1;
    }
    if (strcmp(current->project_filter_summary_line, refreshed->project_filter_summary_line) != 0 ||
        strcmp(current->selected_title, refreshed->selected_title) != 0 ||
        strcmp(current->selected_body, refreshed->selected_body) != 0) {
        return 1;
    }
    if (memcmp(current->visible_items, refreshed->visible_items, sizeof(current->visible_items)) != 0) {
        return 1;
    }
    if (memcmp(current->project_filter_labels, refreshed->project_filter_labels, sizeof(current->project_filter_labels)) != 0) {
        return 1;
    }
    if (memcmp(current->project_filter_keys, refreshed->project_filter_keys, sizeof(current->project_filter_keys)) != 0) {
        return 1;
    }
    if (memcmp(current->project_filter_counts, refreshed->project_filter_counts, sizeof(current->project_filter_counts)) != 0) {
        return 1;
    }
    if (memcmp(current->selected_project_keys, refreshed->selected_project_keys, sizeof(current->selected_project_keys)) != 0) {
        return 1;
    }
    return 0;
}

void mem_console_runtime_copy_selected_project_filters(
    char destination[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int *out_count,
    const char source[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int count) {
    int i;
    int clamped_count = count;

    if (!destination || !out_count || !source) {
        return;
    }

    if (clamped_count < 0) {
        clamped_count = 0;
    }
    if (clamped_count > MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
        clamped_count = MEM_CONSOLE_SCOPE_FILTER_LIMIT;
    }

    for (i = 0; i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        if (i < clamped_count) {
            (void)snprintf(destination[i], 64, "%s", source[i]);
        } else {
            destination[i][0] = '\0';
        }
    }
    *out_count = clamped_count;
}

int mem_console_runtime_selected_project_filters_match(
    const char filters_a[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int count_a,
    const char filters_b[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int count_b) {
    int i;

    if (!filters_a || !filters_b) {
        return 0;
    }
    if (count_a != count_b) {
        return 0;
    }
    if (count_a < 0 || count_a > MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
        return 0;
    }
    if (count_b < 0 || count_b > MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
        return 0;
    }

    for (i = 0; i < count_a; ++i) {
        if (strcmp(filters_a[i], filters_b[i]) != 0) {
            return 0;
        }
    }
    return 1;
}

void mem_console_runtime_apply_refreshed_state(MemConsoleState *state,
                                               const MemConsoleState *refreshed) {
    if (!state || !refreshed) {
        return;
    }

    (void)snprintf(state->schema_version, sizeof(state->schema_version), "%s", refreshed->schema_version);
    state->active_count = refreshed->active_count;
    state->matching_count = refreshed->matching_count;
    state->visible_start_index = refreshed->visible_start_index;
    state->visible_count = refreshed->visible_count;
    state->project_filter_option_count = refreshed->project_filter_option_count;
    state->selected_project_count = refreshed->selected_project_count;
    memcpy(state->visible_items, refreshed->visible_items, sizeof(state->visible_items));
    memcpy(state->project_filter_labels, refreshed->project_filter_labels, sizeof(state->project_filter_labels));
    memcpy(state->project_filter_keys, refreshed->project_filter_keys, sizeof(state->project_filter_keys));
    memcpy(state->project_filter_counts, refreshed->project_filter_counts, sizeof(state->project_filter_counts));
    memcpy(state->selected_project_keys, refreshed->selected_project_keys, sizeof(state->selected_project_keys));
    (void)snprintf(state->project_filter_summary_line,
                   sizeof(state->project_filter_summary_line),
                   "%s",
                   refreshed->project_filter_summary_line);

    state->selected_item_id = refreshed->selected_item_id;
    state->graph_center_item_id = refreshed->graph_center_item_id;
    state->selected_created_ns = refreshed->selected_created_ns;
    state->selected_pinned = refreshed->selected_pinned;
    state->selected_canonical = refreshed->selected_canonical;
    (void)snprintf(state->selected_title, sizeof(state->selected_title), "%s", refreshed->selected_title);
    (void)snprintf(state->selected_body, sizeof(state->selected_body), "%s", refreshed->selected_body);

    state->graph_node_count = refreshed->graph_node_count;
    state->graph_edge_count = refreshed->graph_edge_count;
    memcpy(state->graph_nodes, refreshed->graph_nodes, sizeof(state->graph_nodes));
    memcpy(state->graph_edges, refreshed->graph_edges, sizeof(state->graph_edges));
    state->graph_kind_filter_all_override = refreshed->graph_kind_filter_all_override ? 1 : 0;
    state->graph_kind_filter_mask =
        refreshed->graph_kind_filter_mask & mem_console_graph_kind_filter_all_mask();
    if (state->graph_kind_filter_all_override) {
        state->graph_kind_filter_mask = mem_console_graph_kind_filter_all_mask();
    }
    mem_console_graph_kind_sync_text_filter(state);
    mem_console_graph_edge_limit_set(state, refreshed->graph_query_edge_limit);
    state->graph_query_hops = mem_console_graph_hops_clamp(refreshed->graph_query_hops);
    state->graph_scope_full_mode_enabled = refreshed->graph_scope_full_mode_enabled ? 1 : 0;
}

void mem_console_runtime_publish_metrics(const MemConsoleRuntime *runtime,
                                        MemConsoleState *state) {
    uint64_t dropped_total;

    if (!runtime || !state) {
        return;
    }

    dropped_total = runtime->stats_refresh_dropped_stale +
                    runtime->stats_refresh_dropped_mismatch +
                    runtime->stats_refresh_dropped_editing;

    state->runtime_refresh_submitted = runtime->stats_refresh_submitted;
    state->runtime_refresh_applied = runtime->stats_refresh_applied;
    state->runtime_refresh_dropped = dropped_total;
    state->runtime_refresh_errors = runtime->stats_refresh_errors;
    state->runtime_refresh_coalesced = runtime->stats_refresh_coalesced;
    state->runtime_refresh_in_flight = runtime->refresh_in_flight;
    state->runtime_pending_intent = runtime->pending_intent_valid;

    (void)snprintf(state->runtime_summary_line,
                   sizeof(state->runtime_summary_line),
                   "Async s%llu a%llu d%llu e%llu c%llu | if=%d p=%d",
                   (unsigned long long)state->runtime_refresh_submitted,
                   (unsigned long long)state->runtime_refresh_applied,
                   (unsigned long long)state->runtime_refresh_dropped,
                   (unsigned long long)state->runtime_refresh_errors,
                   (unsigned long long)state->runtime_refresh_coalesced,
                   state->runtime_refresh_in_flight,
                   state->runtime_pending_intent);
}

void mem_console_runtime_capture_intent_from_state(
    const MemConsoleState *state,
    char *out_search_text,
    size_t out_search_cap,
    int64_t *out_selected_item_id,
    int64_t *out_graph_center_item_id,
    int *out_list_query_offset,
    char out_selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int *out_selected_project_count,
    char *out_graph_kind_filter,
    size_t out_graph_kind_filter_cap,
    uint32_t *out_graph_kind_filter_mask,
    int *out_graph_kind_filter_all_override,
    int *out_graph_edge_limit,
    int *out_graph_hops,
    int *out_graph_layout_mode,
    int *out_graph_sort_mode,
    int *out_graph_scope_full_mode_enabled,
    uint32_t *out_graph_node_kind_filter_mask,
    int *out_graph_node_kind_filter_all_override) {
    uint32_t all_mask = 0u;

    if (!state || !out_search_text || out_search_cap == 0u || !out_selected_item_id ||
        !out_graph_center_item_id ||
        !out_list_query_offset || !out_selected_project_keys || !out_selected_project_count ||
        !out_graph_kind_filter || out_graph_kind_filter_cap == 0u ||
        !out_graph_kind_filter_mask || !out_graph_kind_filter_all_override ||
        !out_graph_edge_limit || !out_graph_hops ||
        !out_graph_layout_mode || !out_graph_sort_mode ||
        !out_graph_scope_full_mode_enabled ||
        !out_graph_node_kind_filter_mask || !out_graph_node_kind_filter_all_override) {
        return;
    }
    (void)snprintf(out_search_text, out_search_cap, "%s", state->search_text);
    *out_selected_item_id = state->selected_item_id;
    *out_graph_center_item_id = state->graph_center_item_id;
    *out_list_query_offset = state->list_query_offset;
    mem_console_runtime_copy_selected_project_filters(out_selected_project_keys,
                                                      out_selected_project_count,
                                                      state->selected_project_keys,
                                                      state->selected_project_count);
    (void)snprintf(out_graph_kind_filter, out_graph_kind_filter_cap, "%s", state->graph_kind_filter);
    all_mask = mem_console_graph_kind_filter_all_mask();
    *out_graph_kind_filter_all_override = state->graph_kind_filter_all_override ? 1 : 0;
    *out_graph_kind_filter_mask = state->graph_kind_filter_mask & all_mask;
    if (*out_graph_kind_filter_all_override) {
        *out_graph_kind_filter_mask = all_mask;
    }
    *out_graph_edge_limit = mem_console_graph_edge_limit_clamp(state->graph_query_edge_limit);
    *out_graph_hops = mem_console_graph_hops_clamp(state->graph_query_hops);
    *out_graph_layout_mode = mem_console_graph_layout_mode_clamp(state->graph_layout_mode);
    *out_graph_sort_mode = mem_console_graph_sort_mode_clamp(state->graph_sort_mode);
    *out_graph_scope_full_mode_enabled = state->graph_scope_full_mode_enabled ? 1 : 0;
    all_mask = mem_console_graph_node_kind_filter_all_mask();
    *out_graph_node_kind_filter_all_override = state->graph_node_kind_filter_all_override ? 1 : 0;
    *out_graph_node_kind_filter_mask = state->graph_node_kind_filter_mask & all_mask;
    if (*out_graph_node_kind_filter_all_override) {
        *out_graph_node_kind_filter_mask = all_mask;
    }
}

int mem_console_runtime_intent_matches(
    const char *search_text_a,
    int64_t selected_item_id_a,
    int64_t graph_center_item_id_a,
    int list_query_offset_a,
    const char selected_project_keys_a[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int selected_project_count_a,
    const char *graph_kind_filter_a,
    uint32_t graph_kind_filter_mask_a,
    int graph_kind_filter_all_override_a,
    int graph_edge_limit_a,
    int graph_hops_a,
    int graph_layout_mode_a,
    int graph_sort_mode_a,
    int graph_scope_full_mode_enabled_a,
    uint32_t graph_node_kind_filter_mask_a,
    int graph_node_kind_filter_all_override_a,
    const char *search_text_b,
    int64_t selected_item_id_b,
    int64_t graph_center_item_id_b,
    int list_query_offset_b,
    const char selected_project_keys_b[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int selected_project_count_b,
    const char *graph_kind_filter_b,
    uint32_t graph_kind_filter_mask_b,
    int graph_kind_filter_all_override_b,
    int graph_edge_limit_b,
    int graph_hops_b,
    int graph_layout_mode_b,
    int graph_sort_mode_b,
    int graph_scope_full_mode_enabled_b,
    uint32_t graph_node_kind_filter_mask_b,
    int graph_node_kind_filter_all_override_b) {
    if (!search_text_a || !search_text_b || !graph_kind_filter_a || !graph_kind_filter_b) {
        return 0;
    }
    return strcmp(search_text_a, search_text_b) == 0 &&
           selected_item_id_a == selected_item_id_b &&
           graph_center_item_id_a == graph_center_item_id_b &&
           list_query_offset_a == list_query_offset_b &&
           strcmp(graph_kind_filter_a, graph_kind_filter_b) == 0 &&
           graph_kind_filter_mask_a == graph_kind_filter_mask_b &&
           graph_kind_filter_all_override_a == graph_kind_filter_all_override_b &&
           graph_edge_limit_a == graph_edge_limit_b &&
           graph_hops_a == graph_hops_b &&
           graph_layout_mode_a == graph_layout_mode_b &&
           graph_sort_mode_a == graph_sort_mode_b &&
           graph_scope_full_mode_enabled_a == graph_scope_full_mode_enabled_b &&
           graph_node_kind_filter_mask_a == graph_node_kind_filter_mask_b &&
           graph_node_kind_filter_all_override_a == graph_node_kind_filter_all_override_b &&
           mem_console_runtime_selected_project_filters_match(selected_project_keys_a,
                                                             selected_project_count_a,
                                                             selected_project_keys_b,
                                                             selected_project_count_b);
}

static void *refresh_worker_task(void *task_ctx) {
    MemConsoleRefreshTask *task = (MemConsoleRefreshTask *)task_ctx;
    MemConsoleRefreshCompletion *completion = 0;
    MemConsoleState *worker_state = 0;
    CoreMemDb db = {0};
    CoreResult result = core_result_ok();

    if (!task) {
        return 0;
    }

    completion = (MemConsoleRefreshCompletion *)core_alloc(sizeof(*completion));
    if (!completion) {
        if (task->wake) {
            (void)core_wake_signal(task->wake);
        }
        core_free(task);
        return 0;
    }
    memset(completion, 0, sizeof(*completion));
    completion->request_id = task->request_id;
    completion->selected_item_id = task->selected_item_id;
    completion->graph_center_item_id = task->graph_center_item_id;
    completion->list_query_offset = task->list_query_offset;
    (void)snprintf(completion->search_text, sizeof(completion->search_text), "%s", task->search_text);
    mem_console_runtime_copy_selected_project_filters(completion->selected_project_keys,
                                                      &completion->selected_project_count,
                                                      task->selected_project_keys,
                                                      task->selected_project_count);
    (void)snprintf(completion->graph_kind_filter,
                   sizeof(completion->graph_kind_filter),
                   "%s",
                   task->graph_kind_filter);
    completion->graph_kind_filter_mask = task->graph_kind_filter_mask;
    completion->graph_kind_filter_all_override = task->graph_kind_filter_all_override ? 1 : 0;
    completion->graph_edge_limit = task->graph_edge_limit;
    completion->graph_hops = task->graph_hops;
    completion->graph_layout_mode = task->graph_layout_mode;
    completion->graph_sort_mode = task->graph_sort_mode;
    completion->graph_scope_full_mode_enabled = task->graph_scope_full_mode_enabled ? 1 : 0;
    completion->graph_node_kind_filter_mask = task->graph_node_kind_filter_mask;
    completion->graph_node_kind_filter_all_override = task->graph_node_kind_filter_all_override ? 1 : 0;

    worker_state = (MemConsoleState *)core_alloc(sizeof(*worker_state));
    if (!worker_state) {
        result = (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate worker state" };
        goto finalize;
    }
    seed_state(worker_state, task->db_path);
    worker_state->selected_item_id = task->selected_item_id;
    worker_state->graph_center_item_id = task->graph_center_item_id;
    worker_state->list_query_offset = task->list_query_offset;
    worker_state->list_scroll = 0.0f;
    (void)snprintf(worker_state->search_text, sizeof(worker_state->search_text), "%s", task->search_text);
    mem_console_runtime_copy_selected_project_filters(worker_state->selected_project_keys,
                                                      &worker_state->selected_project_count,
                                                      task->selected_project_keys,
                                                      task->selected_project_count);
    (void)snprintf(worker_state->graph_kind_filter,
                   sizeof(worker_state->graph_kind_filter),
                   "%s",
                   task->graph_kind_filter);
    worker_state->graph_kind_filter_all_override =
        task->graph_kind_filter_all_override ? 1 : 0;
    worker_state->graph_kind_filter_mask =
        task->graph_kind_filter_mask & mem_console_graph_kind_filter_all_mask();
    if (worker_state->graph_kind_filter_all_override) {
        worker_state->graph_kind_filter_mask = mem_console_graph_kind_filter_all_mask();
    }
    mem_console_graph_kind_sync_text_filter(worker_state);
    mem_console_graph_edge_limit_set(worker_state, task->graph_edge_limit);
    worker_state->graph_query_hops = mem_console_graph_hops_clamp(task->graph_hops);
    worker_state->graph_layout_mode = mem_console_graph_layout_mode_clamp(task->graph_layout_mode);
    worker_state->graph_sort_mode = mem_console_graph_sort_mode_clamp(task->graph_sort_mode);
    worker_state->graph_scope_full_mode_enabled = task->graph_scope_full_mode_enabled ? 1 : 0;
    worker_state->graph_node_kind_filter_all_override =
        task->graph_node_kind_filter_all_override ? 1 : 0;
    worker_state->graph_node_kind_filter_mask =
        task->graph_node_kind_filter_mask & mem_console_graph_node_kind_filter_all_mask();
    if (worker_state->graph_node_kind_filter_all_override) {
        worker_state->graph_node_kind_filter_mask = mem_console_graph_node_kind_filter_all_mask();
    }

    result = core_memdb_open(task->db_path, &db);
    if (result.code == CORE_OK) {
        result = refresh_state_from_db(&db, worker_state);
    }
    (void)core_memdb_close(&db);

finalize:
    completion->result.code = result.code;
    if (result.code == CORE_OK) {
        completion->result.message = "ok";
        completion->refreshed_state = *worker_state;
    } else {
        const char *msg = result.message ? result.message : "background refresh failed";
        (void)snprintf(completion->error_message, sizeof(completion->error_message), "%s", msg);
        completion->result.message = completion->error_message;
    }
    if (worker_state) {
        core_free(worker_state);
    }

    if (task->wake) {
        (void)core_wake_signal(task->wake);
    }
    core_free(task);
    return completion;
}

CoreResult mem_console_runtime_schedule_refresh(MemConsoleRuntime *runtime,
                                                const MemConsoleState *state) {
    MemConsoleRefreshTask *task = 0;

    if (!runtime || !state || !state->db_path) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid background refresh request" };
    }
    if (runtime->refresh_in_flight) {
        return core_result_ok();
    }

    task = (MemConsoleRefreshTask *)core_alloc(sizeof(*task));
    if (!task) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate refresh task" };
    }
    memset(task, 0, sizeof(*task));

    (void)snprintf(task->db_path, sizeof(task->db_path), "%s", state->db_path);
    mem_console_runtime_capture_intent_from_state(state,
                                                  task->search_text,
                                                  sizeof(task->search_text),
                                                  &task->selected_item_id,
                                                  &task->graph_center_item_id,
                                                  &task->list_query_offset,
                                                  task->selected_project_keys,
                                                  &task->selected_project_count,
                                                  task->graph_kind_filter,
                                                  sizeof(task->graph_kind_filter),
                                                  &task->graph_kind_filter_mask,
                                                  &task->graph_kind_filter_all_override,
                                                  &task->graph_edge_limit,
                                                  &task->graph_hops,
                                                  &task->graph_layout_mode,
                                                  &task->graph_sort_mode,
                                                  &task->graph_scope_full_mode_enabled,
                                                  &task->graph_node_kind_filter_mask,
                                                  &task->graph_node_kind_filter_all_override);
    task->request_id = runtime->next_request_id++;
    task->wake = &runtime->wake;

    if (!core_workers_submit(&runtime->workers, refresh_worker_task, task)) {
        core_free(task);
        return (CoreResult){ CORE_ERR_IO, "failed to submit refresh task" };
    }

    runtime->refresh_in_flight = 1;
    runtime->stats_refresh_submitted += 1u;
    runtime->in_flight_request_id = task->request_id;
    (void)snprintf(runtime->in_flight_search_text,
                   sizeof(runtime->in_flight_search_text),
                   "%s",
                   task->search_text);
    runtime->in_flight_selected_item_id = task->selected_item_id;
    runtime->in_flight_graph_center_item_id = task->graph_center_item_id;
    runtime->in_flight_list_query_offset = task->list_query_offset;
    mem_console_runtime_copy_selected_project_filters(runtime->in_flight_selected_project_keys,
                                                      &runtime->in_flight_selected_project_count,
                                                      task->selected_project_keys,
                                                      task->selected_project_count);
    (void)snprintf(runtime->in_flight_graph_kind_filter,
                   sizeof(runtime->in_flight_graph_kind_filter),
                   "%s",
                   task->graph_kind_filter);
    runtime->in_flight_graph_kind_filter_mask = task->graph_kind_filter_mask;
    runtime->in_flight_graph_kind_filter_all_override =
        task->graph_kind_filter_all_override ? 1 : 0;
    runtime->in_flight_graph_edge_limit = task->graph_edge_limit;
    runtime->in_flight_graph_hops = task->graph_hops;
    runtime->in_flight_graph_layout_mode = task->graph_layout_mode;
    runtime->in_flight_graph_sort_mode = task->graph_sort_mode;
    runtime->in_flight_graph_scope_full_mode_enabled = task->graph_scope_full_mode_enabled ? 1 : 0;
    runtime->in_flight_graph_node_kind_filter_mask = task->graph_node_kind_filter_mask;
    runtime->in_flight_graph_node_kind_filter_all_override =
        task->graph_node_kind_filter_all_override ? 1 : 0;
    return core_result_ok();
}
