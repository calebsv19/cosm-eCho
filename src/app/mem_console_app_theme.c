#include "mem_console_app_internal.h"

void mem_console_app_apply_compact_ui_density(KitUiContext *ui_ctx,
                                              const KitRenderContext *render_ctx) {
    float zoom_scale = 1.0f;
    float target_padding = 4.0f;
    float target_gap = 3.0f;
    float target_control_height = 20.0f;

    if (!ui_ctx) {
        return;
    }

    if (render_ctx) {
        zoom_scale = (float)kit_render_text_zoom_percent(render_ctx) / 100.0f;
    }
    if (zoom_scale < 0.6f) zoom_scale = 0.6f;
    if (zoom_scale > 1.8f) zoom_scale = 1.8f;

    target_padding *= zoom_scale;
    target_gap *= zoom_scale;
    target_control_height *= zoom_scale;

    if (target_padding < 3.0f) target_padding = 3.0f;
    if (target_padding > 8.0f) target_padding = 8.0f;
    if (target_gap < 2.0f) target_gap = 2.0f;
    if (target_gap > 6.0f) target_gap = 6.0f;
    if (target_control_height < 20.0f) target_control_height = 20.0f;
    if (target_control_height > 34.0f) target_control_height = 34.0f;

    ui_ctx->style.padding = target_padding;
    ui_ctx->style.gap = target_gap;
    ui_ctx->style.control_height = target_control_height;
}

/* The existing cycle order remains owned by state_core. Restore the proposed
 * identity before preparing fonts; only the transaction commits it. */
int mem_console_app_handle_theme_shortcut(KitRenderContext *r, KitUiContext *ui,
    MemConsoleState *s, const char *prefs_path, SDL_Keycode key) {
    if (!r || !ui || !s || (key != SDLK_t && key != SDLK_y)) return 0;
    CoreThemePresetId old = s->theme_preset_id;
    (void)cycle_theme_preset(s, key == SDLK_t ? 1 : -1);
    CoreThemePresetId next = s->theme_preset_id;
    (void)state_set_theme_preset(s, old);
    if (!mem_console_app_apply_text_settings(r, ui, s, next, s->font_preset_id, s->text_zoom_step)) {
        mem_console_app_set_statusf(s, "Theme switch failed; previous settings retained.");
        return 1;
    }
    (void)prefs_path;
    mem_console_pane_prefs_mark_dirty(s);
    mem_console_app_set_statusf(s, "Theme switched to %s.", s->theme_name);
    return 1;
}

int mem_console_app_handle_font_shortcut(KitRenderContext *r, KitUiContext *ui,
    MemConsoleState *s, const char *prefs_path, SDL_Keycode key) {
    if (!r || !ui || !s || (key != SDLK_u && key != SDLK_i)) return 0;
    CoreFontPresetId old = s->font_preset_id;
    (void)cycle_font_preset(s, key == SDLK_u ? 1 : -1);
    CoreFontPresetId next = s->font_preset_id;
    (void)state_set_font_preset(s, old);
    if (!mem_console_app_apply_text_settings(r, ui, s, s->theme_preset_id, next, s->text_zoom_step)) {
        mem_console_app_set_statusf(s, "Font switch failed; previous settings retained.");
        return 1;
    }
    (void)prefs_path;
    mem_console_pane_prefs_mark_dirty(s);
    mem_console_app_set_statusf(s, "Font switched to %s.", s->font_name);
    return 1;
}

int mem_console_app_handle_text_zoom_shortcut(KitRenderContext *r, KitUiContext *ui,
    MemConsoleState *s, const char *prefs_path, SDL_Keycode key) {
    int zoom;
    if (!r || !ui || !s) return 0;
    zoom = s->text_zoom_step;
    if (key == SDLK_EQUALS || key == SDLK_PLUS || key == SDLK_KP_PLUS) ++zoom;
    else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) --zoom;
    else if (key == SDLK_0 || key == SDLK_KP_0) zoom = 0;
    else return 0;
    if (!mem_console_app_apply_text_settings(r, ui, s, s->theme_preset_id, s->font_preset_id, zoom)) {
        mem_console_app_set_statusf(s, "Text zoom update failed; previous settings retained.");
        return 1;
    }
    (void)prefs_path;
    mem_console_pane_prefs_mark_dirty(s);
    mem_console_app_set_statusf(s, "Text zoom set to %d%%.", kit_render_text_zoom_percent(r));
    return 1;
}
