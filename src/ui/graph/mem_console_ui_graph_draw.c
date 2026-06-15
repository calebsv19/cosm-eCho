#include "mem_console_ui_graph_internal.h"

#include <stdio.h>

static KitRenderColor mix_color(KitRenderColor a, KitRenderColor b, float t) {
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

static KitRenderColor scale_color(KitRenderColor color, float factor) {
    float f = factor;
    KitRenderColor out = color;

    if (f < 0.0f) {
        f = 0.0f;
    }
    if (f > 1.0f) {
        f = 1.0f;
    }

    out.r = (uint8_t)((float)color.r * f);
    out.g = (uint8_t)((float)color.g * f);
    out.b = (uint8_t)((float)color.b * f);
    out.a = color.a;
    return out;
}

static KitRenderColor derive_subtle_outline_color(KitRenderColor fill_color) {
    KitRenderColor out = fill_color;
    int luminance = ((int)fill_color.r * 30 +
                     (int)fill_color.g * 59 +
                     (int)fill_color.b * 11) / 100;
    int delta = 18;

    if (luminance < 96) {
        out.r = (uint8_t)(((int)fill_color.r + delta) > 255 ? 255 : ((int)fill_color.r + delta));
        out.g = (uint8_t)(((int)fill_color.g + delta) > 255 ? 255 : ((int)fill_color.g + delta));
        out.b = (uint8_t)(((int)fill_color.b + delta) > 255 ? 255 : ((int)fill_color.b + delta));
    } else {
        out.r = (uint8_t)(((int)fill_color.r - delta) < 0 ? 0 : ((int)fill_color.r - delta));
        out.g = (uint8_t)(((int)fill_color.g - delta) < 0 ? 0 : ((int)fill_color.g - delta));
        out.b = (uint8_t)(((int)fill_color.b - delta) < 0 ? 0 : ((int)fill_color.b - delta));
    }

    out.a = 255u;
    return out;
}

static int color_luminance(KitRenderColor color) {
    return ((int)color.r * 30 + (int)color.g * 59 + (int)color.b * 11) / 100;
}

static int graph_draw_find_layout_index_by_node_id(const KitGraphStructNodeLayout *layouts,
                                                   uint32_t layout_count,
                                                   uint32_t node_id) {
    uint32_t i;

    if (!layouts) {
        return -1;
    }
    for (i = 0u; i < layout_count; ++i) {
        if (layouts[i].node_id == node_id) {
            return (int)i;
        }
    }
    return -1;
}

static float graph_endpoint_marker_radius_for_layout(const KitGraphStructNodeLayout *layout) {
    float node_min_span;
    float marker_r;

    if (!layout) {
        return 0.22f;
    }
    node_min_span = layout->rect.width < layout->rect.height
                        ? layout->rect.width
                        : layout->rect.height;
    marker_r = node_min_span * 0.16f;
    return graph_clampf(marker_r, 0.22f, 2.2f);
}

static CoreResult draw_graph_endpoint_markers(const KitRenderContext *render_ctx,
                                              KitRenderFrame *frame,
                                              const MemConsoleState *state) {
    CoreResult result;
    KitRenderColor out_color;
    KitRenderColor in_color;
    uint32_t i;

    if (!render_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph endpoint marker request" };
    }

    result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_STATUS_OK, &out_color);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_STATUS_ERROR, &in_color);
    if (result.code != CORE_OK) {
        return result;
    }

    for (i = 0u; i < state->graph_layout_edge_count; ++i) {
        const KitGraphStructEdge *layout_edge = &state->graph_layout_edges[i];
        const KitGraphStructEdgeRoute *route = &state->graph_layout_edge_routes[i];
        int from_layout_index;
        int to_layout_index;
        uint32_t last_index;
        KitRenderVec2 start;
        KitRenderVec2 end;
        float start_r = 0.22f;
        float end_r = 0.22f;
        KitRenderRectCommand marker_rect;

        if (route->point_count < 2u) {
            continue;
        }
        from_layout_index = graph_draw_find_layout_index_by_node_id(state->graph_layout_node_layouts,
                                                                     state->graph_layout_node_count,
                                                                     layout_edge->from_id);
        to_layout_index = graph_draw_find_layout_index_by_node_id(state->graph_layout_node_layouts,
                                                                   state->graph_layout_node_count,
                                                                   layout_edge->to_id);
        if (from_layout_index >= 0 && (uint32_t)from_layout_index < state->graph_layout_node_count) {
            start_r = graph_endpoint_marker_radius_for_layout(&state->graph_layout_node_layouts[from_layout_index]);
        }
        if (to_layout_index >= 0 && (uint32_t)to_layout_index < state->graph_layout_node_count) {
            end_r = graph_endpoint_marker_radius_for_layout(&state->graph_layout_node_layouts[to_layout_index]);
        }
        last_index = route->point_count - 1u;
        start = route->points[0];
        end = route->points[last_index];
        marker_rect.transform = kit_render_identity_transform();

        marker_rect.rect = (KitRenderRect){
            start.x - start_r,
            start.y - start_r,
            start_r * 2.0f,
            start_r * 2.0f
        };
        marker_rect.corner_radius = start_r;
        marker_rect.color = out_color;
        result = kit_render_push_rect(frame, &marker_rect);
        if (result.code != CORE_OK) {
            return result;
        }

        marker_rect.rect = (KitRenderRect){
            end.x - end_r,
            end.y - end_r,
            end_r * 2.0f,
            end_r * 2.0f
        };
        marker_rect.corner_radius = end_r;
        marker_rect.color = in_color;
        result = kit_render_push_rect(frame, &marker_rect);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    return core_result_ok();
}

CoreResult mem_console_ui_graph_draw_preview(const KitRenderContext *render_ctx,
                                             KitUiContext *ui_ctx,
                                             const KitUiInputState *input,
                                             KitRenderFrame *frame,
                                             KitRenderRect bounds,
                                             MemConsoleState *state,
                                             int *out_legend_click_consumed,
                                             int *out_graph_filter_changed) {
    CoreResult result;
    KitRenderColor edge_white;
    KitRenderColor node_base_color;
    KitRenderColor node_lane_color;
    KitRenderColor node_selected_color;
    KitRenderColor node_center_outline_color;
    KitRenderColor graph_bg_color;
    CoreResult color_result;
    uint32_t node_count = 0u;
    uint32_t edge_count = 0u;
    uint32_t i;
    int has_graph_data = 0;
    int hovered_node_index = -1;
    int hovered_edge_index = -1;
    int graph_view_mode = MEM_CONSOLE_GRAPH_VIEW_FOCUS;

    if (!render_ctx || !ui_ctx || !frame || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid argument" };
    }
    if (out_legend_click_consumed) {
        *out_legend_click_consumed = 0;
    }
    if (out_graph_filter_changed) {
        *out_graph_filter_changed = 0;
    }

    result = mem_console_ui_graph_ensure_layout_cache(render_ctx, state, bounds);
    if (result.code != CORE_OK) {
        return result;
    }
    node_count = state->graph_layout_node_count;
    edge_count = state->graph_layout_edge_count;
    has_graph_data = state->graph_layout_has_graph_data;
    graph_view_mode = mem_console_graph_view_mode_get(state);

    if (input && has_graph_data && node_count > 0u &&
        kit_ui_point_in_rect(bounds, input->mouse_x, input->mouse_y)) {
        uint32_t rect_hit_index = 0u;
        if (mem_console_ui_graph_find_node_index_at_point(state,
                                                          input->mouse_x,
                                                          input->mouse_y,
                                                          &rect_hit_index) &&
            rect_hit_index < node_count) {
            hovered_node_index = (int)rect_hit_index;
        } else {
            KitGraphStructHit hit = {0};
            CoreResult hit_result = kit_graph_struct_hit_test(state->graph_layout_node_layouts,
                                                              node_count,
                                                              input->mouse_x,
                                                              input->mouse_y,
                                                              &hit);
            if (hit_result.code == CORE_OK && hit.active && hit.node_index < node_count) {
                hovered_node_index = (int)hit.node_index;
            }
        }
        if (hovered_node_index < 0 && edge_count > 0u) {
            uint32_t edge_index = 0u;
            if (graph_find_edge_index_at_point(state,
                                               input->mouse_x,
                                               input->mouse_y,
                                               graph_edge_route_hit_radius_for_zoom(state),
                                               &edge_index)) {
                hovered_edge_index = (int)edge_index;
            }
        }
    }

    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_TEXT_PRIMARY, &edge_white);
    if (color_result.code != CORE_OK) {
        return color_result;
    }
    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_1, &node_base_color);
    if (color_result.code != CORE_OK) {
        return color_result;
    }
    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_2, &node_lane_color);
    if (color_result.code != CORE_OK) {
        return color_result;
    }
    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_ACCENT_PRIMARY, &node_selected_color);
    if (color_result.code != CORE_OK) {
        return color_result;
    }
    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_STATUS_OK, &node_center_outline_color);
    if (color_result.code != CORE_OK) {
        return color_result;
    }
    color_result = mem_console_ui_resolve_theme_color(render_ctx, CORE_THEME_COLOR_SURFACE_0, &graph_bg_color);
    if (color_result.code != CORE_OK) {
        return color_result;
    }

    result = draw_project_pod_overlays(render_ctx, frame, state, bounds);
    if (result.code != CORE_OK) {
        return result;
    }

    for (i = 0u; i < edge_count; ++i) {
        KitRenderLineCommand line_cmd;
        KitRenderTextCommand label_text_cmd;
        KitRenderRect label_bg_rect = state->graph_layout_edge_label_layouts[i].rect;
        KitRenderVec2 label_anchor = state->graph_layout_edge_label_layouts[i].anchor;
        KitRenderVec2 label_attach;
        const KitGraphStructEdgeRoute *route = &state->graph_layout_edge_routes[i];
        int state_edge_index = state->graph_layout_edge_state_indices[i];
        const char *edge_kind_raw = "related";
        const char *edge_kind_label = "RELATED";
        KitRenderColor edge_draw_color;
        KitRenderColor label_outline_color;
        uint8_t edge_alpha = 255u;
        float line_thickness_scale = 1.0f;
        int is_hovered_edge = (int)i == hovered_edge_index;
        int is_hierarchy_edge = 0;
        int edge_touches_selected = 0;
        if (state_edge_index >= 0 &&
            state_edge_index < state->graph_edge_count &&
            state->graph_edges[state_edge_index].kind[0] != '\0') {
            edge_kind_raw = state->graph_edges[state_edge_index].kind;
        }
        if (state_edge_index >= 0 && state_edge_index < state->graph_edge_count) {
            int from_index = state->graph_edges[state_edge_index].from_index;
            int to_index = state->graph_edges[state_edge_index].to_index;

            if (from_index >= 0 &&
                from_index < state->graph_node_count &&
                state->graph_nodes[from_index].item_id == state->selected_item_id) {
                edge_touches_selected = 1;
            }
            if (to_index >= 0 &&
                to_index < state->graph_node_count &&
                state->graph_nodes[to_index].item_id == state->selected_item_id) {
                edge_touches_selected = 1;
            }
        }
        edge_kind_label = graph_edge_display_label_for_kind(edge_kind_raw);
        edge_draw_color = graph_edge_color_for_kind(edge_kind_raw);
        is_hierarchy_edge = graph_edge_is_hierarchy_kind(edge_kind_raw);
        if (!is_hierarchy_edge) {
            edge_draw_color = mix_color(edge_draw_color, edge_white, 0.38f);
        }
        if (graph_view_mode == MEM_CONSOLE_GRAPH_VIEW_WEB) {
            line_thickness_scale = is_hierarchy_edge ? 0.88f : 0.76f;
            edge_alpha = is_hierarchy_edge ? 184u : 142u;
        } else if (graph_view_mode == MEM_CONSOLE_GRAPH_VIEW_FOCUS) {
            if (edge_touches_selected) {
                line_thickness_scale = is_hierarchy_edge ? 1.06f : 0.94f;
                edge_alpha = is_hierarchy_edge ? 222u : 190u;
            } else {
                line_thickness_scale = is_hierarchy_edge ? 0.72f : 0.56f;
                edge_alpha = is_hierarchy_edge ? 138u : 96u;
            }
        }
        if (route->point_count < 2u) {
            continue;
        }
        {
            float text_w = mem_console_ui_measure_text_width_px(render_ctx,
                                                                CORE_FONT_ROLE_UI_REGULAR,
                                                                CORE_FONT_TEXT_SIZE_CAPTION,
                                                                edge_kind_label);
            float desired_w = text_w + 6.0f;
            float center_x = label_bg_rect.x + (label_bg_rect.width * 0.5f);
            float min_x = bounds.x + 2.0f;
            float max_x = (bounds.x + bounds.width) - desired_w - 2.0f;

            if (desired_w < 14.0f) {
                desired_w = 14.0f;
            }
            if (desired_w > bounds.width - 4.0f) {
                desired_w = bounds.width - 4.0f;
            }
            if (label_bg_rect.width < desired_w) {
                label_bg_rect.width = desired_w;
            }
            label_bg_rect.x = center_x - (label_bg_rect.width * 0.5f);
            if (max_x < min_x) {
                max_x = min_x;
            }
            if (label_bg_rect.x < min_x) {
                label_bg_rect.x = min_x;
            }
            if (label_bg_rect.x > max_x) {
                label_bg_rect.x = max_x;
            }
        }

        {
            uint32_t p;
            for (p = 0u; p + 1u < route->point_count; ++p) {
                line_cmd.p0 = route->points[p];
                line_cmd.p1 = route->points[p + 1u];
                line_cmd.thickness = is_hierarchy_edge
                                         ? (is_hovered_edge ? 4.0f : 2.8f)
                                         : (is_hovered_edge ? 3.0f : 1.6f);
                line_cmd.thickness *= line_thickness_scale;
                line_cmd.color = edge_draw_color;
                if (is_hovered_edge) {
                    line_cmd.color = mix_color(edge_draw_color, edge_white, 0.26f);
                }
                line_cmd.color.a = is_hovered_edge ? 230u : edge_alpha;
                line_cmd.transform = kit_render_identity_transform();
                result = kit_render_push_line(frame, &line_cmd);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
        if (!state->graph_edge_labels_enabled) {
            continue;
        }
        if (label_bg_rect.width <= 0.0f || label_bg_rect.height <= 0.0f) {
            continue;
        }
        label_attach = compute_label_attach_point(label_bg_rect, label_anchor);
        line_cmd.p0 = label_anchor;
        line_cmd.p1 = label_attach;
        line_cmd.thickness = (is_hierarchy_edge ? 1.8f : 1.1f) * line_thickness_scale;
        line_cmd.color = edge_draw_color;
        if (is_hovered_edge) {
            line_cmd.color = mix_color(edge_draw_color, edge_white, 0.22f);
        }
        line_cmd.color.a = is_hovered_edge ? 224u : edge_alpha;
        line_cmd.transform = kit_render_identity_transform();
        result = kit_render_push_line(frame, &line_cmd);
        if (result.code != CORE_OK) {
            return result;
        }

        result = mem_console_ui_push_themed_rect(render_ctx,
                                                 frame,
                                                 label_bg_rect,
                                                 2.0f,
                                                 CORE_THEME_COLOR_SURFACE_0);
        if (result.code != CORE_OK) {
            return result;
        }
        label_outline_color = edge_draw_color;
        if (is_hovered_edge) {
            label_outline_color = mix_color(label_outline_color, edge_white, 0.20f);
        }
        if (is_hovered_edge) {
            label_outline_color.a = 228u;
        } else if (edge_alpha >= 231u) {
            label_outline_color.a = 255u;
        } else {
            label_outline_color.a = (uint8_t)(edge_alpha + 24u);
        }
        result = draw_rect_outline(frame, label_bg_rect, 1.0f, label_outline_color);
        if (result.code != CORE_OK) {
            return result;
        }
        result = kit_render_push_rect(frame,
                                      &(KitRenderRectCommand){
                                          (KitRenderRect){
                                              label_anchor.x - 1.5f,
                                              label_anchor.y - 1.5f,
                                              3.0f,
                                              3.0f
                                          },
                                          1.5f,
                                          label_outline_color,
                                          kit_render_identity_transform()
                                      });
        if (result.code != CORE_OK) {
            return result;
        }

        label_text_cmd.origin.x = label_bg_rect.x + 3.0f;
        label_text_cmd.origin.y = label_bg_rect.y + (label_bg_rect.height * 0.5f);
        (void)snprintf(state->graph_draw_edge_labels[i],
                       sizeof(state->graph_draw_edge_labels[i]),
                       "%s",
                       edge_kind_label);
        label_text_cmd.text = state->graph_draw_edge_labels[i];
        label_text_cmd.font_role = CORE_FONT_ROLE_UI_REGULAR;
        label_text_cmd.text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
        label_text_cmd.color_token = is_hierarchy_edge
                                         ? CORE_THEME_COLOR_TEXT_PRIMARY
                                         : CORE_THEME_COLOR_TEXT_MUTED;
        label_text_cmd.transform = kit_render_identity_transform();
        result = kit_render_push_text(frame, &label_text_cmd);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    for (i = 0u; i < node_count; ++i) {
        KitRenderRect node_rect = state->graph_layout_node_layouts[i].rect;
        float node_w = node_rect.width;
        float node_h = node_rect.height;
        float outline_thickness = (node_w < 7.0f || node_h < 6.0f) ? 0.6f : 1.0f;
        float corner_radius = graph_clampf((node_w < node_h ? node_w : node_h) * 0.18f, 0.6f, 2.4f);
        float halo_pad_x = graph_clampf(node_w * 0.08f, 0.0f, 1.2f);
        float halo_pad_y = graph_clampf(node_h * 0.10f, 0.0f, 1.0f);
        int tiny_node = node_w <= GRAPH_NODE_TEXT_HIDE_WIDTH_PX ||
                        node_h <= GRAPH_NODE_TEXT_HIDE_HEIGHT_PX;
        KitRenderColor fill_color = mix_color(node_base_color, edge_white, 0.56f);
        KitRenderColor node_outline_color;
        KitRenderColor project_color = (KitRenderColor){ 0, 0, 0, 0 };
        int has_project_color = 0;
        KitRenderRectCommand rect_cmd;
        KitRenderTextCommand text_cmd;
        CoreThemeColorToken text_color_token = CORE_THEME_COLOR_TEXT_PRIMARY;
        GraphBucketRole bucket_role = GRAPH_BUCKET_ROLE_NONE;
        int anchor_hidden = 0;
        const MemConsoleGraphNode *graph_node = 0;

        if (has_graph_data) {
            graph_node = &state->graph_nodes[i];
            KitRenderColor bucket_border;
            bucket_role = graph_bucket_role_for_node(graph_node);
            if (graph_node->project_key[0] != '\0') {
                project_color = mem_console_ui_project_color_for_key(graph_node->project_key);
                has_project_color = 1;
                fill_color = scale_color(project_color, 0.52f);
            }
            if (graph_node->item_id == state->selected_item_id) {
                fill_color = mix_color(fill_color, node_selected_color, 0.28f);
                fill_color = mix_color(fill_color, edge_white, 0.12f);
            } else if (graph_node->canonical || graph_node->pinned) {
                fill_color = mix_color(fill_color, node_lane_color, 0.34f);
            }
            if (bucket_role != GRAPH_BUCKET_ROLE_NONE) {
                bucket_border = graph_bucket_border_color(bucket_role);
                fill_color = mix_color(fill_color, bucket_border, 0.20f);
                anchor_hidden = mem_console_graph_anchor_hidden_is_set(state, graph_node->item_id);
                if (anchor_hidden) {
                    fill_color = mix_color(fill_color, graph_bg_color, 0.58f);
                }
            }
            if ((int)i == hovered_node_index && graph_node->item_id != state->selected_item_id) {
                fill_color = mix_color(fill_color, edge_white, 0.16f);
            }
        }

        node_outline_color = mix_color(graph_bg_color, edge_white, 0.68f);
        if (has_project_color) {
            node_outline_color = mix_color(node_outline_color, project_color, 0.40f);
        }
        if (bucket_role != GRAPH_BUCKET_ROLE_NONE) {
            node_outline_color = mix_color(node_outline_color,
                                           graph_bucket_border_color(bucket_role),
                                           0.42f);
            if (anchor_hidden) {
                node_outline_color = mix_color(node_outline_color, graph_bg_color, 0.46f);
            }
        } else {
            node_outline_color = derive_subtle_outline_color(fill_color);
        }
        if (color_luminance(fill_color) >= 148) {
            text_color_token = CORE_THEME_COLOR_SURFACE_0;
        }
        if (anchor_hidden) {
            text_color_token = CORE_THEME_COLOR_TEXT_MUTED;
        }

        if (!tiny_node && halo_pad_x > 0.05f && halo_pad_y > 0.05f) {
            rect_cmd.rect = (KitRenderRect){
                node_rect.x - halo_pad_x,
                node_rect.y - halo_pad_y,
                node_rect.width + (halo_pad_x * 2.0f),
                node_rect.height + (halo_pad_y * 2.0f)
            };
            rect_cmd.corner_radius = corner_radius + 0.8f;
            rect_cmd.color = graph_bg_color;
            rect_cmd.color.a = 96u;
            rect_cmd.transform = kit_render_identity_transform();
            result = kit_render_push_rect(frame, &rect_cmd);
            if (result.code != CORE_OK) {
                return result;
            }
        }

        rect_cmd.rect = node_rect;
        rect_cmd.corner_radius = corner_radius;
        rect_cmd.color = fill_color;
        rect_cmd.color.a = anchor_hidden ? 214u : 248u;
        rect_cmd.transform = kit_render_identity_transform();
        result = kit_render_push_rect(frame, &rect_cmd);
        if (result.code != CORE_OK) {
            return result;
        }

        result = draw_rect_outline(frame, node_rect, outline_thickness, node_outline_color);
        if (result.code != CORE_OK) {
            return result;
        }

        if (has_graph_data &&
            i < (uint32_t)state->graph_node_count &&
            state->graph_nodes[i].item_id == state->selected_item_id) {
            KitRenderColor selected_outline = mix_color(node_selected_color, edge_white, 0.32f);
            result = draw_rect_outline(frame,
                                       node_rect,
                                       outline_thickness + 0.5f,
                                       selected_outline);
            if (result.code != CORE_OK) {
                return result;
            }
        }
        if (has_graph_data &&
            i < (uint32_t)state->graph_node_count) {
            GraphBucketRole bucket_role = graph_bucket_role_for_node(&state->graph_nodes[i]);
            if (bucket_role != GRAPH_BUCKET_ROLE_NONE) {
                result = draw_rect_outline(frame,
                                           node_rect,
                                           outline_thickness + 1.0f,
                                           graph_bucket_border_color(bucket_role));
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }

        if (has_graph_data &&
            i < (uint32_t)state->graph_node_count &&
            state->graph_nodes[i].item_id == state->graph_center_item_id) {
            result = draw_rect_outline(frame,
                                       node_rect,
                                       outline_thickness + 0.8f,
                                       node_center_outline_color);
            if (result.code != CORE_OK) {
                return result;
            }
        }

        if (!graph_node_should_render_text(state, i, node_w, node_h)) {
            continue;
        }
        {
            float text_pad_x = graph_clampf(node_w * 0.06f, 0.5f, 1.4f);
            float text_pad_y = graph_clampf(node_h * 0.06f, 0.4f, 1.0f);
            float text_max_w = node_w - (text_pad_x * 2.0f);
            float text_max_h = node_h - (text_pad_y * 2.0f);
            float id_width_px;

            if (text_max_w <= 1.0f || text_max_h <= 1.0f) {
                continue;
            }

            if (graph_node) {
                (void)snprintf(state->graph_draw_node_labels[i],
                               sizeof(state->graph_draw_node_labels[i]),
                               "%lld",
                               (long long)graph_node->item_id);
            } else {
                (void)snprintf(state->graph_draw_node_labels[i],
                               sizeof(state->graph_draw_node_labels[i]),
                               "0");
            }
            id_width_px = mem_console_ui_measure_text_width_px(render_ctx,
                                                                CORE_FONT_ROLE_UI_MONO_SMALL,
                                                                CORE_FONT_TEXT_SIZE_CAPTION,
                                                                state->graph_draw_node_labels[i]);
            if (id_width_px > text_max_w || text_max_h < 7.0f) {
                continue;
            }

            text_cmd.origin.x = node_rect.x + text_pad_x;
            text_cmd.origin.y = node_rect.y + (node_h * 0.5f);
            text_cmd.text = state->graph_draw_node_labels[i];
            text_cmd.font_role = CORE_FONT_ROLE_UI_MONO_SMALL;
            text_cmd.text_tier = CORE_FONT_TEXT_SIZE_CAPTION;
            text_cmd.color_token = text_color_token;
            text_cmd.transform = kit_render_identity_transform();
            result = kit_render_push_text(frame, &text_cmd);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    result = draw_graph_endpoint_markers(render_ctx, frame, state);
    if (result.code != CORE_OK) {
        return result;
    }
    result = draw_graph_edge_legend(ui_ctx,
                                    render_ctx,
                                    input,
                                    frame,
                                    bounds,
                                    state,
                                    out_legend_click_consumed,
                                    out_graph_filter_changed);
    if (result.code != CORE_OK) {
        return result;
    }
    result = draw_graph_view_diagnostics(ui_ctx,
                                         render_ctx,
                                         frame,
                                         state,
                                         bounds,
                                         node_count,
                                         edge_count);
    if (result.code != CORE_OK) {
        return result;
    }

    if (has_graph_data &&
        hovered_node_index >= 0 &&
        hovered_node_index < state->graph_node_count) {
        MemConsoleUiHudCardSpec hud_spec;
        KitRenderVec2 hud_anchor = {
            bounds.x + (bounds.width * 0.5f),
            bounds.y + (bounds.height * 0.5f)
        };
        if (input) {
            hud_anchor.x = input->mouse_x;
            hud_anchor.y = input->mouse_y;
        }
        if (graph_build_node_hud_spec(state, hovered_node_index, &hud_spec)) {
            result = mem_console_ui_hud_draw_cached(render_ctx,
                                                    ui_ctx,
                                                    frame,
                                                    bounds,
                                                    hud_anchor,
                                                    state,
                                                    &hud_spec);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    } else if (has_graph_data &&
               hovered_edge_index >= 0 &&
               hovered_edge_index < (int)state->graph_layout_edge_count) {
        MemConsoleUiHudCardSpec hud_spec;
        KitRenderVec2 hud_anchor = {
            bounds.x + (bounds.width * 0.5f),
            bounds.y + (bounds.height * 0.5f)
        };
        if (input) {
            hud_anchor.x = input->mouse_x;
            hud_anchor.y = input->mouse_y;
        }
        if (graph_build_edge_hud_spec(state, hovered_edge_index, &hud_spec)) {
            result = mem_console_ui_hud_draw_cached(render_ctx,
                                                    ui_ctx,
                                                    frame,
                                                    bounds,
                                                    hud_anchor,
                                                    state,
                                                    &hud_spec);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    return core_result_ok();
}
