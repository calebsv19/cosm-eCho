#include "mem_console_workspace_authoring.h"

#include <stdio.h>
#include <string.h>

#include "mem_console_app_internal.h"

static uint32_t mem_console_workspace_authoring_mod_bits(SDL_Keymod mods) {
    uint32_t bits = 0u;
    if ((mods & KMOD_SHIFT) != 0) bits |= KIT_WORKSPACE_AUTHORING_MOD_SHIFT;
    if ((mods & KMOD_ALT) != 0) bits |= KIT_WORKSPACE_AUTHORING_MOD_ALT;
    if ((mods & KMOD_CTRL) != 0) bits |= KIT_WORKSPACE_AUTHORING_MOD_CTRL;
    if ((mods & KMOD_GUI) != 0) bits |= KIT_WORKSPACE_AUTHORING_MOD_GUI;
    return bits;
}

static KitWorkspaceAuthoringKey mem_console_workspace_authoring_key_from_sdl_keysym(
    const SDL_Keysym *keysym) {
    if (!keysym) {
        return KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    }
    switch (keysym->scancode) {
        case SDL_SCANCODE_C:
            return KIT_WORKSPACE_AUTHORING_KEY_C;
        case SDL_SCANCODE_V:
            return KIT_WORKSPACE_AUTHORING_KEY_V;
        default:
            break;
    }
    switch (keysym->sym) {
        case SDLK_TAB:
            return KIT_WORKSPACE_AUTHORING_KEY_TAB;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            return KIT_WORKSPACE_AUTHORING_KEY_ENTER;
        case SDLK_ESCAPE:
            return KIT_WORKSPACE_AUTHORING_KEY_ESCAPE;
        case SDLK_c:
            return KIT_WORKSPACE_AUTHORING_KEY_C;
        case SDLK_v:
            return KIT_WORKSPACE_AUTHORING_KEY_V;
        default:
            return KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    }
}

static void mem_console_workspace_authoring_note_consumed(MemConsoleWorkspaceAuthoringHost *host,
                                                          int runtime_event) {
    if (!host) {
        return;
    }
    host->last_event_consumed = 1u;
    host->consumed_event_count += 1u;
    if (runtime_event) {
        host->captured_runtime_event_count += 1u;
    }
}

static void mem_console_workspace_authoring_set_status(MemConsoleWorkspaceAuthoringHost *host,
                                                       const char *status) {
    if (!host) {
        return;
    }
    if (!status) {
        status = "";
    }
    (void)snprintf(host->status_text, sizeof(host->status_text), "%s", status);
}

static void mem_console_workspace_authoring_clear_event_flags(MemConsoleWorkspaceAuthoringHost *host) {
    if (!host) {
        return;
    }
    host->last_event_consumed = 0u;
    host->last_event_entered = 0u;
    host->last_event_exited = 0u;
    host->last_event_accepted = 0u;
    host->last_event_canceled = 0u;
}

static void mem_console_workspace_authoring_apply_render_state(MemConsoleState *state,
                                                              KitRenderContext *render_ctx,
                                                              KitUiContext *ui_ctx) {
    if (!state || !render_ctx || !ui_ctx) {
        return;
    }
    (void)kit_render_set_theme_preset(render_ctx, state->theme_preset_id);
    (void)kit_render_set_font_preset(render_ctx, state->font_preset_id);
    (void)kit_render_set_text_zoom_step(render_ctx, state->text_zoom_step);
    (void)kit_ui_style_apply_theme_scale(ui_ctx);
    mem_console_app_apply_compact_ui_density(ui_ctx, render_ctx);
    mem_console_redraw_mark(state,
                            MEM_CONSOLE_REDRAW_REASON_THEME |
                                MEM_CONSOLE_REDRAW_REASON_LAYOUT |
                                MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

static void mem_console_workspace_authoring_capture_baseline(MemConsoleWorkspaceAuthoringHost *host,
                                                             const MemConsoleState *state) {
    if (!host || !state) {
        return;
    }
    host->baseline_theme_preset_id = (int)state->theme_preset_id;
    host->baseline_font_preset_id = (int)state->font_preset_id;
    host->baseline_text_zoom_step = state->text_zoom_step;
    host->font_theme_baseline_valid = 1u;
}

static void mem_console_workspace_authoring_restore_baseline(MemConsoleWorkspaceAuthoringHost *host,
                                                            MemConsoleState *state,
                                                            KitRenderContext *render_ctx,
                                                            KitUiContext *ui_ctx) {
    if (!host || !state || !host->font_theme_baseline_valid) {
        return;
    }
    (void)state_set_theme_preset(state, (CoreThemePresetId)host->baseline_theme_preset_id);
    (void)state_set_font_preset(state, (CoreFontPresetId)host->baseline_font_preset_id);
    (void)state_set_text_zoom_step(state, host->baseline_text_zoom_step);
    mem_console_pane_prefs_mark_clean(state);
    mem_console_workspace_authoring_apply_render_state(state, render_ctx, ui_ctx);
    host->font_theme_pending_changes = 0u;
}

void mem_console_workspace_authoring_host_reset(MemConsoleWorkspaceAuthoringHost *host) {
    if (!host) {
        return;
    }
    memset(host, 0, sizeof(*host));
    host->overlay_mode = MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES;
    host->entry_chord_armed_key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
}

void mem_console_workspace_authoring_host_set_viewport(MemConsoleWorkspaceAuthoringHost *host,
                                                       uint32_t width,
                                                       uint32_t height) {
    if (!host) {
        return;
    }
    host->viewport_width = width;
    host->viewport_height = height;
}

int mem_console_workspace_authoring_host_active(const MemConsoleWorkspaceAuthoringHost *host) {
    return host && host->active ? 1 : 0;
}

int mem_console_workspace_authoring_host_pane_overlay_active(const MemConsoleWorkspaceAuthoringHost *host) {
    return mem_console_workspace_authoring_host_active(host) &&
           host->overlay_mode == MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES;
}

int mem_console_workspace_authoring_host_font_theme_overlay_active(
    const MemConsoleWorkspaceAuthoringHost *host) {
    return mem_console_workspace_authoring_host_active(host) &&
           host->overlay_mode == MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_FONT_THEME;
}

static void mem_console_workspace_authoring_enter(MemConsoleWorkspaceAuthoringHost *host,
                                                  MemConsoleState *state) {
    if (!host || !state) {
        return;
    }
    if (!host->active) {
        mem_console_workspace_authoring_capture_baseline(host, state);
        host->active = 1u;
        host->overlay_mode = MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES;
        host->font_theme_pending_changes = 0u;
        mem_console_workspace_authoring_set_status(host, "Authoring active.");
    }
    host->last_event_entered = 1u;
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_LAYOUT | MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

static void mem_console_workspace_authoring_apply(MemConsoleWorkspaceAuthoringHost *host,
                                                  MemConsoleState *state) {
    if (!host || !state) {
        return;
    }
    if (host->active) {
        host->active = 0u;
        host->last_event_accepted = 1u;
        host->font_theme_baseline_valid = 0u;
        host->font_theme_pending_changes = 0u;
        mem_console_pane_prefs_mark_dirty(state);
        mem_console_app_set_statusf(state, "Authoring applied.");
    }
    host->key_c_down = 0u;
    host->key_v_down = 0u;
    host->entry_chord_armed_key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    host->overlay_mode = MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES;
    host->last_event_exited = 1u;
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_LAYOUT | MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

static void mem_console_workspace_authoring_cancel(MemConsoleWorkspaceAuthoringHost *host,
                                                   MemConsoleState *state,
                                                   KitRenderContext *render_ctx,
                                                   KitUiContext *ui_ctx) {
    if (!host || !state) {
        return;
    }
    if (host->active) {
        mem_console_workspace_authoring_restore_baseline(host, state, render_ctx, ui_ctx);
        host->active = 0u;
        host->last_event_canceled = 1u;
        mem_console_app_set_statusf(state, "Authoring canceled.");
    }
    host->key_c_down = 0u;
    host->key_v_down = 0u;
    host->entry_chord_armed_key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    host->overlay_mode = MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES;
    host->last_event_exited = 1u;
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_LAYOUT | MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

void mem_console_workspace_authoring_host_cancel_active_preview(MemConsoleWorkspaceAuthoringHost *host,
                                                               MemConsoleState *state,
                                                               KitRenderContext *render_ctx,
                                                               KitUiContext *ui_ctx) {
    if (!host || !host->active) {
        return;
    }
    mem_console_workspace_authoring_cancel(host, state, render_ctx, ui_ctx);
}

static void mem_console_workspace_authoring_cycle_overlay(MemConsoleWorkspaceAuthoringHost *host,
                                                          MemConsoleState *state) {
    if (!host || !host->active) {
        return;
    }
    host->overlay_mode = host->overlay_mode == MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES
                             ? MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_FONT_THEME
                             : MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES;
    host->overlay_cycle_count += 1u;
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_LAYOUT | MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

static int mem_console_workspace_authoring_apply_overlay_button(MemConsoleWorkspaceAuthoringHost *host,
                                                                MemConsoleState *state,
                                                                KitRenderContext *render_ctx,
                                                                KitUiContext *ui_ctx,
                                                                KitWorkspaceAuthoringOverlayButtonId button_id) {
    if (!host || !state || !host->active) {
        return 0;
    }
    host->last_overlay_button_id = (uint32_t)button_id;
    switch (button_id) {
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_MODE:
            mem_console_workspace_authoring_cycle_overlay(host, state);
            host->overlay_button_click_count += 1u;
            return 1;
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_APPLY:
            mem_console_workspace_authoring_apply(host, state);
            host->overlay_button_click_count += 1u;
            return 1;
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_CANCEL:
            mem_console_workspace_authoring_cancel(host, state, render_ctx, ui_ctx);
            host->overlay_button_click_count += 1u;
            return 1;
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_ADD:
            host->add_stub_count += 1u;
            host->overlay_button_click_count += 1u;
            mem_console_workspace_authoring_set_status(host, "Add module requested. Memory Console module insertion is not wired yet.");
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            return 1;
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_NONE:
        default:
            break;
    }
    return 0;
}

static int mem_console_workspace_authoring_apply_font_theme_button(MemConsoleWorkspaceAuthoringHost *host,
                                                                   MemConsoleState *state,
                                                                   KitRenderContext *render_ctx,
                                                                   KitUiContext *ui_ctx,
                                                                   KitWorkspaceAuthoringFontThemeButtonId button_id) {
    KitWorkspaceAuthoringFontThemeAction action;

    if (!host || !state || !render_ctx || !ui_ctx ||
        !mem_console_workspace_authoring_host_font_theme_overlay_active(host)) {
        return 0;
    }
    if (!kit_workspace_authoring_ui_font_theme_button_enabled(button_id)) {
        return 0;
    }

    action = kit_workspace_authoring_ui_font_theme_action_for_button(button_id);
    host->last_font_theme_button_id = (uint32_t)button_id;

    switch (action.type) {
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_TEXT_SIZE_DEC:
            (void)state_adjust_text_zoom_step(state, -1);
            mem_console_workspace_authoring_set_status(host, "Text size decreased.");
            break;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_TEXT_SIZE_INC:
            (void)state_adjust_text_zoom_step(state, 1);
            mem_console_workspace_authoring_set_status(host, "Text size increased.");
            break;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_TEXT_SIZE_RESET:
            (void)state_reset_text_zoom_step(state);
            mem_console_workspace_authoring_set_status(host, "Text size reset.");
            break;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_SET_FONT_PRESET:
            if (!state_set_font_preset(state, action.font_preset_id)) {
                return 0;
            }
            mem_console_workspace_authoring_set_status(host, "Font preset previewed.");
            break;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_SET_THEME_PRESET:
            if (!state_set_theme_preset(state, action.theme_preset_id)) {
                return 0;
            }
            mem_console_workspace_authoring_set_status(host, "Theme preset previewed.");
            break;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_CUSTOM_THEME_STATUS:
            mem_console_workspace_authoring_set_status(host, action.custom_status_text);
            break;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_NONE:
        default:
            return 0;
    }

    host->font_theme_button_click_count += 1u;
    host->font_theme_pending_changes += 1u;
    mem_console_workspace_authoring_apply_render_state(state, render_ctx, ui_ctx);
    return 1;
}

static int mem_console_workspace_authoring_handle_overlay_click(MemConsoleWorkspaceAuthoringHost *host,
                                                                MemConsoleState *state,
                                                                KitRenderContext *render_ctx,
                                                                KitUiContext *ui_ctx,
                                                                int x,
                                                                int y) {
    KitWorkspaceAuthoringOverlayButton buttons[4];
    KitWorkspaceAuthoringOverlayButtonId hit = KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_NONE;
    uint32_t count;

    if (!host || !host->active || host->viewport_width == 0u) {
        return 0;
    }

    count = kit_workspace_authoring_ui_build_overlay_buttons(
        (int)host->viewport_width,
        1,
        mem_console_workspace_authoring_host_pane_overlay_active(host),
        buttons,
        (uint32_t)(sizeof(buttons) / sizeof(buttons[0])));
    hit = kit_workspace_authoring_ui_overlay_hit_test(buttons, count, (float)x, (float)y);
    return mem_console_workspace_authoring_apply_overlay_button(host, state, render_ctx, ui_ctx, hit);
}

static int mem_console_workspace_authoring_handle_font_theme_click(MemConsoleWorkspaceAuthoringHost *host,
                                                                   MemConsoleState *state,
                                                                   KitRenderContext *render_ctx,
                                                                   KitUiContext *ui_ctx,
                                                                   int x,
                                                                   int y) {
    KitWorkspaceAuthoringFontThemeLayout layout;
    KitWorkspaceAuthoringFontThemeButtonId hit;

    if (!host || !host->active || host->viewport_width == 0u || host->viewport_height == 0u ||
        !mem_console_workspace_authoring_host_font_theme_overlay_active(host)) {
        return 0;
    }
    if (!kit_workspace_authoring_ui_font_theme_build_layout(render_ctx,
                                                            (int)host->viewport_width,
                                                            (int)host->viewport_height,
                                                            &layout)) {
        return 0;
    }
    hit = kit_workspace_authoring_ui_font_theme_hit_button(&layout, (float)x, (float)y);
    return mem_console_workspace_authoring_apply_font_theme_button(host, state, render_ctx, ui_ctx, hit);
}

int mem_console_workspace_authoring_host_handle_sdl_event(MemConsoleWorkspaceAuthoringHost *host,
                                                          MemConsoleState *state,
                                                          KitRenderContext *render_ctx,
                                                          KitUiContext *ui_ctx,
                                                          const SDL_Event *event,
                                                          int text_entry_active) {
    KitWorkspaceAuthoringKey key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    uint32_t mod_bits = 0u;

    if (!host || !state || !event) {
        return 0;
    }

    mem_console_workspace_authoring_clear_event_flags(host);

    if (event->type == SDL_KEYUP) {
        key = mem_console_workspace_authoring_key_from_sdl_keysym(&event->key.keysym);
        if (key == KIT_WORKSPACE_AUTHORING_KEY_C) {
            host->key_c_down = 0u;
        } else if (key == KIT_WORKSPACE_AUTHORING_KEY_V) {
            host->key_v_down = 0u;
        }
        if (host->entry_chord_armed_key == (uint8_t)key) {
            host->entry_chord_armed_key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
        }
        return host->active ? 1 : 0;
    }

    if (event->type == SDL_MOUSEBUTTONDOWN &&
        event->button.button == SDL_BUTTON_LEFT &&
        host->active) {
        int overlay_hit = mem_console_workspace_authoring_handle_overlay_click(host,
                                                                               state,
                                                                               render_ctx,
                                                                               ui_ctx,
                                                                               event->button.x,
                                                                               event->button.y);
        int font_theme_hit = 0;
        if (!overlay_hit) {
            font_theme_hit = mem_console_workspace_authoring_handle_font_theme_click(host,
                                                                                    state,
                                                                                    render_ctx,
                                                                                    ui_ctx,
                                                                                    event->button.x,
                                                                                    event->button.y);
        }
        mem_console_workspace_authoring_note_consumed(host, (overlay_hit || font_theme_hit) ? 0 : 1);
        return 1;
    }

    if (event->type != SDL_KEYDOWN) {
        if (host->active) {
            mem_console_workspace_authoring_note_consumed(host, 1);
            return 1;
        }
        return 0;
    }

    key = mem_console_workspace_authoring_key_from_sdl_keysym(&event->key.keysym);
    mod_bits = mem_console_workspace_authoring_mod_bits((SDL_Keymod)event->key.keysym.mod);

    if (key == KIT_WORKSPACE_AUTHORING_KEY_C) {
        host->key_c_down = 1u;
    } else if (key == KIT_WORKSPACE_AUTHORING_KEY_V) {
        host->key_v_down = 1u;
    }

    if (!text_entry_active &&
        (mod_bits & KIT_WORKSPACE_AUTHORING_MOD_ALT) != 0u &&
        (mod_bits & (KIT_WORKSPACE_AUTHORING_MOD_SHIFT |
                     KIT_WORKSPACE_AUTHORING_MOD_CTRL |
                     KIT_WORKSPACE_AUTHORING_MOD_GUI)) == 0u &&
        (key == KIT_WORKSPACE_AUTHORING_KEY_C || key == KIT_WORKSPACE_AUTHORING_KEY_V)) {
        if (!kit_workspace_authoring_entry_chord_pressed(key,
                                                         mod_bits,
                                                         host->key_c_down,
                                                         host->key_v_down)) {
            mem_console_workspace_authoring_note_consumed(host, 0);
            return 1;
        }
        if (host->entry_chord_armed_key == (uint8_t)key) {
            mem_console_workspace_authoring_note_consumed(host, 0);
            return 1;
        }
        host->entry_chord_armed_key = (uint8_t)key;
        if (host->active) {
            mem_console_workspace_authoring_cancel(host, state, render_ctx, ui_ctx);
        } else {
            mem_console_workspace_authoring_enter(host, state);
        }
        mem_console_workspace_authoring_note_consumed(host, 0);
        return 1;
    }

    if (!host->active) {
        return 0;
    }

    switch (key) {
        case KIT_WORKSPACE_AUTHORING_KEY_TAB:
            mem_console_workspace_authoring_cycle_overlay(host, state);
            mem_console_workspace_authoring_note_consumed(host, 1);
            return 1;
        case KIT_WORKSPACE_AUTHORING_KEY_ENTER:
            mem_console_workspace_authoring_apply(host, state);
            mem_console_workspace_authoring_note_consumed(host, 1);
            return 1;
        case KIT_WORKSPACE_AUTHORING_KEY_ESCAPE:
            mem_console_workspace_authoring_cancel(host, state, render_ctx, ui_ctx);
            mem_console_workspace_authoring_note_consumed(host, 1);
            return 1;
        default:
            mem_console_workspace_authoring_note_consumed(host, 1);
            return 1;
    }
}
