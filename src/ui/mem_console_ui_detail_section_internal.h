#ifndef MEM_CONSOLE_UI_DETAIL_SECTION_INTERNAL_H
#define MEM_CONSOLE_UI_DETAIL_SECTION_INTERNAL_H

#include "mem_console_ui_detail_section.h"

void mem_console_ui_detail_refresh_reference_path_cache(MemConsoleState *state);
CoreResult mem_console_ui_detail_draw_relationships(KitRenderContext *render_ctx,
                                                    KitUiContext *ui_ctx,
                                                    KitRenderFrame *frame,
                                                    MemConsoleState *state,
                                                    const KitUiInputState *input,
                                                    int wheel_y,
                                                    const MemConsoleLayoutConfig *layout_cfg,
                                                    MemConsoleAction *io_action);

#endif
