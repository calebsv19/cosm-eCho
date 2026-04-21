#include "mem_console_ui_graph_internal.h"

#include <stdio.h>
#include <string.h>

static KitRenderColor graph_darkened_surface(KitRenderColor base, float factor, uint8_t alpha) {
    if (factor < 0.0f) {
        factor = 0.0f;
    }
    if (factor > 1.0f) {
        factor = 1.0f;
    }
    base.r = (uint8_t)((float)base.r * factor);
    base.g = (uint8_t)((float)base.g * factor);
    base.b = (uint8_t)((float)base.b * factor);
    base.a = alpha;
    return base;
}

CoreResult draw_rect_outline(KitRenderFrame *frame,
                             KitRenderRect rect,
                             float thickness,
                             KitRenderColor color) {
    KitRenderRectCommand rect_cmd;
    CoreResult result;
    float inset;
    float horizontal_w;
    float vertical_h;

    if (!frame || thickness <= 0.0f || rect.width <= 0.0f || rect.height <= 0.0f) {
        return core_result_ok();
    }

    inset = thickness;
    if (inset > rect.width * 0.5f) {
        inset = rect.width * 0.5f;
    }
    if (inset > rect.height * 0.5f) {
        inset = rect.height * 0.5f;
    }
    if (inset <= 0.0f) {
        return core_result_ok();
    }

    rect_cmd.corner_radius = 0.0f;
    rect_cmd.color = color;
    rect_cmd.transform = kit_render_identity_transform();

    rect_cmd.rect = (KitRenderRect){ rect.x, rect.y, rect.width, inset };
    result = kit_render_push_rect(frame, &rect_cmd);
    if (result.code != CORE_OK) return result;

    rect_cmd.rect = (KitRenderRect){ rect.x, rect.y + rect.height - inset, rect.width, inset };
    result = kit_render_push_rect(frame, &rect_cmd);
    if (result.code != CORE_OK) return result;

    horizontal_w = rect.width - (inset * 2.0f);
    vertical_h = rect.height - (inset * 2.0f);
    if (horizontal_w <= 0.0f || vertical_h <= 0.0f) {
        return core_result_ok();
    }

    rect_cmd.rect = (KitRenderRect){ rect.x, rect.y + inset, inset, vertical_h };
    result = kit_render_push_rect(frame, &rect_cmd);
    if (result.code != CORE_OK) return result;

    rect_cmd.rect = (KitRenderRect){ rect.x + rect.width - inset, rect.y + inset, inset, vertical_h };
    return kit_render_push_rect(frame, &rect_cmd);
}

CoreResult draw_project_pod_overlays(const KitRenderContext *render_ctx,
                                     KitRenderFrame *frame,
                                     MemConsoleState *state,
                                     KitRenderRect bounds) {
    GraphProjectPod pods[MEM_CONSOLE_GRAPH_PROJECT_POD_LIMIT];
    int pod_count = 0;
    int p;

    if (!render_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid pod overlay draw request" };
    }
    if (!state->graph_scope_full_mode_enabled || state->graph_layout_node_count == 0u) {
        return core_result_ok();
    }

    memset(pods, 0, sizeof(pods));
    pod_count = graph_collect_project_pods(state,
                                           state->graph_layout_node_layouts,
                                           state->graph_layout_node_count,
                                           pods,
                                           MEM_CONSOLE_GRAPH_PROJECT_POD_LIMIT);
    if (pod_count <= 0) {
        return core_result_ok();
    }

    for (p = 0; p < pod_count; ++p) {
        KitRenderRect pod_rect = pods[p].bounds;
        KitRenderColor border = mem_console_ui_project_color_for_key(pods[p].key);
        KitRenderColor fill = border;
        KitRenderColor guide = border;
        KitRenderRectCommand rect_cmd;
        CoreResult result;

        if (pods[p].node_count <= 0) {
            continue;
        }

        pod_rect.x -= 10.0f;
        pod_rect.y -= 14.0f;
        pod_rect.width += 20.0f;
        pod_rect.height += 20.0f;
        if (pod_rect.width <= 0.0f || pod_rect.height <= 0.0f) {
            continue;
        }
        if (pod_rect.x + pod_rect.width < bounds.x - 40.0f ||
            pod_rect.y + pod_rect.height < bounds.y - 40.0f ||
            pod_rect.x > bounds.x + bounds.width + 40.0f ||
            pod_rect.y > bounds.y + bounds.height + 40.0f) {
            continue;
        }
        fill.a = state->graph_viewport.zoom <= 0.70f ? 16u : 10u;
        border.a = 84u;
        rect_cmd.rect = pod_rect;
        rect_cmd.corner_radius = 10.0f;
        rect_cmd.color = fill;
        rect_cmd.transform = kit_render_identity_transform();
        result = kit_render_push_rect(frame, &rect_cmd);
        if (result.code != CORE_OK) {
            return result;
        }
        result = draw_rect_outline(frame, pod_rect, 1.2f, border);
        if (result.code != CORE_OK) {
            return result;
        }

        if (pod_rect.width > 80.0f && pod_rect.height > 40.0f) {
            KitRenderLineCommand line_cmd;
            float lane_gap = 4.0f;
            float pad_x = 8.0f;
            float lane_w = (pod_rect.width - (pad_x * 2.0f) - (lane_gap * 3.0f)) / 4.0f;
            int lane;
            guide.a = 34u;
            for (lane = 1; lane <= 3; ++lane) {
                float x = pod_rect.x + pad_x + ((lane_w + lane_gap) * (float)lane) - (lane_gap * 0.5f);
                line_cmd.p0 = (KitRenderVec2){ x, pod_rect.y + 18.0f };
                line_cmd.p1 = (KitRenderVec2){ x, pod_rect.y + pod_rect.height - 5.0f };
                line_cmd.thickness = 1.0f;
                line_cmd.color = guide;
                line_cmd.transform = kit_render_identity_transform();
                result = kit_render_push_line(frame, &line_cmd);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }

        if (pod_rect.width > 34.0f && pod_rect.height > 16.0f) {
            KitRenderTextCommand text_cmd = {0};
            graph_format_project_display_name(pods[p].key,
                                              state->graph_draw_pod_labels[p],
                                              sizeof(state->graph_draw_pod_labels[p]));
            text_cmd.origin.x = pod_rect.x + 7.0f;
            text_cmd.origin.y = pod_rect.y + 9.0f;
            text_cmd.text = state->graph_draw_pod_labels[p];
            text_cmd.font_role = CORE_FONT_ROLE_UI_MEDIUM;
            text_cmd.text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
            text_cmd.color_token = CORE_THEME_COLOR_TEXT_MUTED;
            text_cmd.transform = kit_render_identity_transform();
            result = kit_render_push_text(frame, &text_cmd);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    (void)render_ctx;
    return core_result_ok();
}

CoreResult draw_graph_edge_legend(KitUiContext *ui_ctx,
                                  const KitRenderContext *render_ctx,
                                  const KitUiInputState *input,
                                  KitRenderFrame *frame,
                                  KitRenderRect bounds,
                                  MemConsoleState *state,
                                  int *out_click_consumed,
                                  int *out_filter_changed) {
    const GraphEdgeLegendEntry *rows[9];
    int row_count = 1;
    int i;
    float row_h = 13.0f;
    float title_h = 12.0f;
    float pad = 4.0f;
    float legend_w = 144.0f;
    float legend_h;
    KitRenderRect legend_outer;
    KitRenderRect legend_inner;
    KitRenderColor legend_outer_color;
    KitRenderColor legend_inner_color;
    CoreResult result;
    int hovered_row = -1;
    int clicked_row = -1;

    if (!ui_ctx || !render_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph legend draw request" };
    }
    if (out_click_consumed) {
        *out_click_consumed = 0;
    }
    if (out_filter_changed) {
        *out_filter_changed = 0;
    }

    rows[0] = 0; /* Row 0 is ALL. */

    for (i = 0; i < (int)graph_edge_legend_entry_count(); ++i) {
        if (row_count < (int)(sizeof(rows) / sizeof(rows[0]))) {
            rows[row_count++] = graph_edge_legend_entry_at((uint32_t)i);
        }
    }

    if (row_count <= 0) {
        return core_result_ok();
    }

    legend_h = pad + title_h + 2.0f + ((float)row_count * row_h) + pad;
    legend_outer = (KitRenderRect){
        bounds.x + bounds.width - legend_w - 8.0f,
        bounds.y + 8.0f,
        legend_w,
        legend_h
    };
    legend_inner = (KitRenderRect){
        legend_outer.x + 2.0f,
        legend_outer.y + 2.0f,
        legend_outer.width - 4.0f,
        legend_outer.height - 4.0f
    };

    result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_1, &legend_outer_color);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_0, &legend_inner_color);
    if (result.code != CORE_OK) {
        return result;
    }
    legend_outer_color = graph_darkened_surface(legend_outer_color, 0.30f, 224u);
    legend_inner_color = graph_darkened_surface(legend_inner_color, 0.24f, 216u);
    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      legend_outer,
                                      6.0f,
                                      legend_outer_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }
    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      legend_inner,
                                      5.0f,
                                      legend_inner_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  (KitRenderRect){
                                                      legend_inner.x + 5.0f,
                                                      legend_inner.y + 2.0f,
                                                      legend_inner.width - 10.0f,
                                                      title_h
                                                  },
                                                  "EDGE KEY",
                                                  CORE_THEME_COLOR_TEXT_MUTED,
                                                  CORE_FONT_ROLE_UI_MEDIUM,
                                                  CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }

    if (input && kit_ui_point_in_rect(legend_inner, input->mouse_x, input->mouse_y)) {
        for (i = 0; i < row_count; ++i) {
            float y = legend_inner.y + pad + title_h + 2.0f + ((float)i * row_h);
            KitRenderRect row_rect = {
                legend_inner.x + 4.0f,
                y - 1.0f,
                legend_inner.width - 8.0f,
                row_h
            };
            if (kit_ui_point_in_rect(row_rect, input->mouse_x, input->mouse_y)) {
                hovered_row = i;
                break;
            }
        }
    }

    if (hovered_row >= 0 &&
        input &&
        (input->mouse_pressed || input->mouse_released)) {
        clicked_row = hovered_row;
        if (out_click_consumed) {
            *out_click_consumed = 1;
        }
    }

    if (clicked_row >= 0) {
        uint32_t before_mask = state->graph_kind_filter_mask;
        int before_all_override = state->graph_kind_filter_all_override;
        if (clicked_row == 0) {
            (void)mem_console_graph_kind_toggle_all_override(state);
        } else if (rows[clicked_row]) {
            (void)mem_console_graph_kind_toggle_enabled(state, rows[clicked_row]->kind);
        }
        if (before_mask != state->graph_kind_filter_mask ||
            before_all_override != state->graph_kind_filter_all_override) {
            if (out_filter_changed) {
                *out_filter_changed = 1;
            }
        }
    }

    for (i = 0; i < row_count; ++i) {
        float y = legend_inner.y + pad + title_h + 2.0f + ((float)i * row_h);
        KitRenderRect row_rect = {
            legend_inner.x + 4.0f,
            y - 1.0f,
            legend_inner.width - 8.0f,
            row_h
        };
        const char *label = "ALL";
        KitRenderColor row_color = (KitRenderColor){ 188, 196, 210, 255 };
        int row_selected = 0;
        KitRenderLineCommand swatch_line;
        KitRenderLineCommand underline;

        if (rows[i]) {
            label = rows[i]->label;
            row_color = rows[i]->color;
        }

        if (i == 0) {
            row_selected = state->graph_kind_filter_all_override ? 1 : 0;
        } else if (rows[i]) {
            row_selected = mem_console_graph_kind_is_enabled(state, rows[i]->kind);
        }

        if (row_selected) {
            result = draw_rect_outline(frame, row_rect, 1.0f, row_color);
            if (result.code != CORE_OK) {
                return result;
            }
        }

        swatch_line.p0 = (KitRenderVec2){ legend_inner.x + 6.0f, y + (row_h * 0.5f) + 0.5f };
        swatch_line.p1 = (KitRenderVec2){ legend_inner.x + 19.0f, y + (row_h * 0.5f) + 0.5f };
        swatch_line.thickness = 2.4f;
        swatch_line.color = row_color;
        swatch_line.transform = kit_render_identity_transform();
        result = kit_render_push_line(frame, &swatch_line);
        if (result.code != CORE_OK) {
            return result;
        }

        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      (KitRenderRect){
                                                          legend_inner.x + 24.0f,
                                                          y,
                                                          legend_inner.width - 28.0f,
                                                          row_h
                                                      },
                                                      label,
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_REGULAR,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }

        if (i == hovered_row) {
            underline.p0 = (KitRenderVec2){ legend_inner.x + 24.0f, y + row_h - 1.0f };
            underline.p1 = (KitRenderVec2){ legend_inner.x + legend_inner.width - 6.0f, y + row_h - 1.0f };
            underline.thickness = 1.0f;
            underline.color = row_color;
            underline.transform = kit_render_identity_transform();
            result = kit_render_push_line(frame, &underline);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    return core_result_ok();
}

static const char *graph_layout_mode_text(int layout_mode) {
    return layout_mode == MEM_CONSOLE_GRAPH_LAYOUT_TREE ? "tree" : "dag";
}

static const char *graph_scope_mode_text(const MemConsoleState *state) {
    if (!state) {
        return "focus";
    }
    return state->graph_scope_full_mode_enabled ? "full" : "focus";
}

CoreResult draw_graph_view_diagnostics(KitUiContext *ui_ctx,
                                       const KitRenderContext *render_ctx,
                                       KitRenderFrame *frame,
                                       MemConsoleState *state,
                                       KitRenderRect bounds,
                                       uint32_t node_count,
                                       uint32_t edge_count) {
    CoreResult result;
    KitRenderRect bar_rect = {
        bounds.x + 8.0f,
        bounds.y + bounds.height - 18.0f,
        300.0f,
        12.0f
    };
    KitRenderColor bar_color;

    if (!ui_ctx || !render_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph diagnostics draw request" };
    }
    if (bounds.width <= 28.0f || bounds.height <= 18.0f) {
        return core_result_ok();
    }

    if (bar_rect.width > bounds.width - 16.0f) {
        bar_rect.width = bounds.width - 16.0f;
    }
    if (bar_rect.y < bounds.y + 2.0f) {
        bar_rect.y = bounds.y + 2.0f;
    }

    (void)snprintf(state->graph_status_line,
                   sizeof(state->graph_status_line),
                   "scope:%s  layout:%s  lbl:%s  fnl:%s  zoom:%.2fx  n:%u e:%u",
                   graph_scope_mode_text(state),
                   graph_layout_mode_text(state->graph_layout_mode),
                   state->graph_edge_labels_enabled ? "on" : "off",
                   state->graph_anchor_funnel_enabled ? "on" : "off",
                   state->graph_viewport.zoom,
                   node_count,
                   edge_count);

    result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_0, &bar_color);
    if (result.code != CORE_OK) {
        return result;
    }
    bar_color = graph_darkened_surface(bar_color, 0.24f, 214u);
    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      (KitRenderRect){
                                          bar_rect.x - 2.0f,
                                          bar_rect.y - 1.0f,
                                          bar_rect.width + 4.0f,
                                          bar_rect.height + 2.0f
                                      },
                                      4.0f,
                                      bar_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }

    return mem_console_ui_draw_info_line_custom(ui_ctx,
                                                frame,
                                                bar_rect,
                                                state->graph_status_line,
                                                CORE_THEME_COLOR_TEXT_MUTED,
                                                CORE_FONT_ROLE_UI_REGULAR,
                                                CORE_FONT_TEXT_SIZE_CAPTION);
}
