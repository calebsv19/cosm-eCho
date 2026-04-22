#include "mem_console_ui_graph_controls.h"

#include "mem_console_ui_common.h"

#include <stdio.h>
#include <string.h>

static const char *k_graph_hops_labels[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10"
};

typedef struct GraphNodeKindToggleDef {
    const char *label;
    const char *kind;
} GraphNodeKindToggleDef;

static const GraphNodeKindToggleDef k_graph_node_kind_toggles[] = {
    { "Plan", "plan" },
    { "Decision", "decision" },
    { "Issue", "issue" },
    { "Scope", "scope" },
    { "Summary", "summary" },
    { "Policy", "policy" },
    { "Runtime", "runtime" }
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
                                           float *out_labels_toggle_w,
                                           float *out_edge_label_w,
                                           float *out_edge_input_w,
                                           float *out_edge_apply_w,
                                           float *out_hops_label_w,
                                           float out_hop_btn_w[MEM_CONSOLE_GRAPH_HOPS_MAX]) {
    float gap = 6.0f;
    float labels_toggle_w = measure_compact_button_width(render_ctx,
                                                        "LBL",
                                                        CORE_FONT_ROLE_UI_MEDIUM,
                                                        CORE_FONT_TEXT_SIZE_CAPTION,
                                                        6.0f,
                                                        28.0f,
                                                        52.0f);
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
                                               "10",
                                               CORE_FONT_ROLE_UI_REGULAR,
                                               CORE_FONT_TEXT_SIZE_CAPTION,
                                               6.0f,
                                               18.0f,
                                               34.0f);
    float required;
    float deficit = 0.0f;
    int i;

    required = labels_toggle_w + edge_label_w + edge_input_w + edge_apply_w + hops_label_w +
               (hop_w * (float)MEM_CONSOLE_GRAPH_HOPS_MAX) +
               (gap * (float)(MEM_CONSOLE_GRAPH_HOPS_MAX + 4));

    if (row_width < required) {
        deficit = required - row_width;
        labels_toggle_w = shrink_with_floor(labels_toggle_w, &deficit, 24.0f);
        edge_label_w = shrink_with_floor(edge_label_w, &deficit, 48.0f);
        edge_input_w = shrink_with_floor(edge_input_w, &deficit, 44.0f);
        edge_apply_w = shrink_with_floor(edge_apply_w, &deficit, 40.0f);
        hops_label_w = shrink_with_floor(hops_label_w, &deficit, 24.0f);
        hop_w = shrink_with_floor(hop_w, &deficit, 16.0f);
    }

    if (out_gap) *out_gap = gap;
    if (out_labels_toggle_w) *out_labels_toggle_w = labels_toggle_w;
    if (out_edge_label_w) *out_edge_label_w = edge_label_w;
    if (out_edge_input_w) *out_edge_input_w = edge_input_w;
    if (out_edge_apply_w) *out_edge_apply_w = edge_apply_w;
    if (out_hops_label_w) *out_hops_label_w = hops_label_w;
    if (out_hop_btn_w) {
        for (i = 0; i < MEM_CONSOLE_GRAPH_HOPS_MAX; ++i) {
            out_hop_btn_w[i] = hop_w;
        }
    }
}

static void place_graph_setting_rects(const KitRenderRect *row,
                                      float gap,
                                      float labels_toggle_w,
                                      float edge_label_w,
                                      float edge_input_w,
                                      float edge_apply_w,
                                      float hops_label_w,
                                      const float hop_btn_w[MEM_CONSOLE_GRAPH_HOPS_MAX],
                                      KitRenderRect *out_labels_toggle_rect,
                                      KitRenderRect *out_edge_label_rect,
                                      KitRenderRect *out_edge_input_rect,
                                      KitRenderRect *out_edge_apply_rect,
                                      KitRenderRect *out_hops_label_rect,
                                      KitRenderRect out_hop_btn_rects[MEM_CONSOLE_GRAPH_HOPS_MAX]) {
    float total_w;
    float x;
    float y;
    float h;
    int i;

    if (!row || !out_labels_toggle_rect || !out_edge_label_rect || !out_edge_input_rect || !out_edge_apply_rect ||
        !out_hops_label_rect || !out_hop_btn_rects || !hop_btn_w) {
        return;
    }

    total_w = labels_toggle_w + edge_label_w + edge_input_w + edge_apply_w + hops_label_w +
              (gap * (float)(MEM_CONSOLE_GRAPH_HOPS_MAX + 4));
    for (i = 0; i < MEM_CONSOLE_GRAPH_HOPS_MAX; ++i) {
        total_w += hop_btn_w[i];
    }

    x = row->x + ((row->width - total_w) * 0.5f);
    y = row->y + 1.0f;
    h = row->height - 2.0f;
    if (h < 12.0f) {
        h = 12.0f;
    }

    *out_labels_toggle_rect = (KitRenderRect){ x, y, labels_toggle_w, h };
    x += labels_toggle_w + gap;
    *out_edge_label_rect = (KitRenderRect){ x, y + 1.0f, edge_label_w, h - 2.0f };
    x += edge_label_w + gap;
    *out_edge_input_rect = (KitRenderRect){ x, y, edge_input_w, h };
    x += edge_input_w + gap;
    *out_edge_apply_rect = (KitRenderRect){ x, y, edge_apply_w, h };
    x += edge_apply_w + gap;
    *out_hops_label_rect = (KitRenderRect){ x, y + 1.0f, hops_label_w, h - 2.0f };
    x += hops_label_w + gap;

    for (i = 0; i < MEM_CONSOLE_GRAPH_HOPS_MAX; ++i) {
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
    KitRenderRect graph_labels_toggle_rect;
    KitRenderRect graph_edge_label_rect;
    KitRenderRect graph_edge_input_rect;
    KitRenderRect graph_edge_apply_rect;
    KitRenderRect graph_hops_label_rect;
    KitRenderRect graph_hop_btn_rects[MEM_CONSOLE_GRAPH_HOPS_MAX];
    KitRenderRect graph_role_filter_row;
    KitRenderRect graph_role_btn_rects[16];
    KitRenderRect graph_hint_row;
    KitRenderRect action_bar;
    KitRenderRect action_row;
    KitRenderRect action_btn_rects[11];
    KitUiButtonResult button_result;
    CoreResult result;
    int graph_hops_index;
    int graph_hops_hovered_index;
    int graph_edge_input_active;
    float controls_gap;
    float labels_toggle_w;
    float edge_label_w;
    float edge_input_w;
    float edge_apply_w;
    float hops_label_w;
    float hop_btn_w[MEM_CONSOLE_GRAPH_HOPS_MAX];
    float graph_role_widths[16];
    float graph_role_min_widths[16];
    float graph_role_gap = 0.0f;
    float graph_section_gap = 10.0f;
    const int graph_role_button_count = (int)(sizeof(k_graph_node_kind_toggles) /
                                              sizeof(k_graph_node_kind_toggles[0])) + 1;
    const int graph_mode_sort_button_count = 6;
    const int graph_total_controls = graph_role_button_count + graph_mode_sort_button_count;
    int action_button_count = 0;
    float action_gap = 0.0f;
    float action_widths[11];
    float action_min_widths[11];
    int i;
    GraphActionDef actions[11];

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
                                       &labels_toggle_w,
                                       &edge_label_w,
                                       &edge_input_w,
                                       &edge_apply_w,
                                       &hops_label_w,
                                       hop_btn_w);
        place_graph_setting_rects(&graph_settings_row,
                                  controls_gap,
                                  labels_toggle_w,
                                  edge_label_w,
                                  edge_input_w,
                                  edge_apply_w,
                                  hops_label_w,
                                  hop_btn_w,
                                  &graph_labels_toggle_rect,
                                  &graph_edge_label_rect,
                                  &graph_edge_input_rect,
                                  &graph_edge_apply_rect,
                                  &graph_hops_label_rect,
                                  graph_hop_btn_rects);

        button_result = kit_ui_eval_button(graph_labels_toggle_rect, input, 1);
        if (button_result.clicked) {
            state->graph_edge_labels_enabled = state->graph_edge_labels_enabled ? 0 : 1;
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        }
        if (state->graph_edge_labels_enabled && button_result.state == KIT_UI_STATE_NORMAL) {
            button_result.state = KIT_UI_STATE_ACTIVE;
        }
        result = mem_console_ui_draw_button_custom(ui_ctx,
                                                   frame,
                                                   graph_labels_toggle_rect,
                                                   "LBL",
                                                   button_result.state,
                                                   CORE_FONT_ROLE_UI_MEDIUM,
                                                   CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }

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
                                                   state->graph_edge_limit_text[0] ? state->graph_edge_limit_text : "128",
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
        for (i = 0; i < MEM_CONSOLE_GRAPH_HOPS_MAX; ++i) {
            if (kit_ui_point_in_rect(graph_hop_btn_rects[i], input->mouse_x, input->mouse_y)) {
                graph_hops_hovered_index = i;
                break;
            }
        }
        for (i = 0; i < MEM_CONSOLE_GRAPH_HOPS_MAX; ++i) {
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

        result = kit_ui_stack_next(right_layout, layout_cfg->graph_settings_h, 0.0f, &graph_role_filter_row);
        if (result.code != CORE_OK) {
            return result;
        }
        graph_role_widths[0] = measure_compact_button_width(render_ctx,
                                                            "All",
                                                            CORE_FONT_ROLE_UI_MONO_SMALL,
                                                            CORE_FONT_TEXT_SIZE_CAPTION,
                                                            6.0f,
                                                            24.0f,
                                                            180.0f);
        graph_role_min_widths[0] = graph_role_widths[0];
        for (i = 0; i < (graph_role_button_count - 1); ++i) {
            graph_role_widths[i + 1] = measure_compact_button_width(render_ctx,
                                                                    k_graph_node_kind_toggles[i].label,
                                                                    CORE_FONT_ROLE_UI_MONO_SMALL,
                                                                    CORE_FONT_TEXT_SIZE_CAPTION,
                                                                    6.0f,
                                                                    24.0f,
                                                                    180.0f);
            graph_role_min_widths[i + 1] = graph_role_widths[i + 1];
        }
        graph_role_widths[graph_role_button_count + 0] = measure_compact_button_width(render_ctx,
                                                                                       "DAG",
                                                                                       CORE_FONT_ROLE_UI_MEDIUM,
                                                                                       CORE_FONT_TEXT_SIZE_CAPTION,
                                                                                       6.0f,
                                                                                       26.0f,
                                                                                       52.0f);
        graph_role_widths[graph_role_button_count + 1] = measure_compact_button_width(render_ctx,
                                                                                       "TREE",
                                                                                       CORE_FONT_ROLE_UI_MEDIUM,
                                                                                       CORE_FONT_TEXT_SIZE_CAPTION,
                                                                                       6.0f,
                                                                                       30.0f,
                                                                                       56.0f);
        graph_role_widths[graph_role_button_count + 2] = measure_compact_button_width(render_ctx,
                                                                                       "NEW",
                                                                                       CORE_FONT_ROLE_UI_MEDIUM,
                                                                                       CORE_FONT_TEXT_SIZE_CAPTION,
                                                                                       6.0f,
                                                                                       30.0f,
                                                                                       56.0f);
        graph_role_widths[graph_role_button_count + 3] = measure_compact_button_width(render_ctx,
                                                                                       "OLD",
                                                                                       CORE_FONT_ROLE_UI_MEDIUM,
                                                                                       CORE_FONT_TEXT_SIZE_CAPTION,
                                                                                       6.0f,
                                                                                       30.0f,
                                                                                       56.0f);
        graph_role_widths[graph_role_button_count + 4] = measure_compact_button_width(render_ctx,
                                                                                       "FULL",
                                                                                       CORE_FONT_ROLE_UI_MEDIUM,
                                                                                       CORE_FONT_TEXT_SIZE_CAPTION,
                                                                                       6.0f,
                                                                                       32.0f,
                                                                                       60.0f);
        graph_role_widths[graph_role_button_count + 5] = measure_compact_button_width(render_ctx,
                                                                                       "FNL",
                                                                                       CORE_FONT_ROLE_UI_MEDIUM,
                                                                                       CORE_FONT_TEXT_SIZE_CAPTION,
                                                                                       6.0f,
                                                                                       30.0f,
                                                                                       56.0f);
        graph_role_min_widths[graph_role_button_count + 0] = graph_role_widths[graph_role_button_count + 0];
        graph_role_min_widths[graph_role_button_count + 1] = graph_role_widths[graph_role_button_count + 1];
        graph_role_min_widths[graph_role_button_count + 2] = graph_role_widths[graph_role_button_count + 2];
        graph_role_min_widths[graph_role_button_count + 3] = graph_role_widths[graph_role_button_count + 3];
        graph_role_min_widths[graph_role_button_count + 4] = graph_role_widths[graph_role_button_count + 4];
        graph_role_min_widths[graph_role_button_count + 5] = graph_role_widths[graph_role_button_count + 5];

        fit_button_widths_to_row(graph_role_filter_row.width - graph_section_gap,
                                 graph_role_widths,
                                 graph_role_min_widths,
                                 graph_total_controls,
                                 2.0f,
                                 8.0f,
                                 &graph_role_gap);
        {
            float total_w = 0.0f;
            float x = 0.0f;
            float section_gap = graph_section_gap;
            int left_gap_count = graph_role_button_count > 0 ? graph_role_button_count - 1 : 0;
            int right_gap_count = graph_mode_sort_button_count > 0 ? graph_mode_sort_button_count - 1 : 0;

            for (i = 0; i < graph_total_controls; ++i) {
                total_w += graph_role_widths[i];
            }
            total_w += graph_role_gap * (float)(left_gap_count + right_gap_count);
            total_w += section_gap;
            if (total_w > graph_role_filter_row.width) {
                float overflow = total_w - graph_role_filter_row.width;
                if (section_gap > 2.0f) {
                    float reducible = section_gap - 2.0f;
                    float reduction = overflow < reducible ? overflow : reducible;
                    section_gap -= reduction;
                    total_w -= reduction;
                }
            }

            if (total_w < graph_role_filter_row.width) {
                x = graph_role_filter_row.x + ((graph_role_filter_row.width - total_w) * 0.5f);
            } else {
                x = graph_role_filter_row.x;
            }

            for (i = 0; i < graph_role_button_count; ++i) {
                graph_role_btn_rects[i] =
                    (KitRenderRect){ x, graph_role_filter_row.y, graph_role_widths[i], graph_role_filter_row.height };
                x += graph_role_widths[i];
                if (i + 1 < graph_role_button_count) {
                    x += graph_role_gap;
                }
            }
            x += section_gap;
            for (i = 0; i < graph_mode_sort_button_count; ++i) {
                int idx = graph_role_button_count + i;
                graph_role_btn_rects[idx] =
                    (KitRenderRect){ x, graph_role_filter_row.y, graph_role_widths[idx], graph_role_filter_row.height };
                x += graph_role_widths[idx];
                if (i + 1 < graph_mode_sort_button_count) {
                    x += graph_role_gap;
                }
            }
        }

        for (i = 0; i < graph_total_controls; ++i) {
            const char *label = "";
            int selected = 0;
            CoreFontRoleId font_role = CORE_FONT_ROLE_UI_MONO_SMALL;
            CoreFontTextSizeTier text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
            button_result = kit_ui_eval_button(graph_role_btn_rects[i], input, 1);

            if (i < graph_role_button_count) {
                if (i == 0) {
                    label = "All";
                    font_role = CORE_FONT_ROLE_UI_MONO_SMALL;
                    selected = state->graph_node_kind_filter_all_override ? 1 : 0;
                    if (button_result.clicked) {
                        if (mem_console_graph_node_kind_toggle_all_override(state) &&
                            *io_action == MEM_CONSOLE_ACTION_NONE) {
                            *io_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
                        }
                    }
                } else {
                    const GraphNodeKindToggleDef *toggle = &k_graph_node_kind_toggles[i - 1];
                    label = toggle->label;
                    selected = mem_console_graph_node_kind_is_enabled(state, toggle->kind);
                    if (button_result.clicked) {
                        if (mem_console_graph_node_kind_toggle_enabled(state, toggle->kind) &&
                            *io_action == MEM_CONSOLE_ACTION_NONE) {
                            *io_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
                        }
                    }
                }
            } else {
                int mode_index = i - graph_role_button_count;
                int changed = 0;
                font_role = CORE_FONT_ROLE_UI_MEDIUM;
                text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
                if (mode_index == 0) {
                    label = "DAG";
                    selected = state->graph_layout_mode == MEM_CONSOLE_GRAPH_LAYOUT_DAG ? 1 : 0;
                } else if (mode_index == 1) {
                    label = "TREE";
                    selected = state->graph_layout_mode == MEM_CONSOLE_GRAPH_LAYOUT_TREE ? 1 : 0;
                } else if (mode_index == 2) {
                    label = "NEW";
                    selected = state->graph_sort_mode == MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST ? 1 : 0;
                } else if (mode_index == 3) {
                    label = "OLD";
                    selected = state->graph_sort_mode == MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST ? 1 : 0;
                } else if (mode_index == 4) {
                    label = "FULL";
                    selected = state->graph_scope_full_mode_enabled ? 1 : 0;
                } else {
                    label = "FNL";
                    selected = state->graph_anchor_funnel_enabled ? 1 : 0;
                }
                if (button_result.clicked) {
                    if (mode_index == 0) {
                        int next_mode = mem_console_graph_layout_mode_clamp(MEM_CONSOLE_GRAPH_LAYOUT_DAG);
                        changed = next_mode != state->graph_layout_mode ? 1 : 0;
                        state->graph_layout_mode = next_mode;
                    } else if (mode_index == 1) {
                        int next_mode = mem_console_graph_layout_mode_clamp(MEM_CONSOLE_GRAPH_LAYOUT_TREE);
                        changed = next_mode != state->graph_layout_mode ? 1 : 0;
                        state->graph_layout_mode = next_mode;
                    } else if (mode_index == 2) {
                        int next_sort = mem_console_graph_sort_mode_clamp(MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST);
                        changed = next_sort != state->graph_sort_mode ? 1 : 0;
                        state->graph_sort_mode = next_sort;
                    } else if (mode_index == 3) {
                        int next_sort = mem_console_graph_sort_mode_clamp(MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST);
                        changed = next_sort != state->graph_sort_mode ? 1 : 0;
                        state->graph_sort_mode = next_sort;
                    } else if (mode_index == 4) {
                        int next_value = state->graph_scope_full_mode_enabled ? 0 : 1;
                        changed = next_value != state->graph_scope_full_mode_enabled ? 1 : 0;
                        state->graph_scope_full_mode_enabled = next_value;
                    } else {
                        int next_value = state->graph_anchor_funnel_enabled ? 0 : 1;
                        changed = next_value != state->graph_anchor_funnel_enabled ? 1 : 0;
                        state->graph_anchor_funnel_enabled = next_value;
                    }
                    if (changed && *io_action == MEM_CONSOLE_ACTION_NONE) {
                        *io_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
                    }
                }
            }

            if (selected && button_result.state == KIT_UI_STATE_NORMAL) {
                button_result.state = KIT_UI_STATE_ACTIVE;
            }
            result = mem_console_ui_draw_button_custom(ui_ctx,
                                                       frame,
                                                       graph_role_btn_rects[i],
                                                       label,
                                                       button_result.state,
                                                       font_role,
                                                       text_tier);
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
    actions[action_button_count++] = (GraphActionDef){ state->graph_scope_full_mode_enabled ? "FULL VIEW" : "FOCUS VIEW",
                                                        1,
                                                        state->graph_scope_full_mode_enabled ? 1 : 0,
                                                        MEM_CONSOLE_ACTION_TOGGLE_GRAPH_MODE };
    actions[action_button_count++] = (GraphActionDef){ "REFRESH", 1, 0, MEM_CONSOLE_ACTION_REFRESH_GRAPH };
    actions[action_button_count++] = (GraphActionDef){ "CENTER", 1, 0, MEM_CONSOLE_ACTION_CENTER_GRAPH };
    actions[action_button_count++] = (GraphActionDef){ "CENTER SEL",
                                                        state->selected_item_id != 0,
                                                        0,
                                                        MEM_CONSOLE_ACTION_CENTER_SELECTED };
    if (state->detail_reference_path_available) {
        actions[action_button_count++] = (GraphActionDef){ "OPEN REF", 1, 0, MEM_CONSOLE_ACTION_OPEN_REFERENCE_PATH };
    }
    actions[action_button_count++] = (GraphActionDef){ "P",
                                                        state->selected_item_id != 0,
                                                        state->selected_pinned ? 1 : 0,
                                                        MEM_CONSOLE_ACTION_TOGGLE_PINNED };
    actions[action_button_count++] = (GraphActionDef){ "C",
                                                        state->selected_item_id != 0,
                                                        state->selected_canonical ? 1 : 0,
                                                        MEM_CONSOLE_ACTION_TOGGLE_CANONICAL };
    actions[action_button_count++] = (GraphActionDef){ "NEW", 1, 0, MEM_CONSOLE_ACTION_CREATE_FROM_SEARCH };
    actions[action_button_count++] = (GraphActionDef){ state->title_edit_mode ? "SAVE TITLE" : "EDIT TITLE",
                                                        state->selected_item_id != 0,
                                                        state->title_edit_mode ? 1 : 0,
                                                        state->title_edit_mode ? MEM_CONSOLE_ACTION_SAVE_TITLE_EDIT
                                                                               : MEM_CONSOLE_ACTION_BEGIN_TITLE_EDIT };
    actions[action_button_count++] = (GraphActionDef){ state->body_edit_mode ? "SAVE BODY" : "EDIT BODY",
                                                        state->selected_item_id != 0,
                                                        state->body_edit_mode ? 1 : 0,
                                                        state->body_edit_mode ? MEM_CONSOLE_ACTION_SAVE_BODY_EDIT
                                                                              : MEM_CONSOLE_ACTION_BEGIN_BODY_EDIT };
    actions[action_button_count++] = (GraphActionDef){ "CANCEL",
                                                        has_any_edit_mode,
                                                        0,
                                                        state->title_edit_mode ? MEM_CONSOLE_ACTION_CANCEL_TITLE_EDIT
                                                                               : MEM_CONSOLE_ACTION_CANCEL_BODY_EDIT };

    for (i = 0; i < action_button_count; ++i) {
        float max_w = 112.0f;
        float min_w = 28.0f;
        if (strcmp(actions[i].label, "P") == 0 || strcmp(actions[i].label, "C") == 0) {
            min_w = 20.0f;
            max_w = 32.0f;
        } else if (strcmp(actions[i].label, "REFRESH") == 0 ||
                   strcmp(actions[i].label, "CENTER") == 0 ||
                   strcmp(actions[i].label, "CENTER SEL") == 0 ||
                   strcmp(actions[i].label, "OPEN REF") == 0) {
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
                             action_button_count,
                             layout_cfg->action_button_gap,
                             16.0f,
                             &action_gap);
    layout_centered_button_row(action_row,
                               action_widths,
                               action_button_count,
                               action_gap,
                               action_btn_rects);

    for (i = 0; i < action_button_count; ++i) {
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
