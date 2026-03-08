#ifndef MEM_CONSOLE_UI_LEFT_SECTION_H
#define MEM_CONSOLE_UI_LEFT_SECTION_H

#include "mem_console_layout_config.h"
#include "mem_console_ui.h"

CoreResult mem_console_ui_draw_left_section(KitRenderContext *render_ctx,
                                            KitUiContext *ui_ctx,
                                            KitRenderFrame *frame,
                                            MemConsoleState *state,
                                            const KitUiInputState *input,
                                            const MemConsoleLayoutConfig *layout_cfg,
                                            int wheel_y,
                                            int has_any_edit_mode,
                                            MemConsoleAction *io_action);

#endif
