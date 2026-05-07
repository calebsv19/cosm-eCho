#include "mem_console_workspace_authoring.h"

#include <stdio.h>

#include "mem_console_ui_common.h"

static KitRenderColor mem_console_authoring_alpha(KitRenderColor color, uint8_t alpha) {
    color.a = alpha;
    return color;
}

static CoreResult mem_console_authoring_resolve(KitRenderContext *render_ctx,
                                                CoreThemeColorToken token,
                                                KitRenderColor *out_color) {
    return mem_console_ui_resolve_theme_color(render_ctx, token, out_color);
}

static CoreResult mem_console_authoring_push_rect(KitRenderFrame *frame,
                                                  KitRenderRect rect,
                                                  KitRenderColor fill) {
    return kit_render_push_rect(frame,
                                &(KitRenderRectCommand){
                                    rect,
                                    0.0f,
                                    fill,
                                    kit_render_identity_transform()
                                });
}

static CoreResult mem_console_authoring_push_outline(KitRenderFrame *frame,
                                                     KitRenderRect rect,
                                                     KitRenderColor color,
                                                     float thickness) {
    CoreResult result;

    result = mem_console_authoring_push_rect(frame,
                                             (KitRenderRect){ rect.x, rect.y, rect.width, thickness },
                                             color);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_push_rect(frame,
                                             (KitRenderRect){ rect.x, rect.y + rect.height - thickness, rect.width, thickness },
                                             color);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_push_rect(frame,
                                             (KitRenderRect){ rect.x, rect.y, thickness, rect.height },
                                             color);
    if (result.code != CORE_OK) return result;
    return mem_console_authoring_push_rect(frame,
                                           (KitRenderRect){ rect.x + rect.width - thickness, rect.y, thickness, rect.height },
                                           color);
}

static CoreResult mem_console_authoring_draw_label(KitUiContext *ui_ctx,
                                                   KitRenderFrame *frame,
                                                   KitRenderRect rect,
                                                   const char *text,
                                                   CoreFontTextSizeTier tier) {
    return mem_console_ui_draw_info_line_custom(ui_ctx,
                                                frame,
                                                rect,
                                                text,
                                                CORE_THEME_COLOR_TEXT_PRIMARY,
                                                CORE_FONT_ROLE_UI_BOLD,
                                                tier);
}

static CoreResult mem_console_authoring_draw_section_text(KitUiContext *ui_ctx,
                                                          KitRenderFrame *frame,
                                                          KitRenderRect section,
                                                          const char *title,
                                                          const char *detail) {
    CoreResult result;
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  (KitRenderRect){ section.x + 12.0f, section.y + 10.0f, section.width - 24.0f, 24.0f },
                                                  title,
                                                  CORE_THEME_COLOR_TEXT_PRIMARY,
                                                  CORE_FONT_ROLE_UI_BOLD,
                                                  CORE_FONT_TEXT_SIZE_TITLE);
    if (result.code != CORE_OK) return result;
    return mem_console_ui_draw_info_line_custom(ui_ctx,
                                                frame,
                                                (KitRenderRect){ section.x + 12.0f, section.y + 36.0f, section.width - 24.0f, 22.0f },
                                                detail,
                                                CORE_THEME_COLOR_TEXT_MUTED,
                                                CORE_FONT_ROLE_UI_REGULAR,
                                                CORE_FONT_TEXT_SIZE_PARAGRAPH);
}

static int mem_console_authoring_font_theme_button_selected(const MemConsoleState *state,
                                                            KitWorkspaceAuthoringFontThemeButtonId button_id) {
    CoreFontPresetId font_id;
    CoreThemePresetId theme_id;

    if (!state) {
        return 0;
    }
    if (kit_workspace_authoring_ui_font_theme_button_font_preset_id(button_id, &font_id)) {
        return state->font_preset_id == font_id;
    }
    if (kit_workspace_authoring_ui_font_theme_button_theme_preset_id(button_id, &theme_id)) {
        return state->theme_preset_id == theme_id;
    }
    return 0;
}

static CoreResult mem_console_authoring_draw_button(KitRenderContext *render_ctx,
                                                    KitUiContext *ui_ctx,
                                                    KitRenderFrame *frame,
                                                    const MemConsoleState *state,
                                                    KitWorkspaceAuthoringFontThemeButtonId button_id,
                                                    KitRenderRect rect,
                                                    const char *override_label) {
    CoreResult result;
    KitRenderColor fill;
    KitRenderColor border;
    const char *label;
    int selected;

    if (!render_ctx || !ui_ctx || !frame || button_id == KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_NONE) {
        return core_result_ok();
    }

    result = mem_console_authoring_resolve(render_ctx, CORE_THEME_COLOR_SURFACE_2, &fill);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_resolve(render_ctx, CORE_THEME_COLOR_TEXT_MUTED, &border);
    if (result.code != CORE_OK) return result;

    selected = mem_console_authoring_font_theme_button_selected(state, button_id);
    if (selected) {
        result = mem_console_authoring_resolve(render_ctx, CORE_THEME_COLOR_ACCENT_PRIMARY, &fill);
        if (result.code != CORE_OK) return result;
    }
    if (!kit_workspace_authoring_ui_font_theme_button_enabled(button_id)) {
        fill = mem_console_authoring_alpha(fill, 120u);
        border = mem_console_authoring_alpha(border, 120u);
    }

    result = mem_console_authoring_push_rect(frame, rect, fill);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_push_outline(frame, rect, border, 1.0f);
    if (result.code != CORE_OK) return result;

    label = override_label ? override_label : kit_workspace_authoring_ui_font_theme_button_label(button_id);
    return mem_console_ui_draw_info_line_custom(ui_ctx,
                                                frame,
                                                rect,
                                                label,
                                                kit_workspace_authoring_ui_font_theme_button_enabled(button_id)
                                                    ? CORE_THEME_COLOR_TEXT_PRIMARY
                                                    : CORE_THEME_COLOR_TEXT_MUTED,
                                                CORE_FONT_ROLE_UI_REGULAR,
                                                CORE_FONT_TEXT_SIZE_PARAGRAPH);
}

static CoreResult mem_console_authoring_render_overlay_buttons(KitRenderContext *render_ctx,
                                                               KitRenderFrame *frame,
                                                               const MemConsoleWorkspaceAuthoringHost *host) {
    KitWorkspaceAuthoringOverlayButton buttons[4];
    uint32_t count;

    if (!host || !host->active) {
        return core_result_ok();
    }

    count = kit_workspace_authoring_ui_build_overlay_buttons((int)host->viewport_width,
                                                             1,
                                                             mem_console_workspace_authoring_host_pane_overlay_active(host),
                                                             buttons,
                                                             (uint32_t)(sizeof(buttons) / sizeof(buttons[0])));
    return kit_workspace_authoring_ui_draw_overlay_buttons(render_ctx,
                                                           frame,
                                                           buttons,
                                                           count,
                                                           KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_NONE,
                                                           KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_NONE);
}

static CoreResult mem_console_authoring_draw_pane(KitRenderContext *render_ctx,
                                                  KitUiContext *ui_ctx,
                                                  KitRenderFrame *frame,
                                                  KitRenderRect rect,
                                                  const char *label) {
    CoreResult result;
    KitRenderColor fill;
    KitRenderColor border;

    result = mem_console_authoring_resolve(render_ctx, CORE_THEME_COLOR_ACCENT_PRIMARY, &fill);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_resolve(render_ctx, CORE_THEME_COLOR_TEXT_PRIMARY, &border);
    if (result.code != CORE_OK) return result;
    fill = mem_console_authoring_alpha(fill, 42u);
    border = mem_console_authoring_alpha(border, 230u);

    result = mem_console_authoring_push_rect(frame, rect, fill);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_push_outline(frame, rect, border, 1.5f);
    if (result.code != CORE_OK) return result;
    return mem_console_authoring_draw_label(ui_ctx,
                                            frame,
                                            (KitRenderRect){ rect.x + 8.0f, rect.y + 8.0f, rect.width - 16.0f, 28.0f },
                                            label,
                                            CORE_FONT_TEXT_SIZE_PARAGRAPH);
}

static CoreResult mem_console_authoring_render_panes(KitRenderContext *render_ctx,
                                                     KitUiContext *ui_ctx,
                                                     KitRenderFrame *frame,
                                                     const MemConsoleState *state) {
    CoreResult result;

    result = mem_console_authoring_draw_pane(render_ctx, ui_ctx, frame, state->left_pane, "P1 Search + Scope");
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_pane(render_ctx, ui_ctx, frame, state->pane_right_detail, "P2 Detail + Editing");
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_pane(render_ctx, ui_ctx, frame, state->pane_right_graph, "P3 Graph + Controls");
    if (result.code != CORE_OK) return result;
    if (state->db_modal_open) {
        result = mem_console_authoring_draw_pane(render_ctx,
                                                 ui_ctx,
                                                 frame,
                                                 (KitRenderRect){ 24.0f, 24.0f, (float)state->pane_right_graph.width, 52.0f },
                                                 "P4 Modal Input Root");
        if (result.code != CORE_OK) return result;
    }
    return core_result_ok();
}

static CoreResult mem_console_authoring_render_font_theme(KitRenderContext *render_ctx,
                                                          KitUiContext *ui_ctx,
                                                          KitRenderFrame *frame,
                                                          MemConsoleState *state,
                                                          int frame_width,
                                                          int frame_height) {
    KitWorkspaceAuthoringFontThemeLayout layout;
    MemConsoleWorkspaceAuthoringHost *host;
    CoreResult result;
    KitRenderColor bg;
    KitRenderColor section_fill;
    KitRenderColor border;
    KitRenderRect section;
    uint32_t i;

    if (!render_ctx || !ui_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid authoring font/theme render request" };
    }
    host = &state->workspace_authoring;
    if (!kit_workspace_authoring_ui_font_theme_build_layout(render_ctx, frame_width, frame_height, &layout)) {
        return core_result_ok();
    }

    result = mem_console_authoring_resolve(render_ctx, CORE_THEME_COLOR_SURFACE_0, &bg);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_resolve(render_ctx, CORE_THEME_COLOR_SURFACE_1, &section_fill);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_resolve(render_ctx, CORE_THEME_COLOR_TEXT_MUTED, &border);
    if (result.code != CORE_OK) return result;
    bg = mem_console_authoring_alpha(bg, 250u);
    section_fill = mem_console_authoring_alpha(section_fill, 232u);
    border = mem_console_authoring_alpha(border, 236u);

    result = mem_console_authoring_push_rect(frame, (KitRenderRect){ 0.0f, 0.0f, (float)frame_width, (float)frame_height }, bg);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_push_outline(frame, layout.panel, border, 1.0f);
    if (result.code != CORE_OK) return result;

    (void)snprintf(host->font_preset_line,
                   sizeof(host->font_preset_line),
                   "Font Preset: %s",
                   state->font_name);
    section = layout.font_preset_section;
    result = mem_console_authoring_push_rect(frame, section, section_fill);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_push_outline(frame, section, border, 1.0f);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_section_text(ui_ctx, frame, section, "Font Preset", host->font_preset_line);
    if (result.code != CORE_OK) return result;
    for (i = 0; i < layout.font_preset_button_count; ++i) {
        KitWorkspaceAuthoringFontThemeButtonId id = (KitWorkspaceAuthoringFontThemeButtonId)(
            KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_FONT_PRESET_DAW_DEFAULT + i);
        result = mem_console_authoring_draw_button(render_ctx, ui_ctx, frame, state, id, layout.font_preset_buttons[i], NULL);
        if (result.code != CORE_OK) return result;
    }

    (void)snprintf(host->text_size_line,
                   sizeof(host->text_size_line),
                   "Text Size step:%+d (%d%%)",
                   state->text_zoom_step,
                   kit_render_text_zoom_percent(render_ctx));
    section = layout.text_size_section;
    result = mem_console_authoring_push_rect(frame, section, section_fill);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_push_outline(frame, section, border, 1.0f);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_section_text(ui_ctx, frame, section, "Text Size", host->text_size_line);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_button(render_ctx, ui_ctx, frame, state, KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_DEC, layout.text_size_dec_button, NULL);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_button(render_ctx, ui_ctx, frame, state, KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_INC, layout.text_size_inc_button, NULL);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_button(render_ctx, ui_ctx, frame, state, KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_RESET, layout.text_size_reset_button, NULL);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_button(render_ctx, ui_ctx, frame, state, KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_INC, layout.text_size_value_chip, host->text_size_line);
    if (result.code != CORE_OK) return result;

    (void)snprintf(host->theme_preset_line,
                   sizeof(host->theme_preset_line),
                   "Theme Preset: %s",
                   state->theme_name);
    section = layout.theme_preset_section;
    result = mem_console_authoring_push_rect(frame, section, section_fill);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_push_outline(frame, section, border, 1.0f);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_section_text(ui_ctx, frame, section, "Theme Preset", host->theme_preset_line);
    if (result.code != CORE_OK) return result;
    for (i = 0; i < layout.theme_preset_button_count; ++i) {
        KitWorkspaceAuthoringFontThemeButtonId id = (KitWorkspaceAuthoringFontThemeButtonId)(
            KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_THEME_PRESET_DAW_DEFAULT + i);
        result = mem_console_authoring_draw_button(render_ctx, ui_ctx, frame, state, id, layout.theme_preset_buttons[i], NULL);
        if (result.code != CORE_OK) return result;
    }

    (void)snprintf(host->custom_preset_line,
                   sizeof(host->custom_preset_line),
                   "%s",
                   host->status_text[0] ? host->status_text : "Custom preset slots are stubbed for this host.");
    section = layout.custom_theme_section;
    result = mem_console_authoring_push_rect(frame, section, section_fill);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_push_outline(frame, section, border, 1.0f);
    if (result.code != CORE_OK) return result;
    result = mem_console_authoring_draw_section_text(ui_ctx, frame, section, "Custom Presets", host->custom_preset_line);
    if (result.code != CORE_OK) return result;
    for (i = 0; i < layout.custom_theme_button_count; ++i) {
        KitWorkspaceAuthoringFontThemeButtonId id = (KitWorkspaceAuthoringFontThemeButtonId)(
            KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_CUSTOM_THEME_CREATE_STUB + i);
        result = mem_console_authoring_draw_button(render_ctx, ui_ctx, frame, state, id, layout.custom_theme_buttons[i], NULL);
        if (result.code != CORE_OK) return result;
    }

    return core_result_ok();
}

CoreResult mem_console_workspace_authoring_overlay_render(KitRenderContext *render_ctx,
                                                          KitUiContext *ui_ctx,
                                                          KitRenderFrame *frame,
                                                          MemConsoleState *state,
                                                          int frame_width,
                                                          int frame_height) {
    CoreResult result;

    if (!render_ctx || !ui_ctx || !frame || !state ||
        !mem_console_workspace_authoring_host_active(&state->workspace_authoring)) {
        return core_result_ok();
    }

    if (mem_console_workspace_authoring_host_font_theme_overlay_active(&state->workspace_authoring)) {
        result = mem_console_authoring_render_font_theme(render_ctx,
                                                         ui_ctx,
                                                         frame,
                                                         state,
                                                         frame_width,
                                                         frame_height);
        if (result.code != CORE_OK) return result;
    } else {
        result = mem_console_authoring_render_panes(render_ctx, ui_ctx, frame, state);
        if (result.code != CORE_OK) return result;
    }

    return mem_console_authoring_render_overlay_buttons(render_ctx, frame, &state->workspace_authoring);
}
