#ifndef MEM_CONSOLE_UI_DETAIL_SECTION_H
#define MEM_CONSOLE_UI_DETAIL_SECTION_H

#include "mem_console_layout_config.h"
#include "mem_console_ui.h"

CoreResult mem_console_ui_draw_detail_section(KitRenderContext *render_ctx,
                                              KitUiContext *ui_ctx,
                                              KitRenderFrame *frame,
                                              MemConsoleState *state,
                                              const KitUiInputState *input,
                                              int wheel_y,
                                              const MemConsoleLayoutConfig *layout_cfg,
                                              KitUiStackLayout *out_right_layout);

#endif
