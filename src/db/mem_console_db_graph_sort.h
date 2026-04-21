#ifndef MEM_CONSOLE_DB_GRAPH_SORT_H
#define MEM_CONSOLE_DB_GRAPH_SORT_H

#include "mem_console_db.h"

void mem_console_db_annotate_rollup_render_anchors(MemConsoleState *state);
void mem_console_db_apply_graph_node_sort(MemConsoleState *state);
void mem_console_db_apply_graph_edge_priority(MemConsoleState *state, int edge_limit);
void mem_console_db_compact_graph_by_node_kind(MemConsoleState *state);

#endif
