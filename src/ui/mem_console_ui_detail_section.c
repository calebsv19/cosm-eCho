#include "mem_console_ui_detail_section.h"

#include "mem_console_ui_detail_section_internal.h"
#include "mem_console_state_roles.h"
#include "mem_console_ui_common.h"
#include "mem_console_ui_detail_panel.h"

#include <string.h>

static float detail_estimate_graph_controls_reserved_height(const MemConsoleLayoutConfig *layout_cfg,
                                                            float stack_gap,
                                                            int graph_mode_enabled) {
    float action_block_h;
    float reserved_h;

    if (!layout_cfg) {
        return 0.0f;
    }

    action_block_h = (layout_cfg->action_button_h * 2.0f) +
                     layout_cfg->action_button_gap +
                     (layout_cfg->action_block_pad * 2.0f);

    reserved_h = layout_cfg->right_section_h + stack_gap;
    if (graph_mode_enabled) {
        reserved_h += layout_cfg->graph_filter_h + stack_gap;
        reserved_h += layout_cfg->graph_settings_h + stack_gap;
    } else {
        reserved_h += layout_cfg->graph_collapsed_hint_h + stack_gap;
    }
    reserved_h += action_block_h + stack_gap;

    return reserved_h;
}

static CoreResult detail_draw_scrollable_wrapped_text(KitUiContext *ui_ctx,
                                                      KitRenderFrame *frame,
                                                      char line_storage[][MEM_CONSOLE_DETAIL_TEXT_LINE_CAP],
                                                      int line_storage_count,
                                                      KitRenderRect bounds,
                                                      const char *text,
                                                      CoreThemeColorToken token,
                                                      CoreFontTextSizeTier text_tier,
                                                      const KitUiInputState *input,
                                                      int wheel_y,
                                                      float *io_scroll,
                                                      KitRenderRect *out_text_viewport,
                                                      float *out_line_step) {
    KitRenderRect text_viewport = bounds;
    float line_step;
    int max_chars;
    int line_count;
    float content_height;
    float scroll = 0.0f;
    int has_scrollbar = 0;
    CoreResult result;
    int i;

    if (!ui_ctx || !frame || !line_storage || line_storage_count <= 0 || !text || !io_scroll) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid scrollable wrapped text draw request" };
    }

    if (bounds.width <= 2.0f || bounds.height <= 2.0f) {
        if (out_text_viewport) {
            *out_text_viewport = bounds;
        }
        if (out_line_step) {
            *out_line_step = mem_console_detail_wrapped_text_line_step(text_tier);
        }
        return core_result_ok();
    }

    line_step = mem_console_detail_wrapped_text_line_step(text_tier);
    if (line_step < 14.0f) {
        line_step = 14.0f;
    }

    max_chars = (int)((bounds.width - 16.0f) / 8.0f);
    line_count = mem_console_detail_wrap_text_lines(text, line_storage, line_storage_count, max_chars);

    content_height = 12.0f + ((float)line_count * line_step);
    if (content_height < bounds.height) {
        content_height = bounds.height;
    }

    scroll = *io_scroll;
    if (scroll < 0.0f) {
        scroll = 0.0f;
    }
    if (scroll > content_height - bounds.height) {
        scroll = content_height - bounds.height;
    }
    if (scroll < 0.0f) {
        scroll = 0.0f;
    }

    if (wheel_y != 0 && input && kit_ui_point_in_rect(bounds, input->mouse_x, input->mouse_y)) {
        KitUiScrollResult scroll_result = kit_ui_eval_scroll(bounds,
                                                             scroll,
                                                             content_height,
                                                             (float)wheel_y);
        if (scroll_result.changed) {
            scroll = scroll_result.offset_y;
        }
    }

    has_scrollbar = (content_height - bounds.height) > 0.5f;
    if (has_scrollbar) {
        text_viewport.width -= 10.0f;
        if (text_viewport.width < 20.0f) {
            text_viewport.width = 20.0f;
        }

        max_chars = (int)((text_viewport.width - 16.0f) / 8.0f);
        line_count = mem_console_detail_wrap_text_lines(text, line_storage, line_storage_count, max_chars);
        content_height = 12.0f + ((float)line_count * line_step);
        if (content_height < bounds.height) {
            content_height = bounds.height;
        }
        if (scroll > content_height - bounds.height) {
            scroll = content_height - bounds.height;
        }
        if (scroll < 0.0f) {
            scroll = 0.0f;
        }
    }

    *io_scroll = scroll;
    if (out_text_viewport) {
        *out_text_viewport = text_viewport;
    }
    if (out_line_step) {
        *out_line_step = line_step;
    }

    result = kit_ui_clip_push(ui_ctx, frame, text_viewport);
    if (result.code != CORE_OK) {
        return result;
    }

    for (i = 0; i < line_count; ++i) {
        float y = bounds.y + 6.0f + ((float)i * line_step) - scroll;
        KitRenderRect line_rect;

        if (y + line_step < bounds.y || y > bounds.y + bounds.height) {
            continue;
        }

        line_rect = (KitRenderRect){
            text_viewport.x + 8.0f,
            y,
            text_viewport.width - 16.0f,
            line_step
        };
        if (line_storage[i][0] == '\0') {
            continue;
        }
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      line_rect,
                                                      line_storage[i],
                                                      token,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      text_tier);
        if (result.code != CORE_OK) {
            (void)kit_ui_clip_pop(ui_ctx, frame);
            return result;
        }
    }

    result = kit_ui_clip_pop(ui_ctx, frame);
    if (result.code != CORE_OK) {
        return result;
    }

    if (has_scrollbar) {
        result = kit_ui_draw_scrollbar(ui_ctx,
                                       frame,
                                       bounds,
                                       scroll,
                                       content_height);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    return core_result_ok();
}

CoreResult mem_console_ui_draw_detail_section(KitRenderContext *render_ctx,
                                              KitUiContext *ui_ctx,
                                              KitRenderFrame *frame,
                                              MemConsoleState *state,
                                              const KitUiInputState *input,
                                              int wheel_y,
                                              const MemConsoleLayoutConfig *layout_cfg,
                                              KitUiStackLayout *out_right_layout,
                                              MemConsoleAction *io_action) {
    KitUiStackLayout meta_layout;
    KitUiStackLayout body_layout;
    MemConsoleDetailRenderState detail_view;
    MemConsoleDetailRenderStorage detail_storage;
    KitRenderRect row;
    KitRenderRect detail_meta_line_rect;
    KitRenderRect detail_title_row;
    KitRenderRect body_panel;
    KitRenderRect body_content;
    KitRenderRect body_text_viewport;
    float body_h;
    float reserved_controls_h;
    float body_line_step = 24.0f;
    float title_line_step = 24.0f;
    float title_text_width = 0.0f;
    CoreResult result;
    int title_input_active;
    int title_line_count = 0;
    int title_line_limit = 0;
    int title_max_chars = 0;
    int i;

    if (!render_ctx || !ui_ctx || !frame || !state || !input || !layout_cfg || !out_right_layout) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid detail section draw request" };
    }

    mem_console_ui_detail_refresh_reference_path_cache(state);
    if (!mem_console_detail_render_state_from_state(state, &detail_view) ||
        !mem_console_detail_render_storage_from_state(state, &detail_storage)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid detail render state" };
    }

    title_input_active = state->input_target == MEM_CONSOLE_INPUT_TITLE_EDIT;

    result = kit_ui_stack_begin(&meta_layout,
                                KIT_UI_AXIS_VERTICAL,
                                (KitRenderRect){
                                    state->pane_right_detail_meta.x + layout_cfg->panel_inner_padding,
                                    state->pane_right_detail_meta.y + layout_cfg->panel_inner_padding,
                                    state->pane_right_detail_meta.width - (layout_cfg->panel_inner_padding * 2.0f),
                                    state->pane_right_detail_meta.height - (layout_cfg->panel_inner_padding * 2.0f)
                                },
                                ui_ctx->style.gap);
    if (result.code != CORE_OK) {
        return result;
    }

    detail_title_row = (KitRenderRect){
        meta_layout.bounds.x,
        meta_layout.bounds.y + 1.0f,
        meta_layout.bounds.width,
        meta_layout.bounds.height - 23.0f
    };
    if (detail_title_row.height < 20.0f) {
        detail_title_row.height = 20.0f;
    }
    detail_meta_line_rect = (KitRenderRect){
        meta_layout.bounds.x,
        meta_layout.bounds.y + meta_layout.bounds.height - 18.0f,
        meta_layout.bounds.width,
        18.0f
    };

    if (state->title_edit_mode) {
        mem_console_detail_clear_title_lines(&detail_storage);
        result = mem_console_ui_draw_editable_line(ui_ctx,
                                                   render_ctx,
                                                   frame,
                                                   detail_title_row,
                                                   state->title_edit_text,
                                                   CORE_THEME_COLOR_TEXT_PRIMARY,
                                                   CORE_FONT_ROLE_UI_BOLD,
                                                   CORE_FONT_TEXT_SIZE_TITLE,
                                                   title_input_active,
                                                   state->title_edit_cursor);
        if (result.code != CORE_OK) {
            return result;
        }
        if (input->mouse_released &&
            kit_ui_point_in_rect(detail_title_row, input->mouse_x, input->mouse_y)) {
            float text_origin_x = detail_title_row.x + ui_ctx->style.padding;
            mem_console_input_target_set(state, MEM_CONSOLE_INPUT_TITLE_EDIT);
            state->title_edit_cursor = mem_console_ui_cursor_index_for_click(state->title_edit_text,
                                                                             render_ctx,
                                                                             input->mouse_x,
                                                                             text_origin_x,
                                                                             CORE_FONT_ROLE_UI_BOLD,
                                                                             CORE_FONT_TEXT_SIZE_TITLE);
        }
    } else {
        title_line_step = mem_console_detail_wrapped_text_line_step(CORE_FONT_TEXT_SIZE_TITLE);
        if (title_line_step < 18.0f) {
            title_line_step = 18.0f;
        }
        title_text_width = detail_title_row.width - (ui_ctx->style.padding * 2.0f);
        if (title_text_width < 40.0f) {
            title_text_width = 40.0f;
        }

        title_line_limit = (int)((detail_title_row.height - 2.0f) / title_line_step);
        if (title_line_limit < 1) {
            title_line_limit = 1;
        }
        if (title_line_limit > detail_storage.title_line_capacity) {
            title_line_limit = detail_storage.title_line_capacity;
        }

        title_max_chars = (int)(title_text_width /
                                (float)mem_console_ui_estimate_char_width_px(CORE_FONT_TEXT_SIZE_TITLE));
        title_line_count = mem_console_detail_derive_title_lines(&detail_view,
                                                                 &detail_storage,
                                                                 title_line_limit,
                                                                 title_max_chars);

        for (i = 0; i < title_line_count; ++i) {
            KitRenderRect title_line_rect = {
                detail_title_row.x,
                detail_title_row.y + ((float)i * title_line_step),
                detail_title_row.width,
                title_line_step
            };
            result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                          frame,
                                                          title_line_rect,
                                                          detail_storage.title_lines[i],
                                                          CORE_THEME_COLOR_TEXT_PRIMARY,
                                                          CORE_FONT_ROLE_UI_BOLD,
                                                          CORE_FONT_TEXT_SIZE_TITLE);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    mem_console_detail_derive_meta_line(&detail_view, &detail_storage);
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  detail_meta_line_rect,
                                                  detail_storage.meta_line,
                                                  CORE_THEME_COLOR_TEXT_MUTED,
                                                  CORE_FONT_ROLE_UI_REGULAR,
                                                  CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }

    result = mem_console_ui_detail_draw_relationships(render_ctx,
                                                      ui_ctx,
                                                      frame,
                                                      state,
                                                      input,
                                                      wheel_y,
                                                      layout_cfg,
                                                      io_action);
    if (result.code != CORE_OK) {
        return result;
    }

    result = kit_ui_stack_begin(&body_layout,
                                KIT_UI_AXIS_VERTICAL,
                                (KitRenderRect){
                                    state->pane_right_detail_body.x + layout_cfg->panel_inner_padding,
                                    state->pane_right_detail_body.y + layout_cfg->panel_inner_padding,
                                    state->pane_right_detail_body.width - (layout_cfg->panel_inner_padding * 2.0f),
                                    state->pane_right_detail_body.height - (layout_cfg->panel_inner_padding * 2.0f)
                                },
                                ui_ctx->style.gap);
    if (result.code != CORE_OK) {
        return result;
    }

    result = kit_ui_stack_next(&body_layout, layout_cfg->right_section_h, 0.0f, &row);
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

    reserved_controls_h = detail_estimate_graph_controls_reserved_height(layout_cfg,
                                                                          body_layout.gap,
                                                                          state->graph_mode_enabled);
    body_h = body_layout.bounds.height - body_layout.cursor - reserved_controls_h;
    if (body_h < 26.0f) {
        body_h = 26.0f;
    }

    result = kit_ui_stack_next(&body_layout, body_h, 0.0f, &body_panel);
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

    result = detail_draw_scrollable_wrapped_text(ui_ctx,
                                                 frame,
                                                 state->wrapped_body_lines,
                                                 MEM_CONSOLE_DETAIL_BODY_WRAP_LINE_LIMIT,
                                                 body_content,
                                                 state->body_edit_mode ? state->body_edit_text : state->selected_body,
                                                 CORE_THEME_COLOR_TEXT_MUTED,
                                                 CORE_FONT_TEXT_SIZE_BASIC,
                                                 input,
                                                 wheel_y,
                                                 &state->detail_body_scroll,
                                                 &body_text_viewport,
                                                 &body_line_step);
    if (result.code != CORE_OK) {
        return result;
    }

    if (state->body_edit_mode) {
        KitRenderLineCommand caret_line;
        KitRenderColor caret_color;
        int char_w = mem_console_ui_estimate_char_width_px(CORE_FONT_TEXT_SIZE_BASIC);
        int body_len = (int)strlen(state->body_edit_text);
        int cursor = mem_console_ui_clamp_cursor_for_text(state->body_edit_text, state->body_edit_cursor);
        int line_capacity;
        int line_index;
        int line_start_idx;
        int line_prefix_len;
        int i;
        char line_prefix[256];
        float caret_x;
        float caret_y0;

        (void)body_line_step;

        if (char_w < 1) {
            char_w = 8;
        }
        line_capacity = (int)((body_text_viewport.width - 16.0f) / (float)char_w);
        if (line_capacity < 1) {
            line_capacity = 1;
        }
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

        caret_x = body_text_viewport.x + 8.0f +
                  mem_console_ui_measure_text_width_px(render_ctx,
                                                       CORE_FONT_ROLE_UI_REGULAR,
                                                       CORE_FONT_TEXT_SIZE_BASIC,
                                                       line_prefix);
        caret_y0 = body_text_viewport.y + 8.0f + ((float)line_index * 24.0f) - state->detail_body_scroll;

        if (caret_x > body_text_viewport.x + body_text_viewport.width - 8.0f) {
            caret_x = body_text_viewport.x + body_text_viewport.width - 8.0f;
        }

        if (input->mouse_released &&
            kit_ui_point_in_rect(body_text_viewport, input->mouse_x, input->mouse_y)) {
            float text_x = body_text_viewport.x + 8.0f;
            float text_y = body_text_viewport.y + 8.0f;
            int click_row = (int)((input->mouse_y - text_y + state->detail_body_scroll + 12.0f) / 24.0f);
            int candidate_cursor;
            int line_end_idx;
            float delta_x;
            float advance = 0.0f;
            char glyph[2];

            if (click_row < 0) {
                click_row = 0;
            }
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
            if (candidate_cursor < 0) {
                candidate_cursor = 0;
            }
            if (candidate_cursor > body_len) {
                candidate_cursor = body_len;
            }

            mem_console_input_target_set(state, MEM_CONSOLE_INPUT_BODY_EDIT);
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
            caret_x = body_text_viewport.x + 8.0f +
                      mem_console_ui_measure_text_width_px(render_ctx,
                                                           CORE_FONT_ROLE_UI_REGULAR,
                                                           CORE_FONT_TEXT_SIZE_BASIC,
                                                           line_prefix);
            caret_y0 = body_text_viewport.y + 8.0f + ((float)line_index * 24.0f) - state->detail_body_scroll;
        }

        result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_ACCENT_PRIMARY, &caret_color);
        if (result.code != CORE_OK) {
            return result;
        }

        result = kit_ui_clip_push(ui_ctx, frame, body_text_viewport);
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

    *out_right_layout = body_layout;
    return core_result_ok();
}
