#include "mem_console_runtime_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum {
    MEM_CONSOLE_RUNTIME_DEFAULT_POLL_MS = 900,
    MEM_CONSOLE_RUNTIME_POLL_STEADY_MS = 2000,
    MEM_CONSOLE_RUNTIME_POLL_MAX_MS = 5000,
    MEM_CONSOLE_RUNTIME_IDLE_MAX_WAIT_MS = 120,
    MEM_CONSOLE_RUNTIME_IDLE_ACTIVE_WAIT_MS = 8
};

enum {
    MEM_CONSOLE_RUNTIME_WAIT_MIN_MS = 1,
    MEM_CONSOLE_RUNTIME_WAIT_MAX_MS = 5000
};

static uint32_t mem_console_runtime_wait_cap_ms(void) {
    const char *env_value = getenv("MEM_CONSOLE_LOOP_MAX_WAIT_MS");
    char *end = NULL;
    long parsed = 0;

    if (!env_value || !env_value[0]) {
        return MEM_CONSOLE_RUNTIME_IDLE_MAX_WAIT_MS;
    }
    parsed = strtol(env_value, &end, 10);
    if (end == env_value || *end != '\0') {
        return MEM_CONSOLE_RUNTIME_IDLE_MAX_WAIT_MS;
    }
    if (parsed < MEM_CONSOLE_RUNTIME_WAIT_MIN_MS || parsed > MEM_CONSOLE_RUNTIME_WAIT_MAX_MS) {
        return MEM_CONSOLE_RUNTIME_IDLE_MAX_WAIT_MS;
    }
    return (uint32_t)parsed;
}

static uint32_t runtime_next_backoff_interval_ms(uint32_t current_ms) {
    if (current_ms < MEM_CONSOLE_RUNTIME_POLL_STEADY_MS) {
        return MEM_CONSOLE_RUNTIME_POLL_STEADY_MS;
    }
    return MEM_CONSOLE_RUNTIME_POLL_MAX_MS;
}

static void runtime_reset_poll_interval(MemConsoleRuntime *runtime) {
    if (!runtime) {
        return;
    }
    runtime->poll_interval_ms = MEM_CONSOLE_RUNTIME_DEFAULT_POLL_MS;
}

static void runtime_backoff_poll_interval(MemConsoleRuntime *runtime) {
    if (!runtime) {
        return;
    }
    runtime->poll_interval_ms = runtime_next_backoff_interval_ms(runtime->poll_interval_ms);
}

CoreResult mem_console_runtime_init(MemConsoleRuntime *runtime, uint64_t now_ms) {
    if (!runtime) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid runtime init" };
    }

    memset(runtime, 0, sizeof(*runtime));
    runtime->poll_interval_ms = MEM_CONSOLE_RUNTIME_DEFAULT_POLL_MS;
    runtime->next_request_id = 1u;
    runtime->next_poll_due_ms = now_ms + runtime->poll_interval_ms;

    if (!core_queue_mutex_init(&runtime->completion_queue,
                               runtime->completion_slots,
                               MEM_CONSOLE_RUNTIME_COMPLETION_CAPACITY)) {
        return (CoreResult){ CORE_ERR_IO, "completion queue init failed" };
    }
    runtime->queue_initialized = 1;

    if (!core_wake_init_cond(&runtime->wake)) {
        core_queue_mutex_destroy(&runtime->completion_queue);
        runtime->queue_initialized = 0;
        return (CoreResult){ CORE_ERR_IO, "wake init failed" };
    }
    runtime->wake_initialized = 1;

    if (!core_workers_init(&runtime->workers,
                           runtime->worker_threads,
                           MEM_CONSOLE_RUNTIME_WORKER_THREADS,
                           runtime->worker_tasks,
                           MEM_CONSOLE_RUNTIME_TASK_CAPACITY,
                           &runtime->completion_queue)) {
        core_wake_shutdown(&runtime->wake);
        runtime->wake_initialized = 0;
        core_queue_mutex_destroy(&runtime->completion_queue);
        runtime->queue_initialized = 0;
        return (CoreResult){ CORE_ERR_IO, "worker init failed" };
    }

    return core_result_ok();
}

void mem_console_runtime_shutdown(MemConsoleRuntime *runtime) {
    void *msg = 0;

    if (!runtime) {
        return;
    }

    if (runtime->workers.initialized) {
        core_workers_shutdown_with_mode(&runtime->workers, CORE_WORKERS_SHUTDOWN_CANCEL);
    }
    if (runtime->queue_initialized) {
        while (core_queue_mutex_pop(&runtime->completion_queue, &msg)) {
            core_free(msg);
        }
    }
    if (runtime->wake_initialized) {
        core_wake_shutdown(&runtime->wake);
    }
    if (runtime->queue_initialized) {
        core_queue_mutex_destroy(&runtime->completion_queue);
    }
    memset(runtime, 0, sizeof(*runtime));
}

void mem_console_runtime_note_local_write(MemConsoleRuntime *runtime, uint64_t now_ms) {
    if (!runtime) {
        return;
    }

    runtime_reset_poll_interval(runtime);
    runtime->next_poll_due_ms = now_ms;
    (void)core_wake_signal(&runtime->wake);
}

void mem_console_runtime_tick(MemConsoleRuntime *runtime,
                              MemConsoleState *state,
                              uint64_t now_ms,
                              MemConsoleRuntimeTickOutcome *outcome) {
    void *msg = 0;
    char current_search_text[256];
    int64_t current_selected_item_id = 0;
    int64_t current_graph_center_item_id = 0;
    int current_list_query_offset = 0;
    char current_selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    int current_selected_project_count = 0;
    char current_graph_kind_filter[32];
    uint32_t current_graph_kind_filter_mask = 0u;
    int current_graph_kind_filter_all_override = 1;
    int current_graph_edge_limit = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    int current_graph_hops = MEM_CONSOLE_GRAPH_HOPS_MIN;
    int current_graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_DAG;
    int current_graph_sort_mode = MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST;
    int current_graph_scope_full_mode_enabled = 0;
    uint32_t current_graph_node_kind_filter_mask = 0u;
    int current_graph_node_kind_filter_all_override = 1;
    int can_schedule = 0;
    MemConsoleRuntimeMetricsSnapshot metrics_before;
    MemConsoleRuntimeMetricsSnapshot metrics_after;

    if (outcome) {
        memset(outcome, 0, sizeof(*outcome));
    }
    if (!runtime || !state) {
        return;
    }
    metrics_before = mem_console_runtime_metrics_snapshot_capture(runtime);
    if (!runtime->queue_initialized || !runtime->wake_initialized || !runtime->workers.initialized) {
        mem_console_runtime_publish_metrics(runtime, state);
        goto finalize_metrics;
    }

    mem_console_runtime_publish_metrics(runtime, state);

    mem_console_runtime_capture_intent_from_state(state,
                                                  current_search_text,
                                                  sizeof(current_search_text),
                                                  &current_selected_item_id,
                                                  &current_graph_center_item_id,
                                                  &current_list_query_offset,
                                                  current_selected_project_keys,
                                                  &current_selected_project_count,
                                                  current_graph_kind_filter,
                                                  sizeof(current_graph_kind_filter),
                                                  &current_graph_kind_filter_mask,
                                                  &current_graph_kind_filter_all_override,
                                                  &current_graph_edge_limit,
                                                  &current_graph_hops,
                                                  &current_graph_layout_mode,
                                                  &current_graph_sort_mode,
                                                  &current_graph_scope_full_mode_enabled,
                                                  &current_graph_node_kind_filter_mask,
                                                  &current_graph_node_kind_filter_all_override);

    can_schedule = !state->title_edit_mode &&
                   !state->body_edit_mode &&
                   !state->search_refresh_pending;

    if (runtime->refresh_in_flight &&
        !mem_console_runtime_intent_matches(current_search_text,
                                            current_selected_item_id,
                                            current_graph_center_item_id,
                                            current_list_query_offset,
                                            current_selected_project_keys,
                                            current_selected_project_count,
                                            current_graph_kind_filter,
                                            current_graph_kind_filter_mask,
                                            current_graph_kind_filter_all_override,
                                            current_graph_edge_limit,
                                            current_graph_hops,
                                            current_graph_layout_mode,
                                            current_graph_sort_mode,
                                            current_graph_scope_full_mode_enabled,
                                            current_graph_node_kind_filter_mask,
                                            current_graph_node_kind_filter_all_override,
                                            runtime->in_flight_search_text,
                                            runtime->in_flight_selected_item_id,
                                            runtime->in_flight_graph_center_item_id,
                                            runtime->in_flight_list_query_offset,
                                            runtime->in_flight_selected_project_keys,
                                            runtime->in_flight_selected_project_count,
                                            runtime->in_flight_graph_kind_filter,
                                            runtime->in_flight_graph_kind_filter_mask,
                                            runtime->in_flight_graph_kind_filter_all_override,
                                            runtime->in_flight_graph_edge_limit,
                                            runtime->in_flight_graph_hops,
                                            runtime->in_flight_graph_layout_mode,
                                            runtime->in_flight_graph_sort_mode,
                                            runtime->in_flight_graph_scope_full_mode_enabled,
                                            runtime->in_flight_graph_node_kind_filter_mask,
                                            runtime->in_flight_graph_node_kind_filter_all_override)) {
        if (!runtime->pending_intent_valid ||
            !mem_console_runtime_intent_matches(current_search_text,
                                                current_selected_item_id,
                                                current_graph_center_item_id,
                                                current_list_query_offset,
                                                current_selected_project_keys,
                                                current_selected_project_count,
                                                current_graph_kind_filter,
                                                current_graph_kind_filter_mask,
                                                current_graph_kind_filter_all_override,
                                                current_graph_edge_limit,
                                                current_graph_hops,
                                                current_graph_layout_mode,
                                                current_graph_sort_mode,
                                                current_graph_scope_full_mode_enabled,
                                                current_graph_node_kind_filter_mask,
                                                current_graph_node_kind_filter_all_override,
                                                runtime->pending_search_text,
                                                runtime->pending_selected_item_id,
                                                runtime->pending_graph_center_item_id,
                                                runtime->pending_list_query_offset,
                                                runtime->pending_selected_project_keys,
                                                runtime->pending_selected_project_count,
                                                runtime->pending_graph_kind_filter,
                                                runtime->pending_graph_kind_filter_mask,
                                                runtime->pending_graph_kind_filter_all_override,
                                                runtime->pending_graph_edge_limit,
                                                runtime->pending_graph_hops,
                                                runtime->pending_graph_layout_mode,
                                                runtime->pending_graph_sort_mode,
                                                runtime->pending_graph_scope_full_mode_enabled,
                                                runtime->pending_graph_node_kind_filter_mask,
                                                runtime->pending_graph_node_kind_filter_all_override)) {
            runtime->stats_refresh_coalesced += 1u;
            runtime->pending_intent_valid = 1;
            (void)snprintf(runtime->pending_search_text,
                           sizeof(runtime->pending_search_text),
                           "%s",
                           current_search_text);
            runtime->pending_selected_item_id = current_selected_item_id;
            runtime->pending_graph_center_item_id = current_graph_center_item_id;
            runtime->pending_list_query_offset = current_list_query_offset;
            mem_console_runtime_copy_selected_project_filters(runtime->pending_selected_project_keys,
                                                              &runtime->pending_selected_project_count,
                                                              current_selected_project_keys,
                                                              current_selected_project_count);
            (void)snprintf(runtime->pending_graph_kind_filter,
                           sizeof(runtime->pending_graph_kind_filter),
                           "%s",
                           current_graph_kind_filter);
            runtime->pending_graph_kind_filter_mask = current_graph_kind_filter_mask;
            runtime->pending_graph_kind_filter_all_override =
                current_graph_kind_filter_all_override ? 1 : 0;
            runtime->pending_graph_edge_limit = current_graph_edge_limit;
            runtime->pending_graph_hops = current_graph_hops;
            runtime->pending_graph_layout_mode = current_graph_layout_mode;
            runtime->pending_graph_sort_mode = current_graph_sort_mode;
            runtime->pending_graph_scope_full_mode_enabled = current_graph_scope_full_mode_enabled;
            runtime->pending_graph_node_kind_filter_mask = current_graph_node_kind_filter_mask;
            runtime->pending_graph_node_kind_filter_all_override =
                current_graph_node_kind_filter_all_override ? 1 : 0;
        }
    }

    if (!runtime->refresh_in_flight &&
        can_schedule &&
        now_ms >= runtime->next_poll_due_ms) {
        CoreResult result = mem_console_runtime_schedule_refresh(runtime, state);
        if (result.code == CORE_OK) {
            runtime->next_poll_due_ms = now_ms + runtime->poll_interval_ms;
        } else {
            const char *msg_text = result.message ? result.message : "background refresh scheduling failed";
            (void)snprintf(state->status_line,
                           sizeof(state->status_line),
                           "Async refresh: %s",
                           msg_text);
            runtime->next_poll_due_ms = now_ms + runtime->poll_interval_ms;
        }
    }

    (void)core_wake_wait(&runtime->wake, 0u);

    while (core_queue_mutex_pop(&runtime->completion_queue, &msg)) {
        MemConsoleRefreshCompletion *completion = (MemConsoleRefreshCompletion *)msg;
        uint32_t state_graph_kind_filter_mask = 0u;
        uint32_t state_graph_node_kind_filter_mask = 0u;
        runtime->refresh_in_flight = 0;

        if (!completion) {
            continue;
        }
        if (completion->request_id < runtime->last_applied_request_id ||
            completion->request_id < runtime->in_flight_request_id) {
            runtime->stats_refresh_dropped_stale += 1u;
            core_free(completion);
            continue;
        }
        state_graph_kind_filter_mask =
            state->graph_kind_filter_mask & mem_console_graph_kind_filter_all_mask();
        if (state->graph_kind_filter_all_override) {
            state_graph_kind_filter_mask = mem_console_graph_kind_filter_all_mask();
        }
        state_graph_node_kind_filter_mask =
            state->graph_node_kind_filter_mask & mem_console_graph_node_kind_filter_all_mask();
        if (state->graph_node_kind_filter_all_override) {
            state_graph_node_kind_filter_mask = mem_console_graph_node_kind_filter_all_mask();
        }
        if (completion->selected_item_id != state->selected_item_id ||
            completion->graph_center_item_id != state->graph_center_item_id ||
            completion->list_query_offset != state->list_query_offset ||
            strcmp(completion->search_text, state->search_text) != 0 ||
            strcmp(completion->graph_kind_filter, state->graph_kind_filter) != 0 ||
            completion->graph_kind_filter_mask != state_graph_kind_filter_mask ||
            completion->graph_kind_filter_all_override !=
                (state->graph_kind_filter_all_override ? 1 : 0) ||
            completion->graph_edge_limit != state->graph_query_edge_limit ||
            completion->graph_hops != state->graph_query_hops ||
            completion->graph_layout_mode != state->graph_layout_mode ||
            completion->graph_sort_mode != state->graph_sort_mode ||
            completion->graph_scope_full_mode_enabled != (state->graph_scope_full_mode_enabled ? 1 : 0) ||
            completion->graph_node_kind_filter_mask != state_graph_node_kind_filter_mask ||
            completion->graph_node_kind_filter_all_override !=
                (state->graph_node_kind_filter_all_override ? 1 : 0) ||
            !mem_console_runtime_selected_project_filters_match(completion->selected_project_keys,
                                                                completion->selected_project_count,
                                                                state->selected_project_keys,
                                                                state->selected_project_count)) {
            runtime->stats_refresh_dropped_mismatch += 1u;
            core_free(completion);
            continue;
        }
        if (state->title_edit_mode || state->body_edit_mode || state->search_refresh_pending) {
            runtime->stats_refresh_dropped_editing += 1u;
            core_free(completion);
            continue;
        }

        runtime->last_applied_request_id = completion->request_id;
        if (completion->result.code != CORE_OK) {
            const char *error_text = completion->result.message ? completion->result.message : "background refresh failed";
            (void)snprintf(state->status_line,
                           sizeof(state->status_line),
                           "Async refresh failed: %s",
                           error_text);
            runtime->stats_refresh_errors += 1u;
            runtime->next_poll_due_ms = now_ms + runtime->poll_interval_ms;
            core_free(completion);
            continue;
        }

        {
            int graph_changed = mem_console_runtime_refreshed_graph_payload_changed(state,
                                                                                    &completion->refreshed_state);
            int content_changed = graph_changed ||
                                  mem_console_runtime_refreshed_non_graph_payload_changed(state,
                                                                                           &completion->refreshed_state);

            if (content_changed) {
                mem_console_runtime_apply_refreshed_state(state, &completion->refreshed_state);
                sync_edit_buffers_from_selection(state);
                runtime_reset_poll_interval(runtime);
                if (outcome) {
                    outcome->content_changed = 1;
                    if (graph_changed) {
                        outcome->graph_content_changed = 1;
                    }
                }
            } else {
                runtime_backoff_poll_interval(runtime);
            }
        }
        runtime->stats_refresh_applied += 1u;
        runtime->next_poll_due_ms = now_ms + runtime->poll_interval_ms;
        core_free(completion);
    }

    if (!runtime->refresh_in_flight &&
        runtime->pending_intent_valid &&
        can_schedule) {
        MemConsoleState pending_state = *state;
        (void)snprintf(pending_state.search_text,
                       sizeof(pending_state.search_text),
                       "%s",
                       runtime->pending_search_text);
        pending_state.selected_item_id = runtime->pending_selected_item_id;
        pending_state.graph_center_item_id = runtime->pending_graph_center_item_id;
        pending_state.list_query_offset = runtime->pending_list_query_offset;
        mem_console_runtime_copy_selected_project_filters(pending_state.selected_project_keys,
                                                          &pending_state.selected_project_count,
                                                          runtime->pending_selected_project_keys,
                                                          runtime->pending_selected_project_count);
        pending_state.graph_kind_filter_all_override =
            runtime->pending_graph_kind_filter_all_override ? 1 : 0;
        pending_state.graph_kind_filter_mask =
            runtime->pending_graph_kind_filter_mask & mem_console_graph_kind_filter_all_mask();
        if (pending_state.graph_kind_filter_all_override) {
            pending_state.graph_kind_filter_mask = mem_console_graph_kind_filter_all_mask();
        }
        mem_console_graph_kind_sync_text_filter(&pending_state);
        mem_console_graph_edge_limit_set(&pending_state, runtime->pending_graph_edge_limit);
        pending_state.graph_query_hops = mem_console_graph_hops_clamp(runtime->pending_graph_hops);
        pending_state.graph_layout_mode =
            mem_console_graph_layout_mode_clamp(runtime->pending_graph_layout_mode);
        pending_state.graph_sort_mode = mem_console_graph_sort_mode_clamp(runtime->pending_graph_sort_mode);
        pending_state.graph_scope_full_mode_enabled =
            runtime->pending_graph_scope_full_mode_enabled ? 1 : 0;
        pending_state.graph_node_kind_filter_all_override =
            runtime->pending_graph_node_kind_filter_all_override ? 1 : 0;
        pending_state.graph_node_kind_filter_mask =
            runtime->pending_graph_node_kind_filter_mask & mem_console_graph_node_kind_filter_all_mask();
        if (pending_state.graph_node_kind_filter_all_override) {
            pending_state.graph_node_kind_filter_mask = mem_console_graph_node_kind_filter_all_mask();
        }

        if (mem_console_runtime_intent_matches(pending_state.search_text,
                                               pending_state.selected_item_id,
                                               pending_state.graph_center_item_id,
                                               pending_state.list_query_offset,
                                               pending_state.selected_project_keys,
                                               pending_state.selected_project_count,
                                               pending_state.graph_kind_filter,
                                               pending_state.graph_kind_filter_mask,
                                               pending_state.graph_kind_filter_all_override,
                                               pending_state.graph_query_edge_limit,
                                               pending_state.graph_query_hops,
                                               pending_state.graph_layout_mode,
                                               pending_state.graph_sort_mode,
                                               pending_state.graph_scope_full_mode_enabled,
                                               pending_state.graph_node_kind_filter_mask,
                                               pending_state.graph_node_kind_filter_all_override,
                                               state->search_text,
                                               state->selected_item_id,
                                               state->graph_center_item_id,
                                               state->list_query_offset,
                                               state->selected_project_keys,
                                               state->selected_project_count,
                                               state->graph_kind_filter,
                                               state->graph_kind_filter_mask &
                                                   mem_console_graph_kind_filter_all_mask(),
                                               state->graph_kind_filter_all_override ? 1 : 0,
                                               state->graph_query_edge_limit,
                                               state->graph_query_hops,
                                               state->graph_layout_mode,
                                               state->graph_sort_mode,
                                               state->graph_scope_full_mode_enabled ? 1 : 0,
                                               state->graph_node_kind_filter_mask &
                                                   mem_console_graph_node_kind_filter_all_mask(),
                                               state->graph_node_kind_filter_all_override ? 1 : 0)) {
            runtime_reset_poll_interval(runtime);
            CoreResult result = mem_console_runtime_schedule_refresh(runtime, &pending_state);
            if (result.code == CORE_OK) {
                runtime->pending_intent_valid = 0;
                runtime->next_poll_due_ms = now_ms + runtime->poll_interval_ms;
            }
        } else {
            runtime->pending_intent_valid = 0;
        }
    }

    mem_console_runtime_publish_metrics(runtime, state);

finalize_metrics:
    metrics_after = mem_console_runtime_metrics_snapshot_capture(runtime);
    if (outcome) {
        outcome->metrics_changed =
            mem_console_runtime_metrics_snapshot_changed(&metrics_before, &metrics_after) ? 1 : 0;
    }
}

uint32_t mem_console_runtime_idle_wait_ms(const MemConsoleRuntime *runtime,
                                          const MemConsoleState *state,
                                          uint64_t now_ms) {
    uint64_t remaining_ms = 0;
    uint32_t wait_cap_ms = mem_console_runtime_wait_cap_ms();

    if (!state) {
        return 0u;
    }

    if (state->search_refresh_pending) {
        uint64_t elapsed_ms = 0;
        if (now_ms > state->search_last_input_ms) {
            elapsed_ms = now_ms - state->search_last_input_ms;
        }
        if (elapsed_ms >= 150u) {
            return 0u;
        }
        remaining_ms = 150u - elapsed_ms;
        if (remaining_ms > wait_cap_ms) {
            remaining_ms = wait_cap_ms;
        }
        return (uint32_t)remaining_ms;
    }

    if (!runtime ||
        !runtime->workers.initialized ||
        !runtime->queue_initialized ||
        !runtime->wake_initialized) {
        return MEM_CONSOLE_RUNTIME_IDLE_ACTIVE_WAIT_MS;
    }

    if (runtime->refresh_in_flight) {
        return MEM_CONSOLE_RUNTIME_IDLE_ACTIVE_WAIT_MS;
    }

    if (now_ms >= runtime->next_poll_due_ms) {
        return 0u;
    }

    remaining_ms = runtime->next_poll_due_ms - now_ms;
    if (remaining_ms > wait_cap_ms) {
        remaining_ms = wait_cap_ms;
    }
    if (remaining_ms == 0u) {
        return 0u;
    }
    return (uint32_t)remaining_ms;
}
