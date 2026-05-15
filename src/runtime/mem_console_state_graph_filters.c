#include "mem_console_state.h"

#include <stdio.h>
#include <string.h>

typedef struct MemConsoleGraphKindBitEntry {
    const char *kind;
    uint32_t bit;
} MemConsoleGraphKindBitEntry;

static const MemConsoleGraphKindBitEntry k_graph_kind_bit_entries[] = {
    { "supports", 1u << 0 },
    { "depends_on", 1u << 1 },
    { "references", 1u << 2 },
    { "summarizes", 1u << 3 },
    { "related", 1u << 4 },
    { "implements", 1u << 5 },
    { "blocks", 1u << 6 },
    { "contradicts", 1u << 7 }
};

typedef struct MemConsoleNodeKindBitEntry {
    const char *kind;
    uint32_t bit;
} MemConsoleNodeKindBitEntry;

static const MemConsoleNodeKindBitEntry k_node_kind_bit_entries[] = {
    { "plan", 1u << 0 },
    { "decision", 1u << 1 },
    { "issue", 1u << 2 },
    { "scope", 1u << 3 },
    { "summary", 1u << 4 },
    { "policy", 1u << 5 },
    { "runtime", 1u << 6 }
};

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

int mem_console_graph_edge_limit_clamp(int value) {
    if (value < MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN) {
        return MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN;
    }
    if (value > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        return MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }
    return value;
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
    if (value != MEM_CONSOLE_GRAPH_LAYOUT_TREE) {
        return MEM_CONSOLE_GRAPH_LAYOUT_DAG;
    }
    return MEM_CONSOLE_GRAPH_LAYOUT_TREE;
}

int mem_console_graph_view_mode_clamp(int value) {
    if (value == MEM_CONSOLE_GRAPH_VIEW_PODS) {
        return MEM_CONSOLE_GRAPH_VIEW_PODS;
    }
    if (value == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        return MEM_CONSOLE_GRAPH_VIEW_WEB;
    }
    return MEM_CONSOLE_GRAPH_VIEW_FOCUS;
}

int mem_console_graph_view_mode_get(const MemConsoleState *state) {
    if (!state) {
        return MEM_CONSOLE_GRAPH_VIEW_FOCUS;
    }
    if (!state->graph_scope_full_mode_enabled) {
        return MEM_CONSOLE_GRAPH_VIEW_FOCUS;
    }
    if (state->graph_layout_mode == MEM_CONSOLE_GRAPH_LAYOUT_TREE) {
        return MEM_CONSOLE_GRAPH_VIEW_PODS;
    }
    return MEM_CONSOLE_GRAPH_VIEW_WEB;
}

void mem_console_graph_view_mode_reset_viewport(MemConsoleState *state) {
    int view_mode;

    if (!state) {
        return;
    }

    kit_graph_struct_viewport_default(&state->graph_viewport);
    view_mode = mem_console_graph_view_mode_get(state);
    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_PODS) {
        state->graph_viewport.zoom = 0.94f;
    } else if (view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        state->graph_viewport.zoom = 1.02f;
    } else {
        state->graph_viewport.zoom = 1.14f;
    }
}

int mem_console_graph_view_mode_set(MemConsoleState *state, int view_mode) {
    int current_view_mode;
    int changed = 0;

    if (!state) {
        return 0;
    }

    current_view_mode = mem_console_graph_view_mode_get(state);
    view_mode = mem_console_graph_view_mode_clamp(view_mode);

    switch (view_mode) {
        case MEM_CONSOLE_GRAPH_VIEW_PODS:
            if (!state->graph_scope_full_mode_enabled) {
                state->graph_scope_full_mode_enabled = 1;
                changed = 1;
            }
            if (state->graph_layout_mode != MEM_CONSOLE_GRAPH_LAYOUT_TREE) {
                state->graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_TREE;
                changed = 1;
            }
            break;
        case MEM_CONSOLE_GRAPH_VIEW_WEB:
            if (!state->graph_scope_full_mode_enabled) {
                state->graph_scope_full_mode_enabled = 1;
                changed = 1;
            }
            if (state->graph_layout_mode != MEM_CONSOLE_GRAPH_LAYOUT_DAG) {
                state->graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_DAG;
                changed = 1;
            }
            break;
        case MEM_CONSOLE_GRAPH_VIEW_FOCUS:
        default:
            if (state->graph_scope_full_mode_enabled) {
                state->graph_scope_full_mode_enabled = 0;
                changed = 1;
            }
            if (state->graph_layout_mode != MEM_CONSOLE_GRAPH_LAYOUT_DAG) {
                state->graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_DAG;
                changed = 1;
            }
            break;
    }

    if (current_view_mode != view_mode) {
        changed = 1;
    }
    if (changed && current_view_mode != view_mode) {
        mem_console_graph_view_mode_reset_viewport(state);
    }
    return changed;
}

int mem_console_graph_sort_mode_clamp(int value) {
    if (value != MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST) {
        return MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST;
    }
    return MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST;
}

uint32_t mem_console_graph_kind_filter_all_mask(void) {
    uint32_t mask = 0u;
    uint32_t i;

    for (i = 0u; i < (uint32_t)(sizeof(k_graph_kind_bit_entries) / sizeof(k_graph_kind_bit_entries[0])); ++i) {
        mask |= k_graph_kind_bit_entries[i].bit;
    }
    return mask;
}

uint32_t mem_console_graph_kind_filter_mask_for_kind(const char *kind) {
    uint32_t i;

    if (!kind || !kind[0]) {
        return mem_console_graph_kind_filter_all_mask();
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_graph_kind_bit_entries) / sizeof(k_graph_kind_bit_entries[0])); ++i) {
        if (strcmp(kind, k_graph_kind_bit_entries[i].kind) == 0) {
            return k_graph_kind_bit_entries[i].bit;
        }
    }
    return 0u;
}

void mem_console_graph_kind_sync_text_filter(MemConsoleState *state) {
    uint32_t all_mask;
    uint32_t mask;
    uint32_t i;
    uint32_t selected_count = 0u;
    const char *selected_kind = "";

    if (!state) {
        return;
    }

    all_mask = mem_console_graph_kind_filter_all_mask();
    mask = state->graph_kind_filter_mask & all_mask;
    state->graph_kind_filter_mask = mask;

    if (state->graph_kind_filter_all_override) {
        state->graph_kind_filter[0] = '\0';
        return;
    }

    for (i = 0u; i < (uint32_t)(sizeof(k_graph_kind_bit_entries) / sizeof(k_graph_kind_bit_entries[0])); ++i) {
        if ((mask & k_graph_kind_bit_entries[i].bit) != 0u) {
            selected_count += 1u;
            selected_kind = k_graph_kind_bit_entries[i].kind;
        }
    }

    if (selected_count == 1u) {
        (void)snprintf(state->graph_kind_filter, sizeof(state->graph_kind_filter), "%s", selected_kind);
    } else {
        state->graph_kind_filter[0] = '\0';
    }
}

int mem_console_graph_kind_is_enabled(const MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;

    if (!state) {
        return 1;
    }
    if (state->graph_kind_filter_all_override) {
        return 1;
    }

    all_mask = mem_console_graph_kind_filter_all_mask();
    kind_mask = mem_console_graph_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u) {
        return (state->graph_kind_filter_mask & all_mask) == all_mask ? 1 : 0;
    }
    if (kind_mask == all_mask) {
        return (state->graph_kind_filter_mask & all_mask) == all_mask ? 1 : 0;
    }
    return (state->graph_kind_filter_mask & kind_mask) != 0u ? 1 : 0;
}

int mem_console_graph_kind_toggle_enabled(MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;
    uint32_t before_mask;
    int before_all_override;

    if (!state || !kind || !kind[0]) {
        return 0;
    }

    all_mask = mem_console_graph_kind_filter_all_mask();
    kind_mask = mem_console_graph_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u || kind_mask == all_mask) {
        return 0;
    }

    before_mask = state->graph_kind_filter_mask;
    before_all_override = state->graph_kind_filter_all_override;
    state->graph_kind_filter_mask = (state->graph_kind_filter_mask ^ kind_mask) & all_mask;
    state->graph_kind_filter_all_override = 0;
    mem_console_graph_kind_sync_text_filter(state);
    return before_mask != state->graph_kind_filter_mask ||
           before_all_override != state->graph_kind_filter_all_override;
}

void mem_console_graph_kind_select_all(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->graph_kind_filter_all_override = 1;
    state->graph_kind_filter_mask = mem_console_graph_kind_filter_all_mask();
    mem_console_graph_kind_sync_text_filter(state);
}

int mem_console_graph_kind_toggle_all_override(MemConsoleState *state) {
    int before;

    if (!state) {
        return 0;
    }
    before = state->graph_kind_filter_all_override;
    state->graph_kind_filter_all_override = state->graph_kind_filter_all_override ? 0 : 1;
    if (state->graph_kind_filter_all_override) {
        state->graph_kind_filter_mask = mem_console_graph_kind_filter_all_mask();
    }
    mem_console_graph_kind_sync_text_filter(state);
    return before != state->graph_kind_filter_all_override ? 1 : 0;
}

void mem_console_graph_kind_set_single(MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;

    if (!state) {
        return;
    }

    all_mask = mem_console_graph_kind_filter_all_mask();
    if (!kind || !kind[0]) {
        state->graph_kind_filter_all_override = 1;
        mem_console_graph_kind_sync_text_filter(state);
        return;
    }

    kind_mask = mem_console_graph_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u || kind_mask == all_mask) {
        state->graph_kind_filter_all_override = 1;
    } else {
        state->graph_kind_filter_mask = kind_mask;
        state->graph_kind_filter_all_override = 0;
    }
    mem_console_graph_kind_sync_text_filter(state);
}

uint32_t mem_console_graph_node_kind_filter_all_mask(void) {
    uint32_t mask = 0u;
    uint32_t i;

    for (i = 0u; i < (uint32_t)(sizeof(k_node_kind_bit_entries) / sizeof(k_node_kind_bit_entries[0])); ++i) {
        mask |= k_node_kind_bit_entries[i].bit;
    }
    return mask;
}

uint32_t mem_console_graph_node_kind_filter_mask_for_kind(const char *kind) {
    uint32_t i;

    if (!kind || !kind[0]) {
        return 0u;
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_node_kind_bit_entries) / sizeof(k_node_kind_bit_entries[0])); ++i) {
        if (strcmp(kind, k_node_kind_bit_entries[i].kind) == 0) {
            return k_node_kind_bit_entries[i].bit;
        }
    }
    return 0u;
}

int mem_console_graph_node_kind_is_enabled(const MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;

    if (!state) {
        return 1;
    }
    if (state->graph_node_kind_filter_all_override) {
        return 1;
    }

    all_mask = mem_console_graph_node_kind_filter_all_mask();
    kind_mask = mem_console_graph_node_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u) {
        return 0;
    }
    return (state->graph_node_kind_filter_mask & kind_mask & all_mask) != 0u ? 1 : 0;
}

int mem_console_graph_node_kind_toggle_enabled(MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;
    uint32_t before_mask;

    if (!state || !kind || !kind[0]) {
        return 0;
    }

    all_mask = mem_console_graph_node_kind_filter_all_mask();
    kind_mask = mem_console_graph_node_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u) {
        return 0;
    }

    before_mask = state->graph_node_kind_filter_mask;
    state->graph_node_kind_filter_mask = (state->graph_node_kind_filter_mask ^ kind_mask) & all_mask;
    state->graph_node_kind_filter_all_override = 0;
    return before_mask != state->graph_node_kind_filter_mask ? 1 : 0;
}

void mem_console_graph_node_kind_select_all(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->graph_node_kind_filter_all_override = 1;
    state->graph_node_kind_filter_mask = mem_console_graph_node_kind_filter_all_mask();
}

int mem_console_graph_node_kind_toggle_all_override(MemConsoleState *state) {
    int before;

    if (!state) {
        return 0;
    }
    before = state->graph_node_kind_filter_all_override;
    state->graph_node_kind_filter_all_override = state->graph_node_kind_filter_all_override ? 0 : 1;
    if (state->graph_node_kind_filter_all_override) {
        state->graph_node_kind_filter_mask = mem_console_graph_node_kind_filter_all_mask();
    }
    return before != state->graph_node_kind_filter_all_override ? 1 : 0;
}

int mem_console_graph_anchor_hidden_is_set(const MemConsoleState *state, int64_t item_id) {
    int i;
    int count = 0;

    if (!state || item_id <= 0) {
        return 0;
    }

    count = state->graph_hidden_anchor_count;
    if (count < 0) {
        count = 0;
    }
    if (count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }
    for (i = 0; i < count; ++i) {
        if (state->graph_hidden_anchor_item_ids[i] == item_id) {
            return 1;
        }
    }
    return 0;
}

int mem_console_graph_anchor_hidden_set(MemConsoleState *state, int64_t item_id, int hidden) {
    int i;
    int count = 0;
    int found_index = -1;
    int changed = 0;

    if (!state || item_id <= 0) {
        return 0;
    }

    count = state->graph_hidden_anchor_count;
    if (count < 0) {
        count = 0;
    }
    if (count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }
    state->graph_hidden_anchor_count = count;

    for (i = 0; i < count; ++i) {
        if (state->graph_hidden_anchor_item_ids[i] == item_id) {
            found_index = i;
            break;
        }
    }

    if (hidden) {
        if (found_index >= 0) {
            return 0;
        }
        if (count >= MEM_CONSOLE_GRAPH_NODE_LIMIT) {
            return 0;
        }
        state->graph_hidden_anchor_item_ids[count] = item_id;
        state->graph_hidden_anchor_count = count + 1;
        return 1;
    }

    if (found_index < 0) {
        return 0;
    }
    for (i = found_index + 1; i < count; ++i) {
        state->graph_hidden_anchor_item_ids[i - 1] = state->graph_hidden_anchor_item_ids[i];
    }
    state->graph_hidden_anchor_item_ids[count - 1] = 0;
    state->graph_hidden_anchor_count = count - 1;
    changed = 1;
    return changed;
}

int mem_console_graph_anchor_hidden_toggle(MemConsoleState *state, int64_t item_id, int *out_hidden) {
    int was_hidden = 0;
    int next_hidden = 0;
    int changed = 0;

    if (!state || item_id <= 0) {
        return 0;
    }

    was_hidden = mem_console_graph_anchor_hidden_is_set(state, item_id);
    next_hidden = was_hidden ? 0 : 1;
    changed = mem_console_graph_anchor_hidden_set(state, item_id, next_hidden);
    if (out_hidden) {
        *out_hidden = next_hidden;
    }
    return changed;
}

int mem_console_graph_edge_limit_parse(const char *text, int fallback) {
    int parsed = 0;
    int i = 0;

    if (!text || text[0] == '\0') {
        return mem_console_graph_edge_limit_clamp(fallback);
    }

    while (text[i] != '\0') {
        if (text[i] < '0' || text[i] > '9') {
            return mem_console_graph_edge_limit_clamp(fallback);
        }
        parsed = (parsed * 10) + (text[i] - '0');
        if (parsed > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
            parsed = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
            break;
        }
        i += 1;
    }

    return mem_console_graph_edge_limit_clamp(parsed);
}

void mem_console_graph_edge_limit_set(MemConsoleState *state, int value) {
    if (!state) {
        return;
    }

    state->graph_query_edge_limit = mem_console_graph_edge_limit_clamp(value);
    (void)snprintf(state->graph_edge_limit_text,
                   sizeof(state->graph_edge_limit_text),
                   "%d",
                   state->graph_query_edge_limit);
    state->graph_edge_limit_cursor = (int)strlen(state->graph_edge_limit_text);
}
