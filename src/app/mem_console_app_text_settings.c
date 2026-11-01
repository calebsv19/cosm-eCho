#include "mem_console_app_internal.h"

int mem_console_app_prepare_initial_text(KitRenderContext *render) {
    KitRenderContext next;
    if (!render || kit_render_prepare_text_context(render, render->font.id,
            kit_render_text_zoom_step(render), 1, &next).code != CORE_OK) return 0;
    kit_render_context_shutdown(render);
    *render = next;
    return 1;
}

int mem_console_app_apply_text_settings(KitRenderContext *render, KitUiContext *ui,
    MemConsoleState *state, CoreThemePresetId theme, CoreFontPresetId font, int zoom) {
    KitRenderContext next;
    CoreThemePreset checked_theme;
    CoreFontPreset checked_font;
    MemConsoleWorkspaceAuthoringHost *host;
    if (!render || !ui || !state || render->frame_open ||
        core_theme_get_preset(theme, &checked_theme).code != CORE_OK ||
        core_font_get_preset(font, &checked_font).code != CORE_OK) return 0;
    if (zoom < -4) zoom = -4;
    if (zoom > 5) zoom = 5;
    host = &state->workspace_authoring;
    if (font != render->font.id || zoom != kit_render_text_zoom_step(render)) {
        if (kit_render_prepare_text_context(render, font, zoom, 1, &next).code != CORE_OK)
            return 0;
        next.theme = checked_theme;
        /* Move the entry bank once. Cancel never opens entry fonts again. */
        if (host->active && host->font_theme_baseline_valid && !host->baseline_render_valid) {
            host->baseline_render = *render;
            host->baseline_render_valid = 1;
        } else kit_render_context_shutdown(render);
        *render = next;
    } else render->theme = checked_theme;
    (void)state_set_theme_preset(state, theme);
    (void)state_set_font_preset(state, font);
    (void)state_set_text_zoom_step(state, zoom);
    (void)kit_ui_style_apply_theme_scale(ui);
    mem_console_app_apply_compact_ui_density(ui, render);
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_THEME |
        MEM_CONSOLE_REDRAW_REASON_LAYOUT | MEM_CONSOLE_REDRAW_REASON_CONTENT);
    return 1;
}
