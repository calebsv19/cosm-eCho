#include "mem_console_ui_left_section.h"

#include "mem_console_ui_common.h"
#include "mem_console_ui_left_panel.h"

#include <limits.h>
#include <stdio.h>

CoreResult mem_console_ui_draw_left_section(KitRenderContext *render_ctx,
                                            KitUiContext *ui_ctx,
                                            KitRenderFrame *frame,
                                            MemConsoleState *state,
                                            const KitUiInputState *input,
                                            const MemConsoleLayoutConfig *layout_cfg,
                                            int wheel_y,
                                            int has_any_edit_mode,
                                            MemConsoleAction *io_action) {
    KitUiStackLayout left_layout;
    KitRenderRect row;
    KitRenderRect search_box;
    KitRenderRect project_filter_box;
    KitRenderRect list_header;
    KitRenderRect list_viewport;
    KitRenderRect list_content_viewport;
    KitRenderRect item_rect;
    KitUiButtonResult button_result;
    int search_input_active;
    int project_filters_changed;
    int loaded_end_index;
    int matching_rows;
    int first_visible_index;
    int desired_query_offset;
    int i;
    const char *search_display_text;
    float row_pitch;
    float content_height;
    float max_scroll;
    if (!render_ctx || !ui_ctx || !frame || !state || !input || !layout_cfg || !io_action) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid left section draw request" };
    }

    search_input_active = state->input_target == MEM_CONSOLE_INPUT_SEARCH;
    project_filters_changed = 0;

    {
        CoreResult result = kit_ui_stack_begin(&left_layout,
                                               KIT_UI_AXIS_VERTICAL,
                                               (KitRenderRect){
                                                   state->left_pane.x + layout_cfg->panel_inner_padding,
                                                   state->left_pane.y + layout_cfg->panel_inner_padding,
                                                   state->left_pane.width - (layout_cfg->panel_inner_padding * 2.0f),
                                                   state->left_pane.height - (layout_cfg->panel_inner_padding * 2.0f)
                                               },
                                               ui_ctx->style.gap);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_header_h, 0.0f, &row);
        if (result.code != CORE_OK) return result;
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      row,
                                                      "MEMORY CONSOLE",
                                                      CORE_THEME_COLOR_TEXT_PRIMARY,
                                                      CORE_FONT_ROLE_UI_BOLD,
                                                      CORE_FONT_TEXT_SIZE_TITLE);
        if (result.code != CORE_OK) return result;
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_info_row_h, 0.0f, &row);
        if (result.code != CORE_OK) return result;
        (void)snprintf(state->db_summary_line, sizeof(state->db_summary_line), "DB: %s", state->db_path);
        format_text_for_width(state->db_summary_draw_line,
                              sizeof(state->db_summary_draw_line),
                              state->db_summary_line,
                              row.width - (ui_ctx->style.padding * 2.0f),
                              CORE_FONT_TEXT_SIZE_CAPTION);
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      row,
                                                      state->db_summary_draw_line,
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) return result;
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_info_row_h, 0.0f, &row);
        if (result.code != CORE_OK) return result;
        (void)snprintf(state->schema_summary_line,
                       sizeof(state->schema_summary_line),
                       "v%s | Active %lld | %s",
                       state->schema_version[0] ? state->schema_version : "?",
                       (long long)state->active_count,
                       state->theme_name[0] ? state->theme_name : "theme");
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      row,
                                                      state->schema_summary_line,
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) return result;
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_reload_h, 0.0f, &row);
        if (result.code != CORE_OK) return result;
        button_result = kit_ui_eval_button(row, input, 1);
        if (button_result.clicked) {
            *io_action = MEM_CONSOLE_ACTION_REFRESH;
        }
        result = mem_console_ui_draw_button_custom(ui_ctx,
                                                   frame,
                                                   row,
                                                   "RELOAD",
                                                   button_result.state,
                                                   CORE_FONT_ROLE_UI_MEDIUM,
                                                   CORE_FONT_TEXT_SIZE_PARAGRAPH);
        if (result.code != CORE_OK) return result;
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_section_h, 0.0f, &row);
        if (result.code != CORE_OK) return result;
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      row,
                                                      "SEARCH FILTER",
                                                      CORE_THEME_COLOR_TEXT_PRIMARY,
                                                      CORE_FONT_ROLE_UI_MEDIUM,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) return result;
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_search_h, 0.0f, &search_box);
        if (result.code != CORE_OK) return result;
        result = mem_console_ui_push_themed_rect(render_ctx,
                                                 frame,
                                                 search_box,
                                                 8.0f,
                                                 CORE_THEME_COLOR_SURFACE_2);
        if (result.code != CORE_OK) return result;
        if (state->search_text[0]) {
            search_display_text = state->search_text;
        } else if (search_input_active) {
            search_display_text = "";
        } else {
            search_display_text = "ALL ACTIVE MEMORIES";
        }
        result = mem_console_ui_draw_editable_line(ui_ctx,
                                                   render_ctx,
                                                   frame,
                                                   (KitRenderRect){
                                                       search_box.x + 6.0f,
                                                       search_box.y + 3.0f,
                                                       search_box.width - 12.0f,
                                                       search_box.height - 6.0f
                                                   },
                                                   search_display_text,
                                                   state->search_text[0] ? CORE_THEME_COLOR_TEXT_PRIMARY : CORE_THEME_COLOR_TEXT_MUTED,
                                                   CORE_FONT_ROLE_UI_REGULAR,
                                                   CORE_FONT_TEXT_SIZE_PARAGRAPH,
                                                   search_input_active,
                                                   state->search_cursor);
        if (result.code != CORE_OK) return result;
        if (input->mouse_released &&
            !has_any_edit_mode &&
            kit_ui_point_in_rect(search_box, input->mouse_x, input->mouse_y)) {
            float text_origin_x = search_box.x + 6.0f + ui_ctx->style.padding;
            state->input_target = MEM_CONSOLE_INPUT_SEARCH;
            state->search_cursor = mem_console_ui_cursor_index_for_click(state->search_text,
                                                                          render_ctx,
                                                                          input->mouse_x,
                                                                          text_origin_x,
                                                                          CORE_FONT_ROLE_UI_REGULAR,
                                                                          CORE_FONT_TEXT_SIZE_PARAGRAPH);
        }
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_section_h, 0.0f, &row);
        if (result.code != CORE_OK) return result;
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      row,
                                                      state->project_filter_summary_line,
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) return result;
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_project_filters_h, 0.0f, &project_filter_box);
        if (result.code != CORE_OK) return result;
        result = mem_console_ui_push_themed_rect(render_ctx,
                                                 frame,
                                                 project_filter_box,
                                                 8.0f,
                                                 CORE_THEME_COLOR_SURFACE_0);
        if (result.code != CORE_OK) return result;
        result = mem_console_ui_left_draw_project_filter_chips(ui_ctx,
                                                               render_ctx,
                                                               frame,
                                                               state,
                                                               input,
                                                               (KitRenderRect){
                                                                   project_filter_box.x + 4.0f,
                                                                   project_filter_box.y + 3.0f,
                                                                   project_filter_box.width - 8.0f,
                                                                   project_filter_box.height - 6.0f
                                                               },
                                                               wheel_y,
                                                               !has_any_edit_mode,
                                                               &project_filters_changed);
        if (result.code != CORE_OK) return result;
        if (project_filters_changed) {
            state->list_scroll = 0.0f;
            state->list_query_offset = 0;
            state->search_refresh_pending = 0;
            if (*io_action == MEM_CONSOLE_ACTION_NONE) {
                *io_action = MEM_CONSOLE_ACTION_REFRESH;
            }
        }
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_section_h, 0.0f, &row);
        if (result.code != CORE_OK) return result;
        (void)snprintf(state->visible_summary_line,
                       sizeof(state->visible_summary_line),
                       "%d loaded | %lld matching",
                       state->visible_count,
                       (long long)state->matching_count);
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      row,
                                                      state->visible_summary_line,
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) return result;
    }

    {
        CoreResult result = kit_ui_stack_next(&left_layout, layout_cfg->left_results_header_h, 0.0f, &list_header);
        if (result.code != CORE_OK) return result;
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      list_header,
                                                      "RESULTS",
                                                      CORE_THEME_COLOR_TEXT_PRIMARY,
                                                      CORE_FONT_ROLE_UI_MEDIUM,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) return result;
    }

    list_viewport = (KitRenderRect){
        left_layout.bounds.x,
        left_layout.bounds.y + left_layout.cursor,
        left_layout.bounds.width,
        left_layout.bounds.y + left_layout.bounds.height - (left_layout.bounds.y + left_layout.cursor) - (layout_cfg->left_status_h + 6.0f)
    };
    if (list_viewport.height < 0.0f) {
        list_viewport.height = 0.0f;
    }
    list_content_viewport = list_viewport;
    list_content_viewport.width -= 10.0f;
    if (list_content_viewport.width < 0.0f) {
        list_content_viewport.width = 0.0f;
    }

    row_pitch = layout_cfg->list_row_pitch;
    if (row_pitch <= 0.0f) {
        row_pitch = (float)MEM_CONSOLE_LIST_ROW_PITCH_PX;
    }
    if (state->matching_count > (int64_t)INT_MAX) {
        matching_rows = INT_MAX;
    } else if (state->matching_count < 0) {
        matching_rows = 0;
    } else {
        matching_rows = (int)state->matching_count;
    }
    content_height = kit_ui_scroll_content_height_top_anchor(matching_rows,
                                                              row_pitch,
                                                              list_viewport.height);
    max_scroll = 0.0f;
    if (content_height > list_viewport.height) {
        max_scroll = content_height - list_viewport.height;
    }
    if (state->list_scroll < 0.0f) {
        state->list_scroll = 0.0f;
    } else if (state->list_scroll > max_scroll) {
        state->list_scroll = max_scroll;
    }

    if (wheel_y != 0 && kit_ui_point_in_rect(list_viewport, input->mouse_x, input->mouse_y)) {
        KitUiScrollResult scroll_result = kit_ui_eval_scroll(list_viewport,
                                                             state->list_scroll,
                                                             content_height,
                                                             (float)wheel_y);
        if (scroll_result.changed) {
            state->list_scroll = scroll_result.offset_y;
        }
    }

    if (matching_rows > 0) {
        first_visible_index = (int)(state->list_scroll / row_pitch);
        if (first_visible_index < 0) {
            first_visible_index = 0;
        }
        if (first_visible_index >= matching_rows) {
            first_visible_index = matching_rows - 1;
        }

        desired_query_offset = first_visible_index - 2;
        if (desired_query_offset < 0) {
            desired_query_offset = 0;
        }

        loaded_end_index = state->visible_start_index + state->visible_count;
        if (state->visible_count == 0 ||
            desired_query_offset < state->visible_start_index ||
            desired_query_offset >= loaded_end_index) {
            state->list_query_offset = desired_query_offset;
            if (*io_action == MEM_CONSOLE_ACTION_NONE) {
                *io_action = MEM_CONSOLE_ACTION_REFRESH;
            }
        }
    } else {
        state->list_query_offset = 0;
    }

    {
        CoreResult result = mem_console_ui_push_themed_rect(render_ctx,
                                                            frame,
                                                            list_viewport,
                                                            6.0f,
                                                            CORE_THEME_COLOR_SURFACE_0);
        if (result.code != CORE_OK) return result;
    }

    {
        CoreResult result = kit_ui_clip_push(ui_ctx, frame, list_content_viewport);
        if (result.code != CORE_OK) return result;

        if (state->visible_count == 0) {
            const char *empty_text = "NO MATCHES FOR THE CURRENT FILTER";
            if (state->matching_count > 0) {
                empty_text = "LOADING LIST WINDOW...";
            }
            result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                          frame,
                                                          (KitRenderRect){
                                                              list_content_viewport.x + 8.0f,
                                                              list_content_viewport.y + 8.0f,
                                                              list_content_viewport.width - 14.0f,
                                                              22.0f
                                                          },
                                                          empty_text,
                                                          CORE_THEME_COLOR_TEXT_MUTED,
                                                          CORE_FONT_ROLE_UI_REGULAR,
                                                          CORE_FONT_TEXT_SIZE_CAPTION);
            if (result.code != CORE_OK) {
                (void)kit_ui_clip_pop(ui_ctx, frame);
                return result;
            }
        } else {
            for (i = 0; i < state->visible_count; ++i) {
                int row_index = state->visible_start_index + i;
                float viewport_bottom = list_content_viewport.y + list_content_viewport.height;

                item_rect = (KitRenderRect){
                    list_content_viewport.x + 4.0f,
                    list_content_viewport.y + 4.0f + ((float)row_index * row_pitch) - state->list_scroll,
                    list_content_viewport.width - 6.0f,
                    layout_cfg->list_item_h
                };
                if (item_rect.width <= 0.0f) {
                    continue;
                }
                if (item_rect.y + item_rect.height < list_content_viewport.y || item_rect.y > viewport_bottom) {
                    continue;
                }

                {
                    (void)snprintf(state->list_item_labels[i],
                                   sizeof(state->list_item_labels[i]),
                                   "%lld %s%s%s%s%s%s",
                                   (long long)state->visible_items[i].id,
                                   state->visible_items[i].pinned ? "[P] " : "",
                                   state->visible_items[i].canonical ? "[C] " : "",
                                   state->visible_items[i].project_key[0] ? "[" : "",
                                   state->visible_items[i].project_key[0] ? state->visible_items[i].project_key : "",
                                   state->visible_items[i].project_key[0] ? "] " : "",
                                   state->visible_items[i].title[0] ? state->visible_items[i].title : "UNTITLED");
                }

                button_result = kit_ui_eval_button(item_rect,
                                                   input,
                                                   !has_any_edit_mode);
                if (button_result.clicked) {
                    state->selected_item_id = state->visible_items[i].id;
                    *io_action = MEM_CONSOLE_ACTION_REFRESH;
                }
                if (state->visible_items[i].id == state->selected_item_id) {
                    button_result.state = KIT_UI_STATE_ACTIVE;
                }

                result = mem_console_ui_draw_button_custom(ui_ctx,
                                                           frame,
                                                           item_rect,
                                                           state->list_item_labels[i],
                                                           button_result.state,
                                                           CORE_FONT_ROLE_UI_REGULAR,
                                                           CORE_FONT_TEXT_SIZE_CAPTION);
                if (result.code != CORE_OK) {
                    (void)kit_ui_clip_pop(ui_ctx, frame);
                    return result;
                }
            }
        }

        result = kit_ui_clip_pop(ui_ctx, frame);
        if (result.code != CORE_OK) return result;
    }

    {
        CoreResult result = kit_ui_draw_scrollbar(ui_ctx,
                                                  frame,
                                                  list_viewport,
                                                  state->list_scroll,
                                                  content_height);
        if (result.code != CORE_OK) return result;
    }

    format_text_for_width(state->status_draw_line,
                          sizeof(state->status_draw_line),
                          state->status_line,
                          left_layout.bounds.width - (ui_ctx->style.padding * 2.0f),
                          CORE_FONT_TEXT_SIZE_CAPTION);
    {
        CoreResult result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                                  frame,
                                                                  (KitRenderRect){
                                                                      left_layout.bounds.x,
                                                                      state->left_pane.y + state->left_pane.height - (layout_cfg->left_status_h + 4.0f),
                                                                      left_layout.bounds.width,
                                                                      layout_cfg->left_status_h
                                                                  },
                                                                  state->status_draw_line,
                                                                  CORE_THEME_COLOR_TEXT_MUTED,
                                                                  CORE_FONT_ROLE_UI_REGULAR,
                                                                  CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) return result;
    }

    return core_result_ok();
}
