#ifndef MEM_CONSOLE_UI_GRAPH_LAYOUT_FOCUS_HELPERS_H
#define MEM_CONSOLE_UI_GRAPH_LAYOUT_FOCUS_HELPERS_H

#include "mem_console_ui_graph_internal.h"

void mem_console_ui_graph_configure_layout_style(KitGraphStructLayoutStyle *style);
void mem_console_ui_graph_transpose_layouts_to_horizontal_flow(KitRenderRect bounds,
                                                               KitGraphStructNodeLayout *layouts,
                                                               uint32_t layout_count);
void mem_console_ui_graph_apply_focus_anchor_priority_layout(KitRenderRect bounds,
                                                             const MemConsoleState *state,
                                                             const KitGraphStructEdge *edges,
                                                             uint32_t edge_count,
                                                             KitGraphStructNodeLayout *layouts,
                                                             uint32_t layout_count);
uint64_t mem_console_ui_graph_preview_layout_signature(const MemConsoleState *state,
                                                       KitRenderRect bounds);
void mem_console_ui_graph_filter_edges_for_visible_layout_nodes(KitRenderRect bounds,
                                                                const KitGraphStructNodeLayout *layouts,
                                                                uint32_t layout_count,
                                                                KitGraphStructEdge *edges,
                                                                int *edge_state_indices,
                                                                uint32_t *io_edge_count);

#endif
