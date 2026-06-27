#include "mem_console_ui.h"
#include "mem_console_layout_config.h"
#include "mem_console_pane_layout.h"
#include "mem_console_ui_common.h"
#include "mem_console_ui_chrome.h"
#include "mem_console_ui_detail_section.h"
#include "mem_console_ui_graph_controls.h"
#include "mem_console_ui_graph_panel.h"
#include "mem_console_ui_left_section.h"
#include "mem_console_visual_artifact.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

static void mem_console_ui_build_visible_path_text(const KitRenderContext *render_ctx,
                                                   const char *full_text,
                                                   int cursor_index,
                                                   float max_width,
                                                   char *out_text,
                                                   size_t out_cap,
                                                   int *out_cursor_index,
                                                   int *out_visible_start) {
    int full_len;
    int cursor;
    int start;
    const char *ellipsis = "...";
    float width;
    char candidate[896];

    if (!full_text || !out_text || out_cap == 0u) {
        return;
    }

    out_text[0] = '\0';
    if (out_cursor_index) {
        *out_cursor_index = 0;
    }
    if (out_visible_start) {
        *out_visible_start = 0;
    }

    full_len = (int)strlen(full_text);
    cursor = mem_console_ui_clamp_cursor_for_text(full_text, cursor_index);
    width = mem_console_ui_measure_text_width_px(render_ctx,
                                                 CORE_FONT_ROLE_UI_REGULAR,
                                                 CORE_FONT_TEXT_SIZE_PARAGRAPH,
                                                 full_text);
    if (width <= max_width || full_len == 0) {
        (void)snprintf(out_text, out_cap, "%s", full_text);
        if (out_cursor_index) {
            *out_cursor_index = cursor;
        }
        return;
    }

    start = cursor;
    while (start > 0) {
        (void)snprintf(candidate, sizeof(candidate), "%s%s", start > 1 ? ellipsis : "", full_text + start - 1);
        width = mem_console_ui_measure_text_width_px(render_ctx,
                                                     CORE_FONT_ROLE_UI_REGULAR,
                                                     CORE_FONT_TEXT_SIZE_PARAGRAPH,
                                                     candidate);
        if (width > max_width) {
            break;
        }
        start -= 1;
    }

    if (start > 0) {
        (void)snprintf(out_text, out_cap, "%s%s", ellipsis, full_text + start);
        if (out_cursor_index) {
            *out_cursor_index = 3 + (cursor - start);
        }
    } else {
        (void)snprintf(out_text, out_cap, "%s", full_text);
        if (out_cursor_index) {
            *out_cursor_index = cursor;
        }
    }

    if (out_visible_start) {
        *out_visible_start = start;
    }
}

static float mem_console_ui_measure_prefix_width_local(const KitRenderContext *render_ctx,
                                                       const char *text,
                                                       int prefix_len) {
    char prefix[896];
    int len;

    if (!text) {
        return 0.0f;
    }
    len = mem_console_ui_clamp_cursor_for_text(text, prefix_len);
    if (len <= 0) {
        return 0.0f;
    }
    if (len >= (int)sizeof(prefix)) {
        len = (int)sizeof(prefix) - 1;
    }
    memcpy(prefix, text, (size_t)len);
    prefix[len] = '\0';
    return mem_console_ui_measure_text_width_px(render_ctx,
                                                CORE_FONT_ROLE_UI_REGULAR,
                                                CORE_FONT_TEXT_SIZE_PARAGRAPH,
                                                prefix);
}

static CoreResult mem_console_ui_draw_db_modal(KitRenderContext *render_ctx,
                                               KitUiContext *ui_ctx,
                                               KitRenderFrame *frame,
                                               MemConsoleState *state,
                                               const KitUiInputState *input,
                                               MemConsoleAction *out_action) {
    KitRenderRect overlay;
    KitRenderRect modal;
    KitRenderRect title_rect;
    KitRenderRect hint_rect;
    KitRenderRect input_rect;
    KitRenderRect suffix_rect;
    KitRenderRect resolved_rect;
    KitRenderRect list_header_rect;
    KitRenderRect list_rect;
    KitRenderRect list_body_rect;
    KitRenderRect buttons_rect;
    KitRenderRect cancel_rect;
    KitRenderRect apply_rect;
    KitUiButtonResult button_result;
    CoreResult result;
    KitRenderColor overlay_color;
    const char *title_text;
    float button_gap = 8.0f;
    float button_width;
    float suffix_width;
    float editable_width;
    float text_origin_x;
    float list_row_h = 22.0f;
    float list_row_gap = 4.0f;
    int list_max_rows = 0;
    int list_rows_drawn = 0;
    int list_start_index = 0;
    int list_index = 0;
    int visible_cursor_index = 0;
    int visible_start = 0;
    int visible_bias = 0;
    int visible_text_len = 0;
    int visible_selection_start = 0;
    int visible_selection_end = 0;
    const char *suffix_text = ".sqlite";
    int show_suffix;

    if (!render_ctx || !ui_ctx || !frame || !state || !input || !out_action) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid db modal draw request" };
    }

    overlay = (KitRenderRect){ 0.0f, 0.0f, frame->width_px, frame->height_px };
    modal = (KitRenderRect){
        (frame->width_px * 0.5f) - 300.0f,
        (frame->height_px * 0.5f) - 180.0f,
        600.0f,
        state->db_modal_input_root_mode ? 260.0f : 420.0f
    };
    if (modal.x < 24.0f) modal.x = 24.0f;
    if (modal.y < 24.0f) modal.y = 24.0f;
    if (modal.x + modal.width > frame->width_px - 24.0f) {
        modal.width = frame->width_px - 48.0f;
    }
    if (modal.y + modal.height > frame->height_px - 24.0f) {
        modal.height = frame->height_px - 48.0f;
    }

    (void)mem_console_db_picker_build_path(state,
                                           state->db_modal_resolved_path,
                                           sizeof(state->db_modal_resolved_path));

    result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_0, &overlay_color);
    if (result.code != CORE_OK) {
        return result;
    }
    overlay_color.a = 200u;
    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      overlay, 0.0f, overlay_color, kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }

    result = mem_console_ui_push_themed_rect(render_ctx, frame, modal, 14.0f, CORE_THEME_COLOR_SURFACE_1);
    if (result.code != CORE_OK) {
        return result;
    }

    title_rect = (KitRenderRect){ modal.x + 18.0f, modal.y + 12.0f, modal.width - 36.0f, 26.0f };
    hint_rect = (KitRenderRect){ modal.x + 18.0f, modal.y + 38.0f, modal.width - 36.0f, 38.0f };
    input_rect = (KitRenderRect){ modal.x + 18.0f, modal.y + 82.0f, modal.width - 36.0f, 36.0f };
    resolved_rect = (KitRenderRect){ modal.x + 18.0f, modal.y + 122.0f, modal.width - 36.0f, 28.0f };
    list_header_rect = (KitRenderRect){ modal.x + 18.0f, modal.y + 156.0f, modal.width - 36.0f, 20.0f };
    list_rect = (KitRenderRect){ modal.x + 18.0f, modal.y + 178.0f, modal.width - 36.0f, modal.height - 236.0f };
    buttons_rect = (KitRenderRect){ modal.x + 18.0f, modal.y + modal.height - 44.0f, modal.width - 36.0f, 28.0f };
    list_body_rect = (KitRenderRect){
        list_rect.x + 6.0f,
        list_rect.y + 24.0f,
        list_rect.width - 12.0f,
        list_rect.height - 30.0f
    };

    if (state->db_modal_input_root_mode) {
        title_text = "Set Input Root";
    } else {
        title_text = state->db_modal_create_mode ? "Create Or Switch Database" : "Load Or Switch Database";
    }
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  title_rect,
                                                  title_text,
                                                  CORE_THEME_COLOR_TEXT_PRIMARY,
                                                  CORE_FONT_ROLE_UI_BOLD,
                                                  CORE_FONT_TEXT_SIZE_TITLE);
    if (result.code != CORE_OK) {
        return result;
    }

    result = mem_console_ui_draw_wrapped_text_block(ui_ctx,
                                                    frame,
                                                    state->detail_connection_summary_lines,
                                                    MEM_CONSOLE_DETAIL_CONNECTION_WRAP_LINE_LIMIT,
                                                    hint_rect,
                                                    state->db_modal_input_root_mode
                                                        ? "Enter or browse the database input root directory. Folder will be created if missing."
                                                        : (state->db_modal_create_mode
                                                               ? "Create mode: enter DB name (saved under input root) or explicit path. Missing directories and DB file are created."
                                                               : "Load/switch mode: enter exact DB path. Path is used directly with no hidden rewrite."),
                                                    CORE_THEME_COLOR_TEXT_MUTED,
                                                    CORE_FONT_TEXT_SIZE_CAPTION,
                                                    2);
    if (result.code != CORE_OK) {
        return result;
    }

    result = mem_console_ui_push_themed_rect(render_ctx, frame, input_rect, 8.0f, CORE_THEME_COLOR_SURFACE_2);
    if (result.code != CORE_OK) {
        return result;
    }

    show_suffix = 0;
    if (!state->db_modal_input_root_mode && state->db_modal_create_mode) {
        show_suffix = strlen(state->db_modal_text) < 7u ||
                      strcmp(state->db_modal_text + (strlen(state->db_modal_text) - 7u), ".sqlite") != 0;
    }
    suffix_width = mem_console_ui_measure_text_width_px(render_ctx,
                                                        CORE_FONT_ROLE_UI_REGULAR,
                                                        CORE_FONT_TEXT_SIZE_PARAGRAPH,
                                                        suffix_text);
    if (suffix_width < 40.0f) {
        suffix_width = 40.0f;
    }
    suffix_rect = (KitRenderRect){
        input_rect.x + input_rect.width - suffix_width - 10.0f,
        input_rect.y + 4.0f,
        suffix_width,
        input_rect.height - 8.0f
    };
    editable_width = input_rect.width - 16.0f - (show_suffix ? suffix_width : 0.0f);
    mem_console_ui_build_visible_path_text(render_ctx,
                                           state->db_modal_text,
                                           state->db_modal_cursor,
                                           editable_width - (ui_ctx->style.padding * 2.0f),
                                           state->db_modal_visible_text,
                                           sizeof(state->db_modal_visible_text),
                                           &visible_cursor_index,
                                           &visible_start);
    visible_text_len = (int)strlen(state->db_modal_visible_text);
    visible_bias = visible_start > 0 ? 3 : 0;
    text_origin_x = input_rect.x + 8.0f + ui_ctx->style.padding;

    if (mem_console_db_picker_has_selection(state)) {
        visible_selection_start = state->db_modal_selection_start - visible_start + visible_bias;
        visible_selection_end = state->db_modal_selection_end - visible_start + visible_bias;
        if (visible_selection_start < 0) visible_selection_start = 0;
        if (visible_selection_end < 0) visible_selection_end = 0;
        if (visible_selection_start > visible_text_len) visible_selection_start = visible_text_len;
        if (visible_selection_end > visible_text_len) visible_selection_end = visible_text_len;
        if (visible_selection_end > visible_selection_start) {
            KitRenderColor selection_color;
            float start_x = text_origin_x +
                            mem_console_ui_measure_prefix_width_local(render_ctx,
                                                                      state->db_modal_visible_text,
                                                                      visible_selection_start);
            float end_x = text_origin_x +
                          mem_console_ui_measure_prefix_width_local(render_ctx,
                                                                    state->db_modal_visible_text,
                                                                    visible_selection_end);
            result = mem_console_ui_resolve_theme_color(render_ctx,
                                                        CORE_THEME_COLOR_ACCENT_PRIMARY,
                                                        &selection_color);
            if (result.code != CORE_OK) {
                return result;
            }
            selection_color.a = 72u;
            result = kit_render_push_rect(frame,
                                          &(KitRenderRectCommand){
                                              (KitRenderRect){
                                                  start_x,
                                                  input_rect.y + 8.0f,
                                                  end_x - start_x,
                                                  input_rect.height - 16.0f
                                              },
                                              4.0f,
                                              selection_color,
                                              kit_render_identity_transform()
                                          });
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    result = mem_console_ui_draw_editable_line(ui_ctx,
                                               render_ctx,
                                               frame,
                                               (KitRenderRect){
                                                   input_rect.x + 8.0f,
                                                   input_rect.y + 4.0f,
                                                   editable_width,
                                                   input_rect.height - 8.0f
                                               },
                                               state->db_modal_visible_text,
                                               CORE_THEME_COLOR_TEXT_PRIMARY,
                                               CORE_FONT_ROLE_UI_REGULAR,
                                               CORE_FONT_TEXT_SIZE_PARAGRAPH,
                                               state->input_target == MEM_CONSOLE_INPUT_DB_PATH,
                                               visible_cursor_index);
    if (result.code != CORE_OK) {
        return result;
    }

    if (show_suffix) {
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      suffix_rect,
                                                      suffix_text,
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    if (input->mouse_released &&
        kit_ui_point_in_rect(input_rect, input->mouse_x, input->mouse_y)) {
        mem_console_input_target_set(state, MEM_CONSOLE_INPUT_DB_PATH);
    }

    if (input->mouse_pressed && kit_ui_point_in_rect(input_rect, input->mouse_x, input->mouse_y)) {
        int local_cursor = mem_console_ui_cursor_index_for_click(state->db_modal_visible_text,
                                                                 render_ctx,
                                                                 input->mouse_x,
                                                                 text_origin_x,
                                                                 CORE_FONT_ROLE_UI_REGULAR,
                                                                 CORE_FONT_TEXT_SIZE_PARAGRAPH);
        mem_console_input_target_set(state, MEM_CONSOLE_INPUT_DB_PATH);
        mem_console_db_picker_begin_selection(state, visible_start + (local_cursor - visible_bias > 0 ? local_cursor - visible_bias : 0));
    } else if (state->db_modal_drag_select_active && input->mouse_down) {
        int local_cursor = mem_console_ui_cursor_index_for_click(state->db_modal_visible_text,
                                                                 render_ctx,
                                                                 input->mouse_x,
                                                                 text_origin_x,
                                                                 CORE_FONT_ROLE_UI_REGULAR,
                                                                 CORE_FONT_TEXT_SIZE_PARAGRAPH);
        mem_console_db_picker_update_selection(state, visible_start + (local_cursor - visible_bias > 0 ? local_cursor - visible_bias : 0));
    } else if (state->db_modal_drag_select_active && input->mouse_released) {
        int local_cursor = mem_console_ui_cursor_index_for_click(state->db_modal_visible_text,
                                                                 render_ctx,
                                                                 input->mouse_x,
                                                                 text_origin_x,
                                                                 CORE_FONT_ROLE_UI_REGULAR,
                                                                 CORE_FONT_TEXT_SIZE_PARAGRAPH);
        mem_console_db_picker_update_selection(state, visible_start + (local_cursor - visible_bias > 0 ? local_cursor - visible_bias : 0));
        mem_console_db_picker_end_selection(state);
    }

    if (state->db_modal_resolved_path[0] != '\0') {
        (void)snprintf(state->db_modal_resolved_line,
                       sizeof(state->db_modal_resolved_line),
                       state->db_modal_input_root_mode ? "Resolved Root: %s" : "Resolved: %s",
                       state->db_modal_resolved_path);
        result = kit_ui_clip_push(ui_ctx, frame, resolved_rect);
        if (result.code != CORE_OK) {
            return result;
        }
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      resolved_rect,
                                                      state->db_modal_resolved_line,
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code == CORE_OK) {
            result = kit_ui_clip_pop(ui_ctx, frame);
        } else {
            (void)kit_ui_clip_pop(ui_ctx, frame);
        }
        if (result.code != CORE_OK) {
            return result;
        }
    }

    if (!state->db_modal_input_root_mode) {
        KitRenderRect active_db_rect = {
            list_rect.x + 6.0f,
            list_rect.y + 4.0f,
            list_rect.width - 12.0f,
            18.0f
        };
        result = mem_console_ui_push_themed_rect(render_ctx, frame, list_rect, 8.0f, CORE_THEME_COLOR_SURFACE_0);
        if (result.code != CORE_OK) {
            return result;
        }
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      list_header_rect,
                                                      "Databases In Input Root",
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_MEDIUM,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }
        (void)snprintf(state->db_modal_active_line,
                       sizeof(state->db_modal_active_line),
                       "Active DB: %s",
                       state->db_path ? state->db_path : "(none)");
        result = kit_ui_clip_push(ui_ctx, frame, active_db_rect);
        if (result.code != CORE_OK) {
            return result;
        }
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      active_db_rect,
                                                      state->db_modal_active_line,
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code == CORE_OK) {
            result = kit_ui_clip_pop(ui_ctx, frame);
        } else {
            (void)kit_ui_clip_pop(ui_ctx, frame);
        }
        if (result.code != CORE_OK) {
            return result;
        }
        list_max_rows = (int)((list_body_rect.height + list_row_gap) / (list_row_h + list_row_gap));
        if (list_max_rows < 1) {
            list_max_rows = 1;
        }
        if (state->db_picker_entry_count <= 0) {
            result = kit_ui_clip_push(ui_ctx, frame, list_body_rect);
            if (result.code != CORE_OK) {
                return result;
            }
            result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                          frame,
                                                          list_body_rect,
                                                          "No .sqlite DBs found in input root.",
                                                          CORE_THEME_COLOR_TEXT_MUTED,
                                                          CORE_FONT_ROLE_UI_REGULAR,
                                                          CORE_FONT_TEXT_SIZE_CAPTION);
            if (result.code == CORE_OK) {
                result = kit_ui_clip_pop(ui_ctx, frame);
            } else {
                (void)kit_ui_clip_pop(ui_ctx, frame);
            }
            if (result.code != CORE_OK) {
                return result;
            }
        } else {
            if (state->db_picker_selected_index >= list_max_rows) {
                list_start_index = state->db_picker_selected_index - list_max_rows + 1;
            }
            result = kit_ui_clip_push(ui_ctx, frame, list_body_rect);
            if (result.code != CORE_OK) {
                return result;
            }
            for (list_index = list_start_index;
                 list_index < state->db_picker_entry_count && list_rows_drawn < list_max_rows;
                 ++list_index, ++list_rows_drawn) {
                KitRenderRect row_rect = {
                    list_rect.x + 6.0f,
                    list_rect.y + 26.0f + (float)list_rows_drawn * (list_row_h + list_row_gap),
                    list_rect.width - 12.0f,
                    list_row_h
                };
                int button_id = 4200 + list_index;
                int selected = list_index == state->db_picker_selected_index;
                KitUiWidgetState draw_state;
                button_result = kit_ui_eval_button(row_rect, input, button_id);
                if (button_result.clicked) {
                    state->db_picker_selected_index = list_index;
                    (void)snprintf(state->db_modal_text, sizeof(state->db_modal_text), "%s", state->db_picker_entry_paths[list_index]);
                    state->db_modal_cursor = (int)strlen(state->db_modal_text);
                    state->db_modal_selection_anchor = 0;
                    state->db_modal_selection_start = 0;
                    state->db_modal_selection_end = state->db_modal_cursor;
                }
                draw_state = button_result.state;
                if (selected && draw_state == KIT_UI_STATE_NORMAL) {
                    draw_state = KIT_UI_STATE_ACTIVE;
                }
                result = mem_console_ui_draw_button_custom(ui_ctx,
                                                           frame,
                                                           row_rect,
                                                           state->db_picker_entry_names[list_index][0]
                                                               ? state->db_picker_entry_names[list_index]
                                                               : state->db_picker_entry_paths[list_index],
                                                           draw_state,
                                                           CORE_FONT_ROLE_UI_REGULAR,
                                                           CORE_FONT_TEXT_SIZE_CAPTION);
                if (result.code != CORE_OK) {
                    (void)kit_ui_clip_pop(ui_ctx, frame);
                    return result;
                }
            }
            result = kit_ui_clip_pop(ui_ctx, frame);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    button_width = (buttons_rect.width - button_gap) * 0.5f;
    cancel_rect = (KitRenderRect){ buttons_rect.x, buttons_rect.y, button_width, buttons_rect.height };
    apply_rect = (KitRenderRect){ buttons_rect.x + button_width + button_gap, buttons_rect.y, buttons_rect.width - button_width - button_gap, buttons_rect.height };

    button_result = kit_ui_eval_button(cancel_rect, input, 4101);
    if (button_result.clicked && *out_action == MEM_CONSOLE_ACTION_NONE) {
        *out_action = MEM_CONSOLE_ACTION_CANCEL_DB_PICKER;
    }
    result = mem_console_ui_draw_button_custom(ui_ctx,
                                               frame,
                                               cancel_rect,
                                               "CANCEL",
                                               button_result.state,
                                               CORE_FONT_ROLE_UI_MEDIUM,
                                               CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }

    button_result = kit_ui_eval_button(apply_rect, input, 4102);
    if (button_result.clicked && *out_action == MEM_CONSOLE_ACTION_NONE) {
        *out_action = MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER;
    }
    result = mem_console_ui_draw_button_custom(ui_ctx,
                                               frame,
                                               apply_rect,
                                               state->db_modal_input_root_mode
                                                   ? "APPLY ROOT"
                                                   : (state->db_modal_create_mode ? "CREATE / LOAD" : "LOAD / SWITCH"),
                                               button_result.state,
                                               CORE_FONT_ROLE_UI_MEDIUM,
                                               CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }

    return core_result_ok();
}

int run_frame(KitRenderContext *render_ctx,
              KitUiContext *ui_ctx,
              MemConsoleState *state,
              const KitUiInputState *input,
              int frame_width,
              int frame_height,
              int wheel_y,
              MemConsoleAction *out_action) {
    KitRenderCommand commands[4096];
    KitRenderCommandBuffer command_buffer;
    KitRenderFrame frame;
    KitUiStackLayout right_layout;
    CoreResult result;
    KitRenderColor clear_color;
    int draw_width;
    int draw_height;
    int has_any_edit_mode;
    int authoring_active;
    KitUiInputState blocked_input;
    const MemConsoleLayoutConfig *layout_cfg;
    int visual_artifact_result;

    if (!render_ctx || !ui_ctx || !state || !input || !out_action) {
        fprintf(stderr, "mem_console: run_frame invalid args\n");
        return 1;
    }
    layout_cfg = mem_console_layout_config_get();

    *out_action = MEM_CONSOLE_ACTION_NONE;
    draw_width = frame_width;
    draw_height = frame_height;
    if (draw_width < layout_cfg->min_frame_width) {
        draw_width = layout_cfg->min_frame_width;
    }
    if (draw_height < layout_cfg->min_frame_height) {
        draw_height = layout_cfg->min_frame_height;
    }

    authoring_active = mem_console_workspace_authoring_host_active(&state->workspace_authoring);
    mem_console_workspace_authoring_host_set_viewport(&state->workspace_authoring,
                                                      (uint32_t)draw_width,
                                                      (uint32_t)draw_height);

    blocked_input = *input;
    if (state->db_modal_open || authoring_active) {
        blocked_input.mouse_down = 0;
        blocked_input.mouse_pressed = 0;
        blocked_input.mouse_released = 0;
    }

    if (!state->db_modal_open && !authoring_active && input->mouse_pressed) {
        int left_drag_started = mem_console_ui_left_begin_panel_drag(state,
                                                                     layout_cfg,
                                                                     ui_ctx->style.gap,
                                                                     input->mouse_x,
                                                                     input->mouse_y);
        if (!left_drag_started) {
            (void)mem_console_pane_layout_begin_drag(state,
                                                     layout_cfg,
                                                     draw_width,
                                                     draw_height,
                                                     input->mouse_x,
                                                     input->mouse_y);
        }
    }
    if (!state->db_modal_open && !authoring_active && state->left_panel_drag_active && input->mouse_down) {
        (void)mem_console_ui_left_update_panel_drag(state,
                                                    layout_cfg,
                                                    ui_ctx->style.gap,
                                                    input->mouse_y);
    } else if (!state->db_modal_open && !authoring_active && state->pane_drag_active && input->mouse_down) {
        (void)mem_console_pane_layout_update_drag(state,
                                                  layout_cfg,
                                                  draw_width,
                                                  draw_height,
                                                  input->mouse_x,
                                                  input->mouse_y);
    }
    if ((!state->db_modal_open && input->mouse_released) || authoring_active) {
        if (state->left_panel_drag_active) {
            mem_console_ui_left_end_panel_drag(state);
        }
        if (state->pane_drag_active) {
            mem_console_pane_layout_end_drag(state);
        }
    }

    if (state->left_panel_drag_active) {
        blocked_input.mouse_down = 0;
        blocked_input.mouse_pressed = 0;
        blocked_input.mouse_released = 0;
    }

    compute_layout(state, draw_width, draw_height);

    command_buffer.commands = commands;
    command_buffer.capacity = 4096u;
    command_buffer.count = 0u;
    kit_ui_clip_stack_reset(ui_ctx);

    result = kit_render_begin_frame(render_ctx,
                                    (uint32_t)draw_width,
                                    (uint32_t)draw_height,
                                    &command_buffer,
                                    &frame);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: kit_render_begin_frame failed: %d\n", (int)result.code);
        if (result.code == CORE_ERR_IO) {
            return MEM_CONSOLE_FRAME_RECOVERABLE;
        }
        return MEM_CONSOLE_FRAME_FATAL;
    }

    result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_0, &clear_color);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: mem_console_ui_resolve_theme_color failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }
    result = kit_render_push_clear(&frame, clear_color);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_render_push_clear failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }

    result = mem_console_ui_draw_root_chrome(render_ctx, &frame, state, layout_cfg);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: mem_console_ui_draw_root_chrome failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }

    has_any_edit_mode = state->title_edit_mode || state->body_edit_mode || state->db_modal_open;

    result = kit_ui_clip_push(ui_ctx, &frame, state->left_pane);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_ui_clip_push(left_pane) failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }
    result = mem_console_ui_draw_left_section(render_ctx,
                                              ui_ctx,
                                              &frame,
                                              state,
                                              &blocked_input,
                                              layout_cfg,
                                              wheel_y,
                                              has_any_edit_mode,
                                              out_action);
    if (result.code != CORE_OK) {
        (void)kit_ui_clip_pop(ui_ctx, &frame);
        fprintf(stderr,
                "mem_console: mem_console_ui_draw_left_section failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }
    result = kit_ui_clip_pop(ui_ctx, &frame);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_ui_clip_pop(left_pane) failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }

    result = kit_ui_clip_push(ui_ctx, &frame, state->pane_right_detail);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_ui_clip_push(pane_right_detail) failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }
    result = mem_console_ui_draw_detail_section(render_ctx,
                                                ui_ctx,
                                                &frame,
                                                state,
                                                &blocked_input,
                                                wheel_y,
                                                layout_cfg,
                                                &right_layout,
                                                out_action);
    if (result.code != CORE_OK) {
        (void)kit_ui_clip_pop(ui_ctx, &frame);
        fprintf(stderr,
                "mem_console: mem_console_ui_draw_detail_section failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }
    result = mem_console_ui_draw_graph_controls(render_ctx,
                                                ui_ctx,
                                                &frame,
                                                state,
                                                &blocked_input,
                                                layout_cfg,
                                                has_any_edit_mode,
                                                &right_layout,
                                                out_action);
    if (result.code != CORE_OK) {
        (void)kit_ui_clip_pop(ui_ctx, &frame);
        fprintf(stderr,
                "mem_console: mem_console_ui_draw_graph_controls failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }
    result = kit_ui_clip_pop(ui_ctx, &frame);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_ui_clip_pop(pane_right_detail) failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }

    result = kit_ui_clip_push(ui_ctx, &frame, state->pane_right_graph);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_ui_clip_push(pane_right_graph) failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }
    result = mem_console_ui_draw_graph_panel(render_ctx,
                                             ui_ctx,
                                             &frame,
                                             state,
                                             &blocked_input,
                                             layout_cfg,
                                             &right_layout,
                                             wheel_y,
                                             out_action);
    if (result.code != CORE_OK) {
        (void)kit_ui_clip_pop(ui_ctx, &frame);
        fprintf(stderr,
                "mem_console: mem_console_ui_draw_graph_panel failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }
    result = kit_ui_clip_pop(ui_ctx, &frame);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_ui_clip_pop(pane_right_graph) failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }

    if (state->db_modal_open) {
        result = mem_console_ui_draw_db_modal(render_ctx, ui_ctx, &frame, state, input, out_action);
        if (result.code != CORE_OK) {
            fprintf(stderr,
                    "mem_console: mem_console_ui_draw_db_modal failed: %d (%s)\n",
                    (int)result.code,
                    result.message ? result.message : "no message");
            return 1;
        }
    }

    result = mem_console_workspace_authoring_overlay_render(render_ctx,
                                                            ui_ctx,
                                                            &frame,
                                                            state,
                                                            draw_width,
                                                            draw_height);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: mem_console_workspace_authoring_overlay_render failed: %d (%s)\n",
                (int)result.code,
                result.message ? result.message : "no message");
        return 1;
    }

    result = kit_render_end_frame(render_ctx, &frame);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: kit_render_end_frame failed: %d\n", (int)result.code);
        if (result.code == CORE_ERR_IO) {
            return MEM_CONSOLE_FRAME_RECOVERABLE;
        }
        return MEM_CONSOLE_FRAME_FATAL;
    }

    visual_artifact_result = mem_console_visual_artifact_capture_if_requested(&command_buffer,
                                                                              (uint32_t)draw_width,
                                                                              (uint32_t)draw_height);
    if (visual_artifact_result < 0) {
        return MEM_CONSOLE_FRAME_FATAL;
    }
    if (visual_artifact_result > 0 && SDL_WasInit(SDL_INIT_VIDEO) != 0u) {
        SDL_Event quit_event;
        memset(&quit_event, 0, sizeof(quit_event));
        quit_event.type = SDL_QUIT;
        SDL_PushEvent(&quit_event);
    }

    return MEM_CONSOLE_FRAME_OK;
}
