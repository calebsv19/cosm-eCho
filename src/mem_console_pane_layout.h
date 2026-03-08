#ifndef MEM_CONSOLE_PANE_LAYOUT_H
#define MEM_CONSOLE_PANE_LAYOUT_H

#include "mem_console_layout_config.h"
#include "mem_console_types.h"

typedef enum MemConsolePaneId {
    MEM_CONSOLE_PANE_LEFT_NAV = 1,
    MEM_CONSOLE_PANE_RIGHT_DETAIL = 2,
    MEM_CONSOLE_PANE_RIGHT_GRAPH = 3
} MemConsolePaneId;

typedef enum MemConsolePaneSplitterId {
    MEM_CONSOLE_PANE_SPLITTER_NONE = 0,
    MEM_CONSOLE_PANE_SPLITTER_LEFT_RIGHT = 1,
    MEM_CONSOLE_PANE_SPLITTER_RIGHT_STACK = 2
} MemConsolePaneSplitterId;

CoreResult mem_console_pane_layout_compute(MemConsoleState *state,
                                           const MemConsoleLayoutConfig *layout_cfg,
                                           int frame_width,
                                           int frame_height);

CoreResult mem_console_pane_layout_get(const MemConsoleState *state,
                                       MemConsolePaneId pane_id,
                                       KitRenderRect *out_bounds);

CoreResult mem_console_pane_layout_get_splitter_bounds(const MemConsoleState *state,
                                                       const MemConsoleLayoutConfig *layout_cfg,
                                                       MemConsolePaneSplitterId splitter_id,
                                                       KitRenderRect *out_bounds);

int mem_console_pane_layout_begin_drag(MemConsoleState *state,
                                       const MemConsoleLayoutConfig *layout_cfg,
                                       int frame_width,
                                       int frame_height,
                                       float mouse_x,
                                       float mouse_y);

int mem_console_pane_layout_update_drag(MemConsoleState *state,
                                        const MemConsoleLayoutConfig *layout_cfg,
                                        int frame_width,
                                        int frame_height,
                                        float mouse_x,
                                        float mouse_y);

void mem_console_pane_layout_end_drag(MemConsoleState *state);

#endif
