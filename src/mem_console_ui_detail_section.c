#include "mem_console_ui_detail_section.h"

#include "mem_console_ui_common.h"
#include "mem_console_ui_detail_panel.h"

#include <stdio.h>
#include <string.h>

CoreResult mem_console_ui_draw_detail_section(KitRenderContext *render_ctx,
                                              KitUiContext *ui_ctx,
                                              KitRenderFrame *frame,
                                              MemConsoleState *state,
                                              const KitUiInputState *input,
                                              const MemConsoleLayoutConfig *layout_cfg,
                                              KitUiStackLayout *out_right_layout) {
    KitUiStackLayout right_layout;
    KitRenderRect row;
    KitRenderRect detail_top_band;
    KitRenderRect detail_title_row;
    KitRenderRect detail_meta_left;
    KitRenderRect detail_meta_right;
    KitRenderRect body_panel;
    KitRenderRect body_content;
    CoreResult result;
    int title_input_active;

    if (!render_ctx || !ui_ctx || !frame || !state || !input || !layout_cfg || !out_right_layout) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid detail section draw request" };
    }

    title_input_active = state->input_target == MEM_CONSOLE_INPUT_TITLE_EDIT;

    result = kit_ui_stack_begin(&right_layout,
                                KIT_UI_AXIS_VERTICAL,
                                (KitRenderRect){
                                    state->pane_right_detail.x + layout_cfg->panel_inner_padding,
                                    state->pane_right_detail.y + layout_cfg->panel_inner_padding,
                                    state->pane_right_detail.width - (layout_cfg->panel_inner_padding * 2.0f),
                                    state->pane_right_detail.height - (layout_cfg->panel_inner_padding * 2.0f)
                                },
                                ui_ctx->style.gap);
    if (result.code != CORE_OK) {
        return result;
    }

    result = kit_ui_stack_next(&right_layout, layout_cfg->right_header_h, 0.0f, &row);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  row,
                                                  "DETAIL",
                                                  CORE_THEME_COLOR_TEXT_PRIMARY,
                                                  CORE_FONT_ROLE_UI_BOLD,
                                                  CORE_FONT_TEXT_SIZE_TITLE);
    if (result.code != CORE_OK) {
        return result;
    }

    result = kit_ui_stack_next(&right_layout, layout_cfg->right_meta_h, 0.0f, &detail_top_band);
    if (result.code != CORE_OK) {
        return result;
    }
    mem_console_ui_detail_build_connection_summary(state,
                                                   state->detail_connection_summary_text,
                                                   sizeof(state->detail_connection_summary_text));
    {
        const float gap = 8.0f;
        const float min_left = 200.0f;
        const float min_right = 170.0f;
        float desired_right = detail_top_band.width * 0.40f;
        float max_right = detail_top_band.width - min_left - gap;
        float right_width;

        if (max_right < min_right) {
            max_right = detail_top_band.width * 0.5f;
        }
        right_width = desired_right;
        if (right_width < min_right) {
            right_width = min_right;
        }
        if (right_width > max_right) {
            right_width = max_right;
        }
        if (right_width < 120.0f) {
            right_width = 120.0f;
        }
        if (right_width > detail_top_band.width - gap - 88.0f) {
            right_width = detail_top_band.width - gap - 88.0f;
        }

        detail_meta_right = detail_top_band;
        detail_meta_right.width = right_width;
        detail_meta_right.x = detail_top_band.x + detail_top_band.width - detail_meta_right.width;

        detail_meta_left = detail_top_band;
        detail_meta_left.width = detail_meta_right.x - detail_top_band.x - gap;
        if (detail_meta_left.width < 100.0f) {
            detail_meta_left.width = 100.0f;
        }
    }

    detail_title_row = (KitRenderRect){
        detail_meta_left.x,
        detail_meta_left.y + 2.0f,
        detail_meta_left.width,
        layout_cfg->right_title_h + 2.0f
    };

    if (state->title_edit_mode) {
        result = mem_console_ui_draw_editable_line(ui_ctx,
                                                   render_ctx,
                                                   frame,
                                                   detail_title_row,
                                                   state->title_edit_text,
                                                   CORE_THEME_COLOR_TEXT_PRIMARY,
                                                   CORE_FONT_ROLE_UI_BOLD,
                                                   CORE_FONT_TEXT_SIZE_PARAGRAPH,
                                                   title_input_active,
                                                   state->title_edit_cursor);
        if (result.code != CORE_OK) {
            return result;
        }
        if (input->mouse_released &&
            kit_ui_point_in_rect(detail_title_row, input->mouse_x, input->mouse_y)) {
            float text_origin_x = detail_title_row.x + ui_ctx->style.padding;
            state->input_target = MEM_CONSOLE_INPUT_TITLE_EDIT;
            state->title_edit_cursor = mem_console_ui_cursor_index_for_click(state->title_edit_text,
                                                                             render_ctx,
                                                                             input->mouse_x,
                                                                             text_origin_x,
                                                                             CORE_FONT_ROLE_UI_BOLD,
                                                                             CORE_FONT_TEXT_SIZE_PARAGRAPH);
        }
    } else {
        format_text_for_width(state->detail_title_draw_line,
                              sizeof(state->detail_title_draw_line),
                              state->selected_title,
                              detail_title_row.width - (ui_ctx->style.padding * 2.0f),
                              CORE_FONT_TEXT_SIZE_PARAGRAPH);
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      detail_title_row,
                                                      state->detail_title_draw_line,
                                                      CORE_THEME_COLOR_TEXT_PRIMARY,
                                                      CORE_FONT_ROLE_UI_BOLD,
                                                      CORE_FONT_TEXT_SIZE_PARAGRAPH);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    if (state->selected_item_id != 0) {
        (void)snprintf(state->detail_meta_line,
                       sizeof(state->detail_meta_line),
                       "MEMORY ID %lld",
                       (long long)state->selected_item_id);
    } else {
        (void)snprintf(state->detail_meta_line,
                       sizeof(state->detail_meta_line),
                       "SELECT A MEMORY TO EDIT");
    }
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  (KitRenderRect){
                                                      detail_meta_left.x,
                                                      detail_title_row.y + detail_title_row.height + 4.0f,
                                                      detail_meta_left.width,
                                                      18.0f
                                                  },
                                                  state->detail_meta_line,
                                                  CORE_THEME_COLOR_TEXT_MUTED,
                                                  CORE_FONT_ROLE_UI_REGULAR,
                                                  CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             detail_meta_right,
                                             8.0f,
                                             CORE_THEME_COLOR_SURFACE_1);
    if (result.code != CORE_OK) {
        return result;
    }
    {
        KitRenderRect summary_content = {
            detail_meta_right.x + 8.0f,
            detail_meta_right.y + 6.0f,
            detail_meta_right.width - 16.0f,
            detail_meta_right.height - 12.0f
        };
        result = mem_console_ui_draw_wrapped_text_block(ui_ctx,
                                                        frame,
                                                        state->detail_connection_summary_lines,
                                                        6,
                                                        summary_content,
                                                        state->detail_connection_summary_text,
                                                        CORE_THEME_COLOR_TEXT_MUTED,
                                                        CORE_FONT_TEXT_SIZE_CAPTION,
                                                        6);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    result = kit_ui_stack_next(&right_layout, layout_cfg->right_section_h, 0.0f, &row);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  row,
                                                  "BODY",
                                                  CORE_THEME_COLOR_TEXT_PRIMARY,
                                                  CORE_FONT_ROLE_UI_MEDIUM,
                                                  CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }

    result = kit_ui_stack_next(&right_layout, layout_cfg->right_body_h, 0.0f, &body_panel);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             body_panel,
                                             10.0f,
                                             CORE_THEME_COLOR_SURFACE_1);
    if (result.code != CORE_OK) {
        return result;
    }

    body_content = (KitRenderRect){
        body_panel.x + 6.0f,
        body_panel.y + 5.0f,
        body_panel.width - 12.0f,
        body_panel.height - 10.0f
    };
    result = mem_console_ui_draw_wrapped_text_block(ui_ctx,
                                                    frame,
                                                    state->wrapped_body_lines,
                                                    6,
                                                    body_content,
                                                    state->body_edit_mode ? state->body_edit_text : state->selected_body,
                                                    CORE_THEME_COLOR_TEXT_MUTED,
                                                    CORE_FONT_TEXT_SIZE_BASIC,
                                                    6);
    if (result.code != CORE_OK) {
        return result;
    }

    if (state->body_edit_mode) {
        KitRenderLineCommand caret_line;
        KitRenderColor caret_color;
        int char_w = mem_console_ui_estimate_char_width_px(CORE_FONT_TEXT_SIZE_BASIC);
        int body_len = (int)strlen(state->body_edit_text);
        int cursor = mem_console_ui_clamp_cursor_for_text(state->body_edit_text, state->body_edit_cursor);
        int line_capacity = (int)((body_content.width - 16.0f) / (float)char_w);
        int line_index;
        int line_start_idx;
        int line_prefix_len;
        int i;
        char line_prefix[256];
        float caret_x;
        float caret_y0;

        if (char_w < 1) {
            char_w = 8;
        }
        if (line_capacity < 1) line_capacity = 1;
        line_index = cursor / line_capacity;
        if (cursor >= body_len && body_len > 0 && body_len % line_capacity == 0) {
            line_index = body_len / line_capacity;
        }

        line_start_idx = line_index * line_capacity;
        if (line_start_idx < 0) {
            line_start_idx = 0;
        }
        if (line_start_idx > body_len) {
            line_start_idx = body_len;
        }
        line_prefix_len = cursor - line_start_idx;
        if (line_prefix_len < 0) {
            line_prefix_len = 0;
        }
        if (line_prefix_len > (int)sizeof(line_prefix) - 1) {
            line_prefix_len = (int)sizeof(line_prefix) - 1;
        }
        for (i = 0; i < line_prefix_len; ++i) {
            line_prefix[i] = state->body_edit_text[line_start_idx + i];
        }
        line_prefix[line_prefix_len] = '\0';

        caret_x = body_content.x + 8.0f +
                  mem_console_ui_measure_text_width_px(render_ctx,
                                                       CORE_FONT_ROLE_UI_REGULAR,
                                                       CORE_FONT_TEXT_SIZE_BASIC,
                                                       line_prefix);
        caret_y0 = body_content.y + 8.0f + ((float)line_index * 24.0f);

        if (caret_x > body_content.x + body_content.width - 8.0f) {
            caret_x = body_content.x + body_content.width - 8.0f;
        }
        if (caret_y0 > body_content.y + body_content.height - 20.0f) {
            caret_y0 = body_content.y + body_content.height - 20.0f;
        }

        if (input->mouse_released && kit_ui_point_in_rect(body_content, input->mouse_x, input->mouse_y)) {
            float text_x = body_content.x + 8.0f;
            float text_y = body_content.y + 8.0f;
            int click_row = (int)((input->mouse_y - text_y + 12.0f) / 24.0f);
            int candidate_cursor;
            int line_end_idx;
            float delta_x;
            float advance = 0.0f;
            char glyph[2];

            if (click_row < 0) click_row = 0;
            line_start_idx = click_row * line_capacity;
            if (line_start_idx < 0) {
                line_start_idx = 0;
            }
            if (line_start_idx > body_len) {
                line_start_idx = body_len;
            }

            line_end_idx = line_start_idx + line_capacity;
            if (line_end_idx > body_len) {
                line_end_idx = body_len;
            }

            delta_x = input->mouse_x - text_x;
            if (delta_x <= 0.0f) {
                candidate_cursor = line_start_idx;
            } else {
                candidate_cursor = line_end_idx;
                glyph[1] = '\0';
                for (i = line_start_idx; i < line_end_idx; ++i) {
                    float glyph_w;
                    glyph[0] = state->body_edit_text[i];
                    glyph_w = mem_console_ui_measure_text_width_px(render_ctx,
                                                                   CORE_FONT_ROLE_UI_REGULAR,
                                                                   CORE_FONT_TEXT_SIZE_BASIC,
                                                                   glyph);
                    if (glyph_w <= 0.0f) {
                        glyph_w = (float)char_w;
                    }
                    if (delta_x < advance + (glyph_w * 0.5f)) {
                        candidate_cursor = i;
                        break;
                    }
                    advance += glyph_w;
                }
            }
            if (candidate_cursor < 0) candidate_cursor = 0;
            if (candidate_cursor > body_len) candidate_cursor = body_len;

            state->input_target = MEM_CONSOLE_INPUT_BODY_EDIT;
            state->body_edit_cursor = candidate_cursor;
            cursor = candidate_cursor;
            line_index = cursor / line_capacity;
            line_start_idx = line_index * line_capacity;
            if (line_start_idx < 0) {
                line_start_idx = 0;
            }
            if (line_start_idx > body_len) {
                line_start_idx = body_len;
            }
            line_prefix_len = cursor - line_start_idx;
            if (line_prefix_len < 0) {
                line_prefix_len = 0;
            }
            if (line_prefix_len > (int)sizeof(line_prefix) - 1) {
                line_prefix_len = (int)sizeof(line_prefix) - 1;
            }
            for (i = 0; i < line_prefix_len; ++i) {
                line_prefix[i] = state->body_edit_text[line_start_idx + i];
            }
            line_prefix[line_prefix_len] = '\0';
            caret_x = body_content.x + 8.0f +
                      mem_console_ui_measure_text_width_px(render_ctx,
                                                           CORE_FONT_ROLE_UI_REGULAR,
                                                           CORE_FONT_TEXT_SIZE_BASIC,
                                                           line_prefix);
            caret_y0 = body_content.y + 8.0f + ((float)line_index * 24.0f);
        }

        result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_ACCENT_PRIMARY, &caret_color);
        if (result.code != CORE_OK) {
            return result;
        }

        result = kit_ui_clip_push(ui_ctx, frame, body_content);
        if (result.code != CORE_OK) {
            return result;
        }

        caret_line.p0.x = caret_x;
        caret_line.p0.y = caret_y0;
        caret_line.p1.x = caret_x;
        caret_line.p1.y = caret_y0 + 18.0f;
        caret_line.thickness = 1.0f;
        caret_line.color = caret_color;
        caret_line.transform = kit_render_identity_transform();
        result = kit_render_push_line(frame, &caret_line);
        if (result.code != CORE_OK) {
            (void)kit_ui_clip_pop(ui_ctx, frame);
            return result;
        }

        result = kit_ui_clip_pop(ui_ctx, frame);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    *out_right_layout = right_layout;
    return core_result_ok();
}
