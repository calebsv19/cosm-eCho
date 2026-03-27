#include "mem_console_app_internal.h"

#include <stdio.h>

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

int mem_console_app_handle_theme_shortcut(KitRenderContext *render_ctx,
                                          KitUiContext *ui_ctx,
                                          MemConsoleState *state,
                                          const char *prefs_path,
                                          SDL_Keycode keycode) {
    int direction;
    CoreResult result;

    if (!render_ctx || !ui_ctx || !state) {
        return 0;
    }

    if (keycode == SDLK_t) {
        direction = 1;
    } else if (keycode == SDLK_y) {
        direction = -1;
    } else {
        return 0;
    }

    if (!cycle_theme_preset(state, direction)) {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Theme switch failed.");
        return 1;
    }

    result = kit_render_set_theme_preset(render_ctx, state->theme_preset_id);
    if (result.code != CORE_OK) {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Theme switch failed.");
        return 1;
    }

    (void)kit_ui_style_apply_theme_scale(ui_ctx);
    mem_console_app_apply_compact_ui_density(ui_ctx, render_ctx);
    (void)prefs_path;
    state->pane_prefs_dirty = 1;

    (void)snprintf(state->status_line, sizeof(state->status_line), "Theme switched to %s.", state->theme_name);
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_THEME | MEM_CONSOLE_REDRAW_REASON_CONTENT);
    return 1;
}

int mem_console_app_handle_font_shortcut(KitRenderContext *render_ctx,
                                         KitUiContext *ui_ctx,
                                         MemConsoleState *state,
                                         const char *prefs_path,
                                         SDL_Keycode keycode) {
    int direction;
    CoreResult result;

    if (!render_ctx || !ui_ctx || !state) {
        return 0;
    }

    if (keycode == SDLK_u) {
        direction = 1;
    } else if (keycode == SDLK_i) {
        direction = -1;
    } else {
        return 0;
    }

    if (!cycle_font_preset(state, direction)) {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Font switch failed.");
        return 1;
    }

    result = kit_render_set_font_preset(render_ctx, state->font_preset_id);
    if (result.code != CORE_OK) {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Font switch failed.");
        return 1;
    }

    (void)kit_ui_style_apply_theme_scale(ui_ctx);
    mem_console_app_apply_compact_ui_density(ui_ctx, render_ctx);
    (void)prefs_path;
    state->pane_prefs_dirty = 1;

    (void)snprintf(state->status_line, sizeof(state->status_line), "Font switched to %s.", state->font_name);
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_THEME | MEM_CONSOLE_REDRAW_REASON_CONTENT);
    return 1;
}

int mem_console_app_handle_text_zoom_shortcut(KitRenderContext *render_ctx,
                                              KitUiContext *ui_ctx,
                                              MemConsoleState *state,
                                              const char *prefs_path,
                                              SDL_Keycode keycode) {
    CoreResult result;
    int handled = 0;

    if (!render_ctx || !ui_ctx || !state) {
        return 0;
    }

    if (keycode == SDLK_EQUALS || keycode == SDLK_PLUS || keycode == SDLK_KP_PLUS) {
        result = kit_render_adjust_text_zoom_step(render_ctx, 1);
        handled = 1;
    } else if (keycode == SDLK_MINUS || keycode == SDLK_KP_MINUS) {
        result = kit_render_adjust_text_zoom_step(render_ctx, -1);
        handled = 1;
    } else if (keycode == SDLK_0 || keycode == SDLK_KP_0) {
        result = kit_render_reset_text_zoom_step(render_ctx);
        handled = 1;
    } else {
        return 0;
    }

    if (!handled || result.code != CORE_OK) {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Text zoom update failed.");
        return 1;
    }

    (void)state_set_text_zoom_step(state, kit_render_text_zoom_step(render_ctx));
    (void)kit_ui_style_apply_theme_scale(ui_ctx);
    mem_console_app_apply_compact_ui_density(ui_ctx, render_ctx);

    (void)prefs_path;
    state->pane_prefs_dirty = 1;

    (void)snprintf(state->status_line,
                   sizeof(state->status_line),
                   "Text zoom set to %d%%.",
                   kit_render_text_zoom_percent(render_ctx));
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_LAYOUT | MEM_CONSOLE_REDRAW_REASON_CONTENT);
    return 1;
}
