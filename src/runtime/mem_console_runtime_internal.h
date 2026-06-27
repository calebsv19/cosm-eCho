#ifndef MEM_CONSOLE_RUNTIME_INTERNAL_H
#define MEM_CONSOLE_RUNTIME_INTERNAL_H

#include "mem_console_runtime.h"

typedef struct MemConsoleRefreshCompletion {
    CoreResult result;
    char error_message[160];
    char search_text[256];
    int64_t selected_item_id;
    int64_t graph_center_item_id;
    int list_query_offset;
    int browse_pinned_only;
    int browse_canonical_only;
    int browse_kind_index;
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
    MemConsoleState refreshed_state;
} MemConsoleRefreshCompletion;

typedef struct MemConsoleRuntimeMetricsSnapshot {
    uint64_t refresh_submitted;
    uint64_t refresh_applied;
    uint64_t refresh_dropped_stale;
    uint64_t refresh_dropped_mismatch;
    uint64_t refresh_dropped_editing;
    uint64_t refresh_errors;
    uint64_t refresh_coalesced;
    uint64_t latest_refresh_error_id;
    int refresh_in_flight;
    int pending_intent_valid;
} MemConsoleRuntimeMetricsSnapshot;

MemConsoleRuntimeMetricsSnapshot
mem_console_runtime_metrics_snapshot_capture(const MemConsoleRuntime *runtime);

int mem_console_runtime_metrics_snapshot_changed(
    const MemConsoleRuntimeMetricsSnapshot *before,
    const MemConsoleRuntimeMetricsSnapshot *after);

int mem_console_runtime_refreshed_graph_payload_changed(const MemConsoleState *current,
                                                        const MemConsoleState *refreshed);

int mem_console_runtime_refreshed_non_graph_payload_changed(const MemConsoleState *current,
                                                            const MemConsoleState *refreshed);

void mem_console_runtime_copy_selected_project_filters(
    char destination[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int *out_count,
    const char source[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int count);

int mem_console_runtime_selected_project_filters_match(
    const char filters_a[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int count_a,
    const char filters_b[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64],
    int count_b);

void mem_console_runtime_apply_refreshed_state(MemConsoleState *state,
                                               const MemConsoleState *refreshed);

void mem_console_runtime_publish_metrics(const MemConsoleRuntime *runtime,
                                        MemConsoleState *state);

void mem_console_runtime_capture_intent_from_state(
    const MemConsoleState *state,
    char *out_search_text,
    size_t out_search_cap,
    int64_t *out_selected_item_id,
    int64_t *out_graph_center_item_id,
    int *out_list_query_offset,
    int *out_browse_pinned_only,
    int *out_browse_canonical_only,
    int *out_browse_kind_index,
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
    int *out_graph_node_kind_filter_all_override);

int mem_console_runtime_intent_matches(
    const char *search_text_a,
    int64_t selected_item_id_a,
    int64_t graph_center_item_id_a,
    int list_query_offset_a,
    int browse_pinned_only_a,
    int browse_canonical_only_a,
    int browse_kind_index_a,
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
    int browse_pinned_only_b,
    int browse_canonical_only_b,
    int browse_kind_index_b,
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
    int graph_node_kind_filter_all_override_b);

CoreResult mem_console_runtime_schedule_refresh(MemConsoleRuntime *runtime,
                                                const MemConsoleState *state);

#endif
