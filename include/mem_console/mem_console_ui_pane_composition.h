#ifndef MEM_CONSOLE_UI_PANE_COMPOSITION_H
#define MEM_CONSOLE_UI_PANE_COMPOSITION_H
#include "mem_console_types.h"
#include "kit_pane_composition.h"
CoreResult mem_console_ui_panes_build(const MemConsoleState *state,int width,int height,KitPaneComposition *view);
KitUiInputState mem_console_ui_pane_input(const KitUiInputState *input,CorePaneId owner,CorePaneId pane);
CoreResult mem_console_ui_leaf_begin(KitUiContext *ui, KitRenderFrame *frame, const MemConsoleState *state, CorePaneId id);
KitUiInputState mem_console_ui_leaf_input(const MemConsoleState *state, const KitUiInputState *input, CorePaneId id);
#endif
