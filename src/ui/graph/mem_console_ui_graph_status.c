#include "mem_console_ui_graph_internal.h"

#include <stdio.h>

static const char *graph_status_view_mode_text(const MemConsoleState *state) {
    int view_mode = mem_console_graph_view_mode_get(state);

    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_PODS) {
        return "pods";
    }
    if (view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
        return "web";
    }
    return "focus";
}

void graph_status_format_view_line(MemConsoleState *state,
                                   uint32_t node_count,
                                   uint32_t edge_count) {
    if (!state) {
        return;
    }

    (void)snprintf(state->graph_status_line,
                   sizeof(state->graph_status_line),
                   "mode:%s  sort:%s  lbl:%s  fnl:%s  zoom:%.2fx  n:%u e:%u",
                   graph_status_view_mode_text(state),
                   state->graph_sort_mode == MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST ? "old" : "new",
                   state->graph_edge_labels_enabled ? "on" : "off",
                   state->graph_anchor_funnel_enabled ? "on" : "off",
                   state->graph_viewport.zoom,
                   node_count,
                   edge_count);
}

void graph_status_format_node_hud(MemConsoleState *state,
                                  const MemConsoleGraphNode *node) {
    const char *project_key;
    int64_t anchor_item_id;

    if (!state || !node) {
        return;
    }

    project_key = node->project_key[0] ? node->project_key : "misc";
    anchor_item_id = node->render_anchor_item_id > 0 ? node->render_anchor_item_id : node->item_id;

    (void)snprintf(state->graph_hud_id_line,
                   sizeof(state->graph_hud_id_line),
                   "ID %lld | %s",
                   (long long)node->item_id,
                   project_key);
    if (node->is_rollup_node) {
        (void)snprintf(state->graph_hud_flags,
                       sizeof(state->graph_hud_flags),
                       "PIN %s | CAN %s | ROLLUP ON | ANCHOR %lld",
                       node->pinned ? "ON" : "OFF",
                       node->canonical ? "ON" : "OFF",
                       (long long)anchor_item_id);
    } else {
        (void)snprintf(state->graph_hud_flags,
                       sizeof(state->graph_hud_flags),
                       "PIN %s | CAN %s",
                       node->pinned ? "ON" : "OFF",
                       node->canonical ? "ON" : "OFF");
    }
}

void graph_status_format_edge_hud(MemConsoleState *state,
                                  const MemConsoleGraphNode *from_node,
                                  const MemConsoleGraphNode *to_node,
                                  const char *edge_kind_label) {
    const char *from_title;
    const char *to_title;

    if (!state || !from_node || !to_node || !edge_kind_label) {
        return;
    }

    from_title = from_node->title[0] ? from_node->title : "UNKNOWN";
    to_title = to_node->title[0] ? to_node->title : "UNKNOWN";

    (void)snprintf(state->graph_hud_id_line,
                   sizeof(state->graph_hud_id_line),
                   "EDGE %s",
                   edge_kind_label);
    (void)snprintf(state->graph_hud_flags,
                   sizeof(state->graph_hud_flags),
                   "%lld --%s--> %lld",
                   (long long)from_node->item_id,
                   edge_kind_label,
                   (long long)to_node->item_id);
    (void)snprintf(state->graph_hud_body,
                   sizeof(state->graph_hud_body),
                   "LINK: %s -> %s",
                   from_title,
                   to_title);
}

void graph_status_format_anchor_visibility_line(MemConsoleState *state,
                                                const MemConsoleGraphNode *node,
                                                int now_hidden) {
    if (!state || !node) {
        return;
    }

    (void)snprintf(state->status_line,
                   sizeof(state->status_line),
                   "%s anchor %s.",
                   node->title[0] ? node->title : "Top-level",
                   now_hidden ? "hidden" : "shown");
}
