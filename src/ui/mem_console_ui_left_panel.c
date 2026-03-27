#include "mem_console_ui_left_panel.h"

#include "mem_console_ui_common.h"

static KitRenderColor left_mix_color(KitRenderColor a, KitRenderColor b, float t) {
    float clamped_t = t;
    float inv_t;
    KitRenderColor out;

    if (clamped_t < 0.0f) clamped_t = 0.0f;
    if (clamped_t > 1.0f) clamped_t = 1.0f;
    inv_t = 1.0f - clamped_t;

    out.r = (uint8_t)((a.r * inv_t) + (b.r * clamped_t));
    out.g = (uint8_t)((a.g * inv_t) + (b.g * clamped_t));
    out.b = (uint8_t)((a.b * inv_t) + (b.b * clamped_t));
    out.a = 255u;
    return out;
}

static CoreResult draw_project_chip(KitUiContext *ui_ctx,
                                    KitRenderFrame *frame,
                                    KitRenderRect rect,
                                    const char *label,
                                    const char *project_key,
                                    KitUiWidgetState state) {
    CoreResult result;
    KitRenderColor base_fill;
    KitRenderColor project_color;
    KitRenderColor fill_color;
    KitRenderColor outline_color;
    CoreThemeColorToken text_token;
    float outline_thickness;
    float tint_t;

    if (!ui_ctx || !frame || !label) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid project chip request" };
    }

    if (!project_key || project_key[0] == '\0') {
        return mem_console_ui_draw_button_custom(ui_ctx,
                                                 frame,
                                                 rect,
                                                 label,
                                                 state,
                                                 CORE_FONT_ROLE_UI_REGULAR,
                                                 CORE_FONT_TEXT_SIZE_CAPTION);
    }

    result = mem_console_ui_resolve_theme_color(ui_ctx->render_ctx, CORE_THEME_COLOR_SURFACE_1, &base_fill);
    if (result.code != CORE_OK) {
        return result;
    }

    project_color = mem_console_ui_project_color_for_key(project_key);
    tint_t = state == KIT_UI_STATE_ACTIVE ? 0.74f : (state == KIT_UI_STATE_HOVERED ? 0.34f : 0.14f);
    fill_color = left_mix_color(base_fill, project_color, tint_t);
    fill_color.a = state == KIT_UI_STATE_ACTIVE ? 244u : (state == KIT_UI_STATE_HOVERED ? 228u : 196u);
    outline_color = left_mix_color(project_color, (KitRenderColor){ 232, 232, 236, 255 }, 0.24f);
    outline_color.a = state == KIT_UI_STATE_ACTIVE ? 246u : (state == KIT_UI_STATE_HOVERED ? 176u : 96u);
    outline_thickness = state == KIT_UI_STATE_ACTIVE ? 1.8f : 0.8f;
    text_token = state == KIT_UI_STATE_ACTIVE ? CORE_THEME_COLOR_TEXT_PRIMARY : CORE_THEME_COLOR_TEXT_MUTED;

    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      rect,
                                      6.0f,
                                      fill_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }

    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      (KitRenderRect){ rect.x, rect.y, rect.width, outline_thickness },
                                      0.0f,
                                      outline_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }
    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      (KitRenderRect){ rect.x, rect.y + rect.height - outline_thickness, rect.width, outline_thickness },
                                      0.0f,
                                      outline_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }
    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      (KitRenderRect){ rect.x, rect.y, outline_thickness, rect.height },
                                      0.0f,
                                      outline_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }
    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      (KitRenderRect){ rect.x + rect.width - outline_thickness, rect.y, outline_thickness, rect.height },
                                      0.0f,
                                      outline_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }

    return kit_ui_draw_label_custom(ui_ctx,
                                    frame,
                                    rect,
                                    label,
                                    text_token,
                                    CORE_FONT_ROLE_UI_REGULAR,
                                    CORE_FONT_TEXT_SIZE_CAPTION);
}

CoreResult mem_console_ui_left_draw_project_filter_chips(KitUiContext *ui_ctx,
                                                         const KitRenderContext *render_ctx,
                                                         KitRenderFrame *frame,
                                                         MemConsoleState *state,
                                                         const KitUiInputState *input,
                                                         KitRenderRect bounds,
                                                         int wheel_y,
                                                         int input_enabled,
                                                         int *out_changed) {
    KitRenderRect layout_rects[MEM_CONSOLE_SCOPE_FILTER_LIMIT + 1];
    const char *layout_labels[MEM_CONSOLE_SCOPE_FILTER_LIMIT + 1];
    const char *layout_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT + 1];
    float x = 0.0f;
    float y = 0.0f;
    float right = bounds.width;
    float chip_height = 20.0f;
    float line_gap = 4.0f;
    float chip_gap = 4.0f;
    float content_height = 0.0f;
    float max_scroll = 0.0f;
    int i;
    int chip_count = 0;
    int changed = 0;
    int ordered_option_indices[MEM_CONSOLE_SCOPE_FILTER_LIMIT];
    int ordered_option_count = 0;

    if (!ui_ctx || !render_ctx || !frame || !state || !input || !out_changed) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid project filter draw request" };
    }

    {
        float all_width = mem_console_ui_measure_text_width_px(render_ctx,
                                                               CORE_FONT_ROLE_UI_REGULAR,
                                                               CORE_FONT_TEXT_SIZE_CAPTION,
                                                               "ALL PROJECTS");
        all_width += ui_ctx->style.padding * 2.0f + 8.0f;
        if (all_width < 76.0f) {
            all_width = 76.0f;
        }
        if (all_width > bounds.width) {
            all_width = bounds.width;
        }

        layout_rects[chip_count] = (KitRenderRect){x, y, all_width, chip_height};
        layout_labels[chip_count] = "ALL PROJECTS";
        layout_keys[chip_count] = "";
        chip_count += 1;
        x += all_width + chip_gap;
    }

    for (i = 0; i < state->project_filter_option_count; ++i) {
        if (state->project_filter_keys[i][0] == '\0') {
            continue;
        }
        if (mem_console_project_filter_is_selected(state, state->project_filter_keys[i])) {
            ordered_option_indices[ordered_option_count] = i;
            ordered_option_count += 1;
            if (ordered_option_count >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
                break;
            }
        }
    }
    for (i = 0; i < state->project_filter_option_count; ++i) {
        int j;
        int already_added = 0;
        if (state->project_filter_keys[i][0] == '\0') {
            continue;
        }
        for (j = 0; j < ordered_option_count; ++j) {
            if (ordered_option_indices[j] == i) {
                already_added = 1;
                break;
            }
        }
        if (!already_added) {
            ordered_option_indices[ordered_option_count] = i;
            ordered_option_count += 1;
            if (ordered_option_count >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
                break;
            }
        }
    }

    for (i = 0; i < ordered_option_count; ++i) {
        float label_width;
        float chip_width;
        int option_index = ordered_option_indices[i];

        label_width = mem_console_ui_measure_text_width_px(render_ctx,
                                                           CORE_FONT_ROLE_UI_REGULAR,
                                                           CORE_FONT_TEXT_SIZE_CAPTION,
                                                           state->project_filter_labels[option_index]);
        chip_width = label_width + (ui_ctx->style.padding * 2.0f) + 8.0f;
        if (chip_width < 70.0f) {
            chip_width = 70.0f;
        }
        if (chip_width > bounds.width) {
            chip_width = bounds.width;
        }

        if (x + chip_width > right) {
            x = 0.0f;
            y += chip_height + line_gap;
        }
        if (chip_count >= (int)(MEM_CONSOLE_SCOPE_FILTER_LIMIT + 1)) {
            break;
        }
        layout_rects[chip_count] = (KitRenderRect){x, y, chip_width, chip_height};
        layout_labels[chip_count] = state->project_filter_labels[option_index];
        layout_keys[chip_count] = state->project_filter_keys[option_index];
        chip_count += 1;
        x += chip_width + chip_gap;
    }

    for (i = 0; i < chip_count; ++i) {
        float bottom = layout_rects[i].y + layout_rects[i].height;
        if (bottom > content_height) {
            content_height = bottom;
        }
    }
    if (content_height < bounds.height) {
        content_height = bounds.height;
    }

    if (content_height > bounds.height) {
        max_scroll = content_height - bounds.height;
    }
    if (state->project_filter_scroll < 0.0f) {
        state->project_filter_scroll = 0.0f;
    } else if (state->project_filter_scroll > max_scroll) {
        state->project_filter_scroll = max_scroll;
    }

    if (wheel_y != 0 && kit_ui_point_in_rect(bounds, input->mouse_x, input->mouse_y)) {
        KitUiScrollResult scroll_result = kit_ui_eval_scroll(bounds,
                                                             state->project_filter_scroll,
                                                             content_height,
                                                             (float)wheel_y);
        if (scroll_result.changed) {
            state->project_filter_scroll = scroll_result.offset_y;
        }
    }

    {
        CoreResult clip_result = kit_ui_clip_push(ui_ctx, frame, bounds);
        if (clip_result.code != CORE_OK) {
            return clip_result;
        }
    }

    for (i = 0; i < chip_count; ++i) {
        const char *project_key = layout_keys[i];
        KitRenderRect chip_rect = {
            bounds.x + layout_rects[i].x,
            bounds.y + layout_rects[i].y - state->project_filter_scroll,
            layout_rects[i].width,
            layout_rects[i].height
        };
        KitUiButtonResult button_result;

        if (chip_rect.y + chip_rect.height < bounds.y || chip_rect.y > (bounds.y + bounds.height)) {
            continue;
        }

        button_result = kit_ui_eval_button(chip_rect, input, input_enabled);
        if (!project_key || project_key[0] == '\0') {
            if (state->selected_project_count == 0) {
                button_result.state = KIT_UI_STATE_ACTIVE;
            }
            if (button_result.clicked) {
                mem_console_project_filter_clear(state);
                changed = 1;
            }
        } else {
            if (mem_console_project_filter_is_selected(state, project_key)) {
                button_result.state = KIT_UI_STATE_ACTIVE;
            }
            if (button_result.clicked && mem_console_project_filter_toggle(state, project_key)) {
                changed = 1;
            }
        }

        {
            CoreResult draw_result = draw_project_chip(ui_ctx,
                                                       frame,
                                                       chip_rect,
                                                       layout_labels[i],
                                                       project_key,
                                                       button_result.state);
            if (draw_result.code != CORE_OK) {
                (void)kit_ui_clip_pop(ui_ctx, frame);
                return draw_result;
            }
        }
    }

    {
        CoreResult clip_result = kit_ui_clip_pop(ui_ctx, frame);
        if (clip_result.code != CORE_OK) {
            return clip_result;
        }
    }

    {
        CoreResult draw_result = kit_ui_draw_scrollbar(ui_ctx,
                                                       frame,
                                                       bounds,
                                                       state->project_filter_scroll,
                                                       content_height);
        if (draw_result.code != CORE_OK) {
            return draw_result;
        }
    }

    *out_changed = changed;
    return core_result_ok();
}
