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

int mem_console_ui_left_begin_panel_drag(MemConsoleState *state,
                                         const MemConsoleLayoutConfig *layout_cfg,
                                         float ui_gap,
                                         float mouse_x,
                                         float mouse_y);
int mem_console_ui_left_update_panel_drag(MemConsoleState *state,
                                          const MemConsoleLayoutConfig *layout_cfg,
                                          float ui_gap,
                                          float mouse_y);
void mem_console_ui_left_end_panel_drag(MemConsoleState *state);

#endif
