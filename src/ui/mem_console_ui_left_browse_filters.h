#ifndef MEM_CONSOLE_UI_LEFT_BROWSE_FILTERS_H
#define MEM_CONSOLE_UI_LEFT_BROWSE_FILTERS_H

#include "mem_console_ui_left_section.h"

CoreResult mem_console_ui_left_draw_browse_filters(KitRenderContext *render_ctx,
                                                   KitUiContext *ui_ctx,
                                                   KitRenderFrame *frame,
                                                   MemConsoleState *state,
                                                   const KitUiInputState *input,
                                                   KitRenderRect bounds,
                                                   int controls_enabled,
                                                   MemConsoleAction *io_action);

#endif
