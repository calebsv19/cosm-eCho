#ifndef MEM_CONSOLE_UI_GRAPH_PANEL_H
#define MEM_CONSOLE_UI_GRAPH_PANEL_H

#include "mem_console_layout_config.h"
#include "mem_console_ui.h"

CoreResult mem_console_ui_draw_graph_panel(KitRenderContext *render_ctx,
                                           KitUiContext *ui_ctx,
                                           KitRenderFrame *frame,
                                           MemConsoleState *state,
                                           const KitUiInputState *input,
                                           const MemConsoleLayoutConfig *layout_cfg,
                                           KitUiStackLayout *right_layout,
                                           int wheel_y,
                                           MemConsoleAction *io_action);

#endif
