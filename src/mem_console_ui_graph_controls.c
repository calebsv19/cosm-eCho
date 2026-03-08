#include "mem_console_ui_graph_controls.h"

#include "mem_console_ui_common.h"

#include <stdio.h>
#include <string.h>

static const char *k_graph_kind_filter_labels[] = {
    "ALL",
    "SUPPORTS",
    "DEPENDS",
    "REFS",
    "SUMMARY",
    "RELATED"
};

static const char *k_graph_kind_filter_values[] = {
    "",
    "supports",
    "depends_on",
    "references",
    "summarizes",
    "related"
};

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

static void compute_graph_settings_metrics(float row_width,
                                           float *out_gap,
                                           float *out_edge_label_w,
                                           float *out_edge_input_w,
                                           float *out_edge_apply_w,
                                           float *out_hops_label_w,
                                           float *out_hops_w) {
    float gap = 8.0f;
    float edge_label_w = 86.0f;
    float edge_input_w = 68.0f;
    float edge_apply_w = 62.0f;
    float hops_label_w = 40.0f;
    float hops_w = 140.0f;
    float required = edge_label_w + edge_input_w + edge_apply_w + hops_label_w + hops_w + (gap * 4.0f);

    if (row_width < required) {
        float scalable_total = edge_label_w + edge_input_w + edge_apply_w + hops_label_w;
        float target_scalable = row_width - hops_w - (gap * 4.0f);
        float scale = 1.0f;

        if (target_scalable < scalable_total) {
            if (scalable_total > 0.0f) {
                scale = target_scalable / scalable_total;
            }
            if (scale < 0.56f) {
                scale = 0.56f;
            }
            if (scale > 1.0f) {
                scale = 1.0f;
            }
            edge_label_w *= scale;
            edge_input_w *= scale;
            edge_apply_w *= scale;
            hops_label_w *= scale;
        }
    }

    hops_w = row_width - (edge_label_w + edge_input_w + edge_apply_w + hops_label_w + (gap * 4.0f));
    if (hops_w < 88.0f) {
        float deficit = 88.0f - hops_w;
        edge_label_w = shrink_with_floor(edge_label_w, &deficit, 48.0f);
        edge_input_w = shrink_with_floor(edge_input_w, &deficit, 44.0f);
        edge_apply_w = shrink_with_floor(edge_apply_w, &deficit, 44.0f);
        hops_label_w = shrink_with_floor(hops_label_w, &deficit, 28.0f);
        hops_w = row_width - (edge_label_w + edge_input_w + edge_apply_w + hops_label_w + (gap * 4.0f));
    }
    if (hops_w < 64.0f) {
        hops_w = 64.0f;
    }

    if (out_gap) *out_gap = gap;
    if (out_edge_label_w) *out_edge_label_w = edge_label_w;
    if (out_edge_input_w) *out_edge_input_w = edge_input_w;
    if (out_edge_apply_w) *out_edge_apply_w = edge_apply_w;
    if (out_hops_label_w) *out_hops_label_w = hops_label_w;
    if (out_hops_w) *out_hops_w = hops_w;
}

static int graph_kind_filter_index(const MemConsoleState *state) {
    int i;

    if (!state) {
        return 0;
    }
    for (i = 0; i < (int)(sizeof(k_graph_kind_filter_values) / sizeof(k_graph_kind_filter_values[0])); ++i) {
        if (strcmp(state->graph_kind_filter, k_graph_kind_filter_values[i]) == 0) {
            return i;
        }
    }
    return 0;
}

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
    KitRenderRect graph_filter_bar;
    KitRenderRect graph_settings_row;
    KitRenderRect graph_edge_label_rect;
    KitRenderRect graph_edge_input_rect;
    KitRenderRect graph_edge_apply_rect;
    KitRenderRect graph_hops_label_rect;
    KitRenderRect graph_hops_rect;
    KitRenderRect graph_hint_row;
    KitRenderRect action_bar;
    KitRenderRect action_row_memory;
    KitRenderRect action_row_graph;
    KitRenderRect action_button_rect;
    KitUiButtonResult button_result;
    CoreResult result;
    int action_index;
    int graph_kind_index;
    int graph_hovered_index;
    int graph_hops_index;
    int graph_hops_hovered_index;
    int graph_edge_input_active;
    float controls_gap;
    float edge_label_w;
    float edge_input_w;
    float edge_apply_w;
    float hops_label_w;
    float hops_w;

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
        result = kit_ui_stack_next(right_layout, layout_cfg->graph_filter_h, 0.0f, &graph_filter_bar);
        if (result.code != CORE_OK) {
            return result;
        }
        graph_kind_index = graph_kind_filter_index(state);
        graph_hovered_index = -1;
        if (kit_ui_point_in_rect(graph_filter_bar, input->mouse_x, input->mouse_y)) {
            float seg_w = graph_filter_bar.width / (float)(sizeof(k_graph_kind_filter_labels) / sizeof(k_graph_kind_filter_labels[0]));
            if (seg_w > 0.0f) {
                graph_hovered_index = (int)((input->mouse_x - graph_filter_bar.x) / seg_w);
                if (graph_hovered_index < 0) {
                    graph_hovered_index = 0;
                }
                if (graph_hovered_index >= (int)(sizeof(k_graph_kind_filter_labels) / sizeof(k_graph_kind_filter_labels[0]))) {
                    graph_hovered_index = (int)(sizeof(k_graph_kind_filter_labels) / sizeof(k_graph_kind_filter_labels[0])) - 1;
                }
            }
        }
        {
            KitUiSegmentedResult segmented_result = kit_ui_eval_segmented(graph_filter_bar,
                                                                           input,
                                                                           1,
                                                                           (int)(sizeof(k_graph_kind_filter_labels) / sizeof(k_graph_kind_filter_labels[0])),
                                                                           graph_kind_index);
            result = kit_ui_draw_segmented(ui_ctx,
                                           frame,
                                           graph_filter_bar,
                                           k_graph_kind_filter_labels,
                                           (int)(sizeof(k_graph_kind_filter_labels) / sizeof(k_graph_kind_filter_labels[0])),
                                           graph_kind_index,
                                           graph_hovered_index,
                                           1);
            if (result.code != CORE_OK) {
                return result;
            }
            if (segmented_result.changed) {
                int selected = segmented_result.selected_index;
                if (selected < 0) {
                    selected = 0;
                }
                if (selected >= (int)(sizeof(k_graph_kind_filter_values) / sizeof(k_graph_kind_filter_values[0]))) {
                    selected = (int)(sizeof(k_graph_kind_filter_values) / sizeof(k_graph_kind_filter_values[0])) - 1;
                }
                (void)snprintf(state->graph_kind_filter,
                               sizeof(state->graph_kind_filter),
                               "%s",
                               k_graph_kind_filter_values[selected]);
                if (*io_action == MEM_CONSOLE_ACTION_NONE) {
                    *io_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
                }
            }
        }

        result = kit_ui_stack_next(right_layout, layout_cfg->graph_settings_h, 0.0f, &graph_settings_row);
        if (result.code != CORE_OK) {
            return result;
        }
        compute_graph_settings_metrics(graph_settings_row.width,
                                       &controls_gap,
                                       &edge_label_w,
                                       &edge_input_w,
                                       &edge_apply_w,
                                       &hops_label_w,
                                       &hops_w);
        graph_edge_label_rect = (KitRenderRect){
            graph_settings_row.x,
            graph_settings_row.y + 2.0f,
            edge_label_w,
            graph_settings_row.height - 4.0f
        };
        graph_edge_input_rect = (KitRenderRect){
            graph_edge_label_rect.x + graph_edge_label_rect.width + controls_gap,
            graph_settings_row.y + 1.0f,
            edge_input_w,
            graph_settings_row.height - 2.0f
        };
        graph_edge_apply_rect = (KitRenderRect){
            graph_edge_input_rect.x + graph_edge_input_rect.width + controls_gap,
            graph_settings_row.y + 1.0f,
            edge_apply_w,
            graph_settings_row.height - 2.0f
        };
        graph_hops_label_rect = (KitRenderRect){
            graph_edge_apply_rect.x + graph_edge_apply_rect.width + controls_gap,
            graph_settings_row.y + 2.0f,
            hops_label_w,
            graph_settings_row.height - 4.0f
        };
        graph_hops_rect = (KitRenderRect){
            graph_hops_label_rect.x + graph_hops_label_rect.width + controls_gap,
            graph_settings_row.y + 1.0f,
            hops_w,
            graph_settings_row.height - 2.0f
        };
        {
            float row_right = graph_settings_row.x + graph_settings_row.width;
            if (graph_hops_rect.x > row_right - 32.0f) {
                graph_hops_rect.x = row_right - 32.0f;
            }
            if ((graph_hops_rect.x + graph_hops_rect.width) > row_right) {
                graph_hops_rect.width = row_right - graph_hops_rect.x;
            }
            if (graph_hops_rect.width < 32.0f) {
                graph_hops_rect.width = 32.0f;
            }
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
                                                       graph_edge_input_rect.x + 8.0f,
                                                       graph_edge_input_rect.y + 2.0f,
                                                       graph_edge_input_rect.width - 16.0f,
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
            float text_origin_x = graph_edge_input_rect.x + 8.0f + ui_ctx->style.padding;
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
        if (kit_ui_point_in_rect(graph_hops_rect, input->mouse_x, input->mouse_y)) {
            float seg_w = graph_hops_rect.width / (float)(sizeof(k_graph_hops_labels) / sizeof(k_graph_hops_labels[0]));
            if (seg_w > 0.0f) {
                graph_hops_hovered_index = (int)((input->mouse_x - graph_hops_rect.x) / seg_w);
                if (graph_hops_hovered_index < 0) {
                    graph_hops_hovered_index = 0;
                }
                if (graph_hops_hovered_index >= (int)(sizeof(k_graph_hops_labels) / sizeof(k_graph_hops_labels[0]))) {
                    graph_hops_hovered_index = (int)(sizeof(k_graph_hops_labels) / sizeof(k_graph_hops_labels[0])) - 1;
                }
            }
        }
        {
            KitUiSegmentedResult hops_result = kit_ui_eval_segmented(graph_hops_rect,
                                                                      input,
                                                                      1,
                                                                      (int)(sizeof(k_graph_hops_labels) / sizeof(k_graph_hops_labels[0])),
                                                                      graph_hops_index);
            result = kit_ui_draw_segmented(ui_ctx,
                                           frame,
                                           graph_hops_rect,
                                           k_graph_hops_labels,
                                           (int)(sizeof(k_graph_hops_labels) / sizeof(k_graph_hops_labels[0])),
                                           graph_hops_index,
                                           graph_hops_hovered_index,
                                           1);
            if (result.code != CORE_OK) {
                return result;
            }
            if (hops_result.changed) {
                int selected = hops_result.selected_index;
                if (selected < 0) {
                    selected = 0;
                }
                if (selected >= (int)(sizeof(k_graph_hops_labels) / sizeof(k_graph_hops_labels[0]))) {
                    selected = (int)(sizeof(k_graph_hops_labels) / sizeof(k_graph_hops_labels[0])) - 1;
                }
                state->graph_query_hops = mem_console_graph_hops_clamp(MEM_CONSOLE_GRAPH_HOPS_MIN + selected);
                if (*io_action == MEM_CONSOLE_ACTION_NONE) {
                    *io_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
                }
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
        float action_block_h = (layout_cfg->action_button_h * 2.0f) +
                               layout_cfg->action_button_gap +
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

    action_row_memory = (KitRenderRect){
        action_bar.x + layout_cfg->action_block_pad,
        action_bar.y + layout_cfg->action_block_pad,
        action_bar.width - (layout_cfg->action_block_pad * 2.0f),
        layout_cfg->action_button_h
    };
    action_row_graph = (KitRenderRect){
        action_row_memory.x,
        action_row_memory.y + action_row_memory.height + layout_cfg->action_button_gap,
        action_row_memory.width,
        layout_cfg->action_button_h
    };

    for (action_index = 0; action_index < 6; ++action_index) {
        const char *label = "";
        int enabled = 1;
        float row_gap = layout_cfg->action_button_gap;
        float button_width = (action_row_memory.width - (row_gap * 5.0f)) / 6.0f;
        action_button_rect = (KitRenderRect){
            action_row_memory.x + ((button_width + row_gap) * (float)action_index),
            action_row_memory.y,
            button_width,
            action_row_memory.height
        };
        if (action_button_rect.width < 48.0f) {
            action_button_rect.width = 48.0f;
        }

        switch (action_index) {
            case 0:
                label = state->selected_pinned ? "PINNED ON" : "PINNED OFF";
                enabled = state->selected_item_id != 0;
                break;
            case 1:
                label = state->selected_canonical ? "CANON ON" : "CANON OFF";
                enabled = state->selected_item_id != 0;
                break;
            case 2:
                label = "NEW MEMORY";
                break;
            case 3:
                label = state->title_edit_mode ? "SAVE TITLE" : "EDIT TITLE";
                enabled = state->selected_item_id != 0;
                break;
            case 4:
                label = state->body_edit_mode ? "SAVE BODY" : "EDIT BODY";
                enabled = state->selected_item_id != 0;
                break;
            case 5:
                label = "CANCEL EDIT";
                enabled = has_any_edit_mode;
                break;
            default:
                break;
        }

        button_result = kit_ui_eval_button(action_button_rect, input, enabled);
        if (button_result.clicked) {
            switch (action_index) {
                case 0:
                    *io_action = MEM_CONSOLE_ACTION_TOGGLE_PINNED;
                    break;
                case 1:
                    *io_action = MEM_CONSOLE_ACTION_TOGGLE_CANONICAL;
                    break;
                case 2:
                    *io_action = MEM_CONSOLE_ACTION_CREATE_FROM_SEARCH;
                    break;
                case 3:
                    *io_action = state->title_edit_mode
                                     ? MEM_CONSOLE_ACTION_SAVE_TITLE_EDIT
                                     : MEM_CONSOLE_ACTION_BEGIN_TITLE_EDIT;
                    break;
                case 4:
                    *io_action = state->body_edit_mode
                                     ? MEM_CONSOLE_ACTION_SAVE_BODY_EDIT
                                     : MEM_CONSOLE_ACTION_BEGIN_BODY_EDIT;
                    break;
                case 5:
                    if (state->title_edit_mode) {
                        *io_action = MEM_CONSOLE_ACTION_CANCEL_TITLE_EDIT;
                    } else if (state->body_edit_mode) {
                        *io_action = MEM_CONSOLE_ACTION_CANCEL_BODY_EDIT;
                    }
                    break;
                default:
                    break;
            }
        }

        result = mem_console_ui_draw_button_custom(ui_ctx,
                                                   frame,
                                                   action_button_rect,
                                                   label,
                                                   button_result.state,
                                                   CORE_FONT_ROLE_UI_MEDIUM,
                                                   CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    for (action_index = 0; action_index < 4; ++action_index) {
        const char *label = "";
        int enabled = 0;
        float row_gap = layout_cfg->action_button_gap;
        float button_width = (action_row_graph.width - (row_gap * 3.0f)) / 4.0f;
        action_button_rect = (KitRenderRect){
            action_row_graph.x + ((button_width + row_gap) * (float)action_index),
            action_row_graph.y,
            button_width,
            action_row_graph.height
        };
        switch (action_index) {
            case 0:
                label = state->graph_mode_enabled ? "GRAPH: ON" : "GRAPH: OFF";
                enabled = 1;
                break;
            case 1:
                label = "REFRESH GRAPH";
                enabled = state->graph_mode_enabled;
                break;
            case 2:
                label = "CENTER GRAPH";
                enabled = state->graph_mode_enabled;
                break;
            case 3:
                label = "CENTER SEL";
                enabled = state->graph_mode_enabled && state->selected_item_id != 0;
                break;
            default:
                break;
        }

        button_result = kit_ui_eval_button(action_button_rect, input, enabled);
        if (button_result.clicked) {
            switch (action_index) {
                case 0:
                    *io_action = MEM_CONSOLE_ACTION_TOGGLE_GRAPH_MODE;
                    break;
                case 1:
                    *io_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
                    break;
                case 2:
                    *io_action = MEM_CONSOLE_ACTION_CENTER_GRAPH;
                    break;
                case 3:
                    *io_action = MEM_CONSOLE_ACTION_CENTER_SELECTED;
                    break;
                default:
                    break;
            }
        }

        result = mem_console_ui_draw_button_custom(ui_ctx,
                                                   frame,
                                                   action_button_rect,
                                                   label,
                                                   button_result.state,
                                                   CORE_FONT_ROLE_UI_MEDIUM,
                                                   CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    return core_result_ok();
}
