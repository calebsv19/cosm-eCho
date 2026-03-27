#ifndef MEM_CONSOLE_UI_CHROME_H
#define MEM_CONSOLE_UI_CHROME_H

#include "core_base.h"
#include "kit_render.h"
#include "mem_console_layout_config.h"
#include "mem_console_types.h"

CoreResult mem_console_ui_draw_root_chrome(const KitRenderContext *render_ctx,
                                           KitRenderFrame *frame,
                                           const MemConsoleState *state,
                                           const MemConsoleLayoutConfig *layout_cfg);

#endif
