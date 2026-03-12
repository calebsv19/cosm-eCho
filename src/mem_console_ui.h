#ifndef MEM_CONSOLE_UI_H
#define MEM_CONSOLE_UI_H

#include "kit_ui.h"
#include "mem_console_state.h"

#define MEM_CONSOLE_FRAME_OK 0
#define MEM_CONSOLE_FRAME_FATAL 1
#define MEM_CONSOLE_FRAME_RECOVERABLE 2

int run_frame(KitRenderContext *render_ctx,
              KitUiContext *ui_ctx,
              MemConsoleState *state,
              const KitUiInputState *input,
              int frame_width,
              int frame_height,
              int wheel_y,
              MemConsoleAction *out_action);

#endif
