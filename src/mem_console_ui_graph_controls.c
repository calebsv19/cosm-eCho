#include "mem_console_ui_graph_controls.h"

#include "mem_console_ui_common.h"

#include <stdio.h>
#include <string.h>

static const char *k_graph_hops_labels[] = {
    "1",
    "2",
    "3",
    "4",
    "5"
};

static float shrink_with_floor(float value, float *io_deficit, float floor_value) {
    float next_value = value;

    if (!io_deficit || *io_deficit <= 0.0f) {
        return value;
    }

    if (next_value > floor_value) {
        float reducible = next_value - floor_value;
        float reduction = reducible < *io_deficit ? reducible : *io_deficit;
        next_value -= reduction;
        *io_deficit -= reduction;
    }
    return next_value;
}

static float measure_compact_button_width(const KitRenderContext *render_ctx,
                                          const char *text,
                                          CoreFontRoleId font_role,
                                          CoreFontTextSizeTier text_tier,
                                          float pad_x,
                                          float min_w,
                                          float max_w) {
    float w = mem_console_ui_measure_text_width_px(render_ctx, font_role, text_tier, text);
    if (w < 0.0f) {
        w = 0.0f;
    }
    w += pad_x * 2.0f;
    if (w < min_w) {
        w = min_w;
    }
    if (w > max_w) {
        w = max_w;
    }
    return w;
}

static void layout_centered_button_row(KitRenderRect row,
                                       const float *widths,
                                       int count,
                                       float gap,
                                       KitRenderRect *out_rects) {
    float total_w = 0.0f;
    float x;
    int i;

    if (!widths || !out_rects || count <= 0) {
        return;
    }

    for (i = 0; i < count; ++i) {
        total_w += widths[i];
    }
    total_w += gap * (float)(count - 1);
    if (total_w > row.width) {
        total_w = row.width;
    }

    x = row.x + ((row.width - total_w) * 0.5f);
    for (i = 0; i < count; ++i) {
        out_rects[i] = (KitRenderRect){ x, row.y, widths[i], row.height };
        x += widths[i] + gap;
    }
}

static void fit_button_widths_to_row(float row_width,
                                     float *widths,
                                     const float *min_widths,
                                     int count,
                                     float min_gap,
                                     float max_gap,
                                     float *out_gap) {
    float width_sum = 0.0f;
    float deficit = 0.0f;
    float gap = min_gap;
    int i;

    if (!widths || !min_widths || !out_gap || count <= 0) {
        return;
    }

    for (i = 0; i < count; ++i) {
        width_sum += widths[i];
    }

    if (count > 1) {
        float min_total = width_sum + (min_gap * (float)(count - 1));
        if (row_width < min_total) {
            deficit = min_total - row_width;
            for (i = 0; i < count && deficit > 0.0f; ++i) {
                widths[i] = shrink_with_floor(widths[i], &deficit, min_widths[i]);
            }
        } else {
            gap = (row_width - width_sum) / (float)(count - 1);
            if (gap < min_gap) {
                gap = min_gap;
            }
            if (gap > max_gap) {
                gap = max_gap;
            }
        }
    }

    *out_gap = gap;
}

static void compute_graph_settings_metrics(const KitRenderContext *render_ctx,
                                           float row_width,
                                           float *out_gap,
                                           float *out_edge_label_w,
                                           float *out_edge_input_w,
                                           float *out_edge_apply_w,
                                           float *out_hops_label_w,
                                           float out_hop_btn_w[5]) {
    float gap = 6.0f;
    float edge_label_w = measure_compact_button_width(render_ctx,
                                                      "EDGE LIMIT",
                                                      CORE_FONT_ROLE_UI_MEDIUM,
                                                      CORE_FONT_TEXT_SIZE_CAPTION,
                                                      4.0f,
                                                      56.0f,
                                                      96.0f);
    float edge_input_w = 56.0f;
    float edge_apply_w = measure_compact_button_width(render_ctx,
                                                      "APPLY",
                                                      CORE_FONT_ROLE_UI_MEDIUM,
                                                      CORE_FONT_TEXT_SIZE_CAPTION,
                                                      6.0f,
                                                      44.0f,
                                                      72.0f);
    float hops_label_w = measure_compact_button_width(render_ctx,
                                                      "HOPS",
                                                      CORE_FONT_ROLE_UI_MEDIUM,
                                                      CORE_FONT_TEXT_SIZE_CAPTION,
                                                      3.0f,
                                                      28.0f,
                                                      44.0f);
    float hop_w = measure_compact_button_width(render_ctx,
                                               "5",
                                               CORE_FONT_ROLE_UI_REGULAR,
                                               CORE_FONT_TEXT_SIZE_CAPTION,
                                               6.0f,
                                               18.0f,
                                               28.0f);
    float required;
    float deficit = 0.0f;
    int i;

    required = edge_label_w + edge_input_w + edge_apply_w + hops_label_w +
               (hop_w * 5.0f) + (gap * 8.0f);

    if (row_width < required) {
        deficit = required - row_width;
        edge_label_w = shrink_with_floor(edge_label_w, &deficit, 48.0f);
        edge_input_w = shrink_with_floor(edge_input_w, &deficit, 44.0f);
        edge_apply_w = shrink_with_floor(edge_apply_w, &deficit, 40.0f);
        hops_label_w = shrink_with_floor(hops_label_w, &deficit, 24.0f);
        hop_w = shrink_with_floor(hop_w, &deficit, 16.0f);
    }

    if (out_gap) *out_gap = gap;
    if (out_edge_label_w) *out_edge_label_w = edge_label_w;
    if (out_edge_input_w) *out_edge_input_w = edge_input_w;
    if (out_edge_apply_w) *out_edge_apply_w = edge_apply_w;
    if (out_hops_label_w) *out_hops_label_w = hops_label_w;
    if (out_hop_btn_w) {
        for (i = 0; i < 5; ++i) {
            out_hop_btn_w[i] = hop_w;
        }
    }
}

static void place_graph_setting_rects(const KitRenderRect *row,
                                      float gap,
                                      float edge_label_w,
                                      float edge_input_w,
                                      float edge_apply_w,
                                      float hops_label_w,
                                      const float hop_btn_w[5],
                                      KitRenderRect *out_edge_label_rect,
                                      KitRenderRect *out_edge_input_rect,
                                      KitRenderRect *out_edge_apply_rect,
                                      KitRenderRect *out_hops_label_rect,
                                      KitRenderRect out_hop_btn_rects[5]) {
    float total_w;
    float x;
    float y;
    float h;
    int i;

    if (!row || !out_edge_label_rect || !out_edge_input_rect || !out_edge_apply_rect ||
        !out_hops_label_rect || !out_hop_btn_rects || !hop_btn_w) {
        return;
    }

    total_w = edge_label_w + edge_input_w + edge_apply_w + hops_label_w + (gap * 8.0f);
    for (i = 0; i < 5; ++i) {
        total_w += hop_btn_w[i];
    }

    x = row->x + ((row->width - total_w) * 0.5f);
    y = row->y + 1.0f;
    h = row->height - 2.0f;
    if (h < 12.0f) {
        h = 12.0f;
    }

    *out_edge_label_rect = (KitRenderRect){ x, y + 1.0f, edge_label_w, h - 2.0f };
    x += edge_label_w + gap;
    *out_edge_input_rect = (KitRenderRect){ x, y, edge_input_w, h };
    x += edge_input_w + gap;
    *out_edge_apply_rect = (KitRenderRect){ x, y, edge_apply_w, h };
    x += edge_apply_w + gap;
    *out_hops_label_rect = (KitRenderRect){ x, y + 1.0f, hops_label_w, h - 2.0f };
    x += hops_label_w + gap;

    for (i = 0; i < 5; ++i) {
        out_hop_btn_rects[i] = (KitRenderRect){ x, y, hop_btn_w[i], h };
        x += hop_btn_w[i] + gap;
    }
}

typedef struct GraphActionDef {
    const char *label;
    int enabled;
    int selected;
    MemConsoleAction action;
} GraphActionDef;

CoreResult mem_console_ui_draw_graph_controls(KitRenderContext *render_ctx,
                                              KitUiContext *ui_ctx,
                                              KitRenderFrame *frame,
                                              MemConsoleState *state,
                                              const KitUiInputState *input,
                                              const MemConsoleLayoutConfig *layout_cfg,
                                              int has_any_edit_mode,
                                              KitUiStackLayout *right_layout,
                                              MemConsoleAction *io_action) {
    KitRenderRect row;
    KitRenderRect graph_settings_row;
    KitRenderRect graph_edge_label_rect;
    KitRenderRect graph_edge_input_rect;
    KitRenderRect graph_edge_apply_rect;
    KitRenderRect graph_hops_label_rect;
    KitRenderRect graph_hop_btn_rects[5];
    KitRenderRect graph_hint_row;
    KitRenderRect action_bar;
    KitRenderRect action_row;
    KitRenderRect action_btn_rects[10];
    KitUiButtonResult button_result;
    CoreResult result;
    int graph_hops_index;
    int graph_hops_hovered_index;
    int graph_edge_input_active;
    float controls_gap;
    float edge_label_w;
    float edge_input_w;
    float edge_apply_w;
    float hops_label_w;
    float hop_btn_w[5];
    float action_gap = 0.0f;
    float action_widths[10];
    float action_min_widths[10];
    int i;
    GraphActionDef actions[10];

    if (!render_ctx || !ui_ctx || !frame || !state || !input || !layout_cfg || !right_layout || !io_action) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph controls draw request" };
    }

    graph_edge_input_active = state->input_target == MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT;

    result = kit_ui_stack_next(right_layout, layout_cfg->right_section_h, 0.0f, &row);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  row,
                                                  "GRAPH",
                                                  CORE_THEME_COLOR_TEXT_PRIMARY,
                                                  CORE_FONT_ROLE_UI_MEDIUM,
                                                  CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }

    if (state->graph_mode_enabled) {
        result = kit_ui_stack_next(right_layout, layout_cfg->graph_settings_h, 0.0f, &graph_settings_row);
        if (result.code != CORE_OK) {
            return result;
        }
        compute_graph_settings_metrics(render_ctx,
                                       graph_settings_row.width,
                                       &controls_gap,
                                       &edge_label_w,
                                       &edge_input_w,
                                       &edge_apply_w,
                                       &hops_label_w,
                                       hop_btn_w);
        place_graph_setting_rects(&graph_settings_row,
                                  controls_gap,
                                  edge_label_w,
                                  edge_input_w,
                                  edge_apply_w,
                                  hops_label_w,
                                  hop_btn_w,
                                  &graph_edge_label_rect,
                                  &graph_edge_input_rect,
                                  &graph_edge_apply_rect,
                                  &graph_hops_label_rect,
                                  graph_hop_btn_rects);

        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      graph_edge_label_rect,
                                                      "EDGE LIMIT",
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_MEDIUM,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }
        result = mem_console_ui_push_themed_rect(render_ctx,
                                                 frame,
                                                 graph_edge_input_rect,
                                                 6.0f,
                                                 CORE_THEME_COLOR_SURFACE_2);
        if (result.code != CORE_OK) {
            return result;
        }
        result = mem_console_ui_draw_editable_line(ui_ctx,
                                                   render_ctx,
                                                   frame,
                                                   (KitRenderRect){
                                                       graph_edge_input_rect.x + 6.0f,
                                                       graph_edge_input_rect.y + 2.0f,
                                                       graph_edge_input_rect.width - 12.0f,
                                                       graph_edge_input_rect.height - 4.0f
                                                   },
                                                   state->graph_edge_limit_text[0] ? state->graph_edge_limit_text : "48",
                                                   CORE_THEME_COLOR_TEXT_PRIMARY,
                                                   CORE_FONT_ROLE_UI_REGULAR,
                                                   CORE_FONT_TEXT_SIZE_CAPTION,
                                                   graph_edge_input_active,
                                                   state->graph_edge_limit_cursor);
        if (result.code != CORE_OK) {
            return result;
        }
        if (input->mouse_released &&
            !has_any_edit_mode &&
            kit_ui_point_in_rect(graph_edge_input_rect, input->mouse_x, input->mouse_y)) {
            float text_origin_x = graph_edge_input_rect.x + 6.0f + ui_ctx->style.padding;
            state->input_target = MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT;
            state->graph_edge_limit_cursor = mem_console_ui_cursor_index_for_click(state->graph_edge_limit_text,
                                                                                   render_ctx,
                                                                                   input->mouse_x,
                                                                                   text_origin_x,
                                                                                   CORE_FONT_ROLE_UI_REGULAR,
                                                                                   CORE_FONT_TEXT_SIZE_CAPTION);
            graph_edge_input_active = 1;
        }

        button_result = kit_ui_eval_button(graph_edge_apply_rect, input, 1);
        if (button_result.clicked) {
            int parsed_limit = mem_console_graph_edge_limit_parse(state->graph_edge_limit_text,
                                                                  state->graph_query_edge_limit);
            mem_console_graph_edge_limit_set(state, parsed_limit);
            if (*io_action == MEM_CONSOLE_ACTION_NONE) {
                *io_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
            }
        }
        result = mem_console_ui_draw_button_custom(ui_ctx,
                                                   frame,
                                                   graph_edge_apply_rect,
                                                   "APPLY",
                                                   button_result.state,
                                                   CORE_FONT_ROLE_UI_MEDIUM,
                                                   CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }

        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      graph_hops_label_rect,
                                                      "HOPS",
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_MEDIUM,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }

        graph_hops_index = mem_console_graph_hops_clamp(state->graph_query_hops) - MEM_CONSOLE_GRAPH_HOPS_MIN;
        graph_hops_hovered_index = -1;
        for (i = 0; i < 5; ++i) {
            if (kit_ui_point_in_rect(graph_hop_btn_rects[i], input->mouse_x, input->mouse_y)) {
                graph_hops_hovered_index = i;
                break;
            }
        }
        for (i = 0; i < 5; ++i) {
            KitUiWidgetState hop_state = KIT_UI_STATE_NORMAL;
            int enabled = 1;
            if (i == graph_hops_index) {
                hop_state = KIT_UI_STATE_ACTIVE;
            } else if (i == graph_hops_hovered_index) {
                hop_state = KIT_UI_STATE_HOVERED;
            }
            button_result = kit_ui_eval_button(graph_hop_btn_rects[i], input, enabled);
            if (button_result.clicked) {
                state->graph_query_hops = mem_console_graph_hops_clamp(MEM_CONSOLE_GRAPH_HOPS_MIN + i);
                if (*io_action == MEM_CONSOLE_ACTION_NONE) {
                    *io_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
                }
            }
            if (button_result.state == KIT_UI_STATE_HOVERED && hop_state != KIT_UI_STATE_ACTIVE) {
                hop_state = KIT_UI_STATE_HOVERED;
            }
            result = mem_console_ui_draw_button_custom(ui_ctx,
                                                       frame,
                                                       graph_hop_btn_rects[i],
                                                       k_graph_hops_labels[i],
                                                       hop_state,
                                                       CORE_FONT_ROLE_UI_REGULAR,
                                                       CORE_FONT_TEXT_SIZE_CAPTION);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    } else {
        state->input_target = state->input_target == MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT
                                  ? MEM_CONSOLE_INPUT_SEARCH
                                  : state->input_target;
        result = kit_ui_stack_next(right_layout, layout_cfg->graph_collapsed_hint_h, 0.0f, &graph_hint_row);
        if (result.code != CORE_OK) {
            return result;
        }
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      graph_hint_row,
                                                      "Graph controls are hidden while GRAPH: OFF.",
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    {
        float action_block_h = layout_cfg->action_button_h +
                               (layout_cfg->action_block_pad * 2.0f);
        result = kit_ui_stack_next(right_layout, action_block_h, 0.0f, &action_bar);
    }
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             action_bar,
                                             6.0f,
                                             CORE_THEME_COLOR_SURFACE_1);
    if (result.code != CORE_OK) {
        return result;
    }

    action_row = (KitRenderRect){
        action_bar.x + layout_cfg->action_block_pad,
        action_bar.y + layout_cfg->action_block_pad,
        action_bar.width - (layout_cfg->action_block_pad * 2.0f),
        layout_cfg->action_button_h
    };
    actions[0] = (GraphActionDef){ state->graph_mode_enabled ? "GRAPH ON" : "GRAPH OFF", 1, state->graph_mode_enabled ? 1 : 0, MEM_CONSOLE_ACTION_TOGGLE_GRAPH_MODE };
    actions[1] = (GraphActionDef){ "REFRESH", state->graph_mode_enabled, 0, MEM_CONSOLE_ACTION_REFRESH_GRAPH };
    actions[2] = (GraphActionDef){ "CENTER", state->graph_mode_enabled, 0, MEM_CONSOLE_ACTION_CENTER_GRAPH };
    actions[3] = (GraphActionDef){ "CENTER SEL", state->graph_mode_enabled && state->selected_item_id != 0, 0, MEM_CONSOLE_ACTION_CENTER_SELECTED };
    actions[4] = (GraphActionDef){ "P", state->selected_item_id != 0, state->selected_pinned ? 1 : 0, MEM_CONSOLE_ACTION_TOGGLE_PINNED };
    actions[5] = (GraphActionDef){ "C", state->selected_item_id != 0, state->selected_canonical ? 1 : 0, MEM_CONSOLE_ACTION_TOGGLE_CANONICAL };
    actions[6] = (GraphActionDef){ "NEW", 1, 0, MEM_CONSOLE_ACTION_CREATE_FROM_SEARCH };
    actions[7] = (GraphActionDef){ state->title_edit_mode ? "SAVE TITLE" : "EDIT TITLE",
                                   state->selected_item_id != 0,
                                   state->title_edit_mode ? 1 : 0,
                                   state->title_edit_mode ? MEM_CONSOLE_ACTION_SAVE_TITLE_EDIT
                                                          : MEM_CONSOLE_ACTION_BEGIN_TITLE_EDIT };
    actions[8] = (GraphActionDef){ state->body_edit_mode ? "SAVE BODY" : "EDIT BODY",
                                   state->selected_item_id != 0,
                                   state->body_edit_mode ? 1 : 0,
                                   state->body_edit_mode ? MEM_CONSOLE_ACTION_SAVE_BODY_EDIT
                                                         : MEM_CONSOLE_ACTION_BEGIN_BODY_EDIT };
    actions[9] = (GraphActionDef){ "CANCEL", has_any_edit_mode, 0,
                                   state->title_edit_mode ? MEM_CONSOLE_ACTION_CANCEL_TITLE_EDIT
                                                          : MEM_CONSOLE_ACTION_CANCEL_BODY_EDIT };

    for (i = 0; i < 10; ++i) {
        float max_w = 112.0f;
        float min_w = 28.0f;
        if (i == 4 || i == 5) {
            min_w = 20.0f;
            max_w = 32.0f;
        } else if (i >= 1 && i <= 3) {
            min_w = 40.0f;
            max_w = 96.0f;
        }
        action_widths[i] = measure_compact_button_width(render_ctx,
                                                        actions[i].label,
                                                        CORE_FONT_ROLE_UI_MEDIUM,
                                                        CORE_FONT_TEXT_SIZE_CAPTION,
                                                        6.0f,
                                                        min_w,
                                                        max_w);
        action_min_widths[i] = min_w;
    }

    fit_button_widths_to_row(action_row.width,
                             action_widths,
                             action_min_widths,
                             10,
                             layout_cfg->action_button_gap,
                             16.0f,
                             &action_gap);
    layout_centered_button_row(action_row,
                               action_widths,
                               10,
                               action_gap,
                               action_btn_rects);

    for (i = 0; i < 10; ++i) {
        KitUiWidgetState draw_state;
        button_result = kit_ui_eval_button(action_btn_rects[i], input, actions[i].enabled);
        draw_state = button_result.state;
        if (actions[i].selected && draw_state == KIT_UI_STATE_NORMAL) {
            draw_state = KIT_UI_STATE_ACTIVE;
        }
        if (button_result.clicked && actions[i].enabled) {
            *io_action = actions[i].action;
        }
        result = mem_console_ui_draw_button_custom(ui_ctx,
                                                   frame,
                                                   action_btn_rects[i],
                                                   actions[i].label,
                                                   draw_state,
                                                   CORE_FONT_ROLE_UI_MEDIUM,
                                                   CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    return core_result_ok();
}
