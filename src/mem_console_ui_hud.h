#ifndef MEM_CONSOLE_UI_HUD_H
#define MEM_CONSOLE_UI_HUD_H

#include "mem_console_state.h"
#include "kit_ui.h"

typedef struct MemConsoleUiHudRowSpec {
    const char *text;
    CoreThemeColorToken token;
    CoreFontRoleId font_role;
    CoreFontTextSizeTier text_tier;
    int max_lines;
} MemConsoleUiHudRowSpec;

typedef struct MemConsoleUiHudCardSpec {
    uint64_t cache_key;
    float width_ratio;
    float min_width;
    float max_width;
    float edge_margin;
    int row_count;
    MemConsoleUiHudRowSpec rows[MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT];
} MemConsoleUiHudCardSpec;

CoreResult mem_console_ui_hud_draw_cached(const KitRenderContext *render_ctx,
                                          KitUiContext *ui_ctx,
                                          KitRenderFrame *frame,
                                          KitRenderRect bounds,
                                          MemConsoleState *state,
                                          const MemConsoleUiHudCardSpec *spec);

#endif
