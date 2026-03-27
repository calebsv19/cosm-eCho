#include "mem_console_app_internal.h"

#include <stdio.h>

void mem_console_app_apply_compact_ui_density(KitUiContext *ui_ctx) {
    if (!ui_ctx) {
        return;
    }

    if (ui_ctx->style.padding > 6.0f) {
        ui_ctx->style.padding = 6.0f;
    }
    if (ui_ctx->style.padding < 3.0f) {
        ui_ctx->style.padding = 3.0f;
    }

    if (ui_ctx->style.gap > 4.0f) {
        ui_ctx->style.gap = 4.0f;
    }
    if (ui_ctx->style.gap < 2.0f) {
        ui_ctx->style.gap = 2.0f;
    }

    if (ui_ctx->style.control_height > 20.0f) {
        ui_ctx->style.control_height = 20.0f;
    }
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
    mem_console_app_apply_compact_ui_density(ui_ctx);
    (void)prefs_path;
    state->pane_prefs_dirty = 1;

    (void)snprintf(state->status_line, sizeof(state->status_line), "Theme switched to %s.", state->theme_name);
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_THEME | MEM_CONSOLE_REDRAW_REASON_CONTENT);
    return 1;
}

int mem_console_app_handle_font_shortcut(KitRenderContext *render_ctx,
                                         MemConsoleState *state,
                                         const char *prefs_path,
                                         SDL_Keycode keycode) {
    int direction;
    CoreResult result;

    if (!render_ctx || !state) {
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

    (void)prefs_path;
    state->pane_prefs_dirty = 1;

    (void)snprintf(state->status_line, sizeof(state->status_line), "Font switched to %s.", state->font_name);
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_THEME | MEM_CONSOLE_REDRAW_REASON_CONTENT);
    return 1;
}
