#ifndef MEM_CONSOLE_UI_GRAPH_CONTROLS_H
#define MEM_CONSOLE_UI_GRAPH_CONTROLS_H

#include "mem_console_layout_config.h"
#include "mem_console_ui.h"

CoreResult mem_console_ui_draw_graph_controls(KitRenderContext *render_ctx,
                                              KitUiContext *ui_ctx,
                                              KitRenderFrame *frame,
                                              MemConsoleState *state,
                                              const KitUiInputState *input,
                                              const MemConsoleLayoutConfig *layout_cfg,
                                              int has_any_edit_mode,
                                              KitUiStackLayout *right_layout,
                                              MemConsoleAction *io_action);

#endif
