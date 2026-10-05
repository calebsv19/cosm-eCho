#ifndef MEM_CONSOLE_UI_SURFACE_H
#define MEM_CONSOLE_UI_SURFACE_H
#include "mem_console_state.h"
#include "kit_ui_surface.h"
enum {
    MC_BUTTON_LEFT=1, MC_BUTTON_PROJECT, MC_BUTTON_ITEM,
    MC_BUTTON_REL_ADD, MC_BUTTON_REL_NAV, MC_BUTTON_REL_KIND, MC_BUTTON_REL_DELETE,
    MC_BUTTON_DB_ROW, MC_BUTTON_DB_ACTION, MC_BUTTON_GRAPH_SETTING,
    MC_BUTTON_GRAPH_HOP, MC_BUTTON_GRAPH_ROLE, MC_BUTTON_ACTION,
    MC_BUTTON_LEGEND, MC_BUTTON_FONT, MC_BUTTON_OVERLAY
};
uint32_t mem_console_ui_surface_scope(const MemConsoleState *state);
uint64_t mem_console_ui_surface_string_key(const char *text);
KitUiButtonResult mem_console_ui_surface_button(KitUiContext *ui, MemConsoleState *state,
    uint32_t domain, uint64_t key, KitRenderRect bounds, int enabled);
void mem_console_ui_surface_text_focus(MemConsoleState *state);
int mem_console_ui_surface_event(MemConsoleState *state, KitRenderContext *render,
    KitUiContext *ui, const SDL_Event *event, KitUiInputState *input);
#endif
