#include "mem_console_ui_detail_section_internal.h"

#include "mem_console_ui_common.h"

#include <stdio.h>
#include <string.h>

static int relationship_group_changed(const MemConsoleRelationshipItem *prev,
                                      const MemConsoleRelationshipItem *item) {
    if (!item) {
        return 0;
    }
    if (!prev) {
        return 1;
    }
    if (prev->outgoing != item->outgoing) {
        return 1;
    }
    if (strncmp(prev->kind, item->kind, sizeof(item->kind)) != 0) {
        return 1;
    }
    return 0;
}

static int relationship_group_count(const MemConsoleState *state) {
    const MemConsoleRelationshipItem *prev = 0;
    int count = 0;
    int i;

    if (!state) {
        return 0;
    }

    for (i = 0; i < state->detail_relationship_count; ++i) {
        const MemConsoleRelationshipItem *item = &state->detail_relationships[i];
        if (relationship_group_changed(prev, item)) {
            count += 1;
        }
        prev = item;
    }
    return count;
}

static void relationship_format_group_label(MemConsoleState *state,
                                            int group_index,
                                            const MemConsoleRelationshipItem *item) {
    const char *direction;
    const char *kind;

    if (!state || group_index < 0 || group_index >= MEM_CONSOLE_DETAIL_RELATIONSHIP_LIMIT || !item) {
        return;
    }

    direction = item->outgoing ? "OUT" : "IN";
    kind = item->kind[0] ? item->kind : "RELATED";
    (void)snprintf(state->detail_relationship_group_labels[group_index],
                   sizeof(state->detail_relationship_group_labels[group_index]),
                   "%s %s",
                   direction,
                   kind);
}

static void relationship_format_row_label(MemConsoleState *state,
                                          int row_index,
                                          const MemConsoleRelationshipItem *item) {
    const char *arrow;
    const char *project_key;
    const char *neighbor_kind;
    const char *title;

    if (!state || row_index < 0 || row_index >= MEM_CONSOLE_DETAIL_RELATIONSHIP_LIMIT || !item) {
        return;
    }

    arrow = item->outgoing ? "->" : "<-";
    project_key = item->neighbor_project_key[0] ? item->neighbor_project_key : "misc";
    neighbor_kind = item->neighbor_kind[0] ? item->neighbor_kind : "memory";
    title = item->neighbor_title[0] ? item->neighbor_title : "UNTITLED";

    (void)snprintf(state->detail_relationship_row_labels[row_index],
                   sizeof(state->detail_relationship_row_labels[row_index]),
                   "%s %lld [%s] %s | %s",
                   arrow,
                   (long long)item->neighbor_item_id,
                   project_key,
                   neighbor_kind,
                   title);
}

static CoreResult relationship_draw_empty(KitUiContext *ui_ctx,
                                          KitRenderFrame *frame,
                                          KitRenderRect bounds,
                                          MemConsoleState *state) {
    (void)snprintf(state->detail_connection_summary_lines[0],
                   sizeof(state->detail_connection_summary_lines[0]),
                   "%s",
                   state->selected_item_id == 0
                       ? "Select a memory to inspect relationships."
                       : "No relationships for selected memory.");
    return mem_console_ui_draw_info_line_custom(ui_ctx,
                                                frame,
                                                bounds,
                                                state->detail_connection_summary_lines[0],
                                                CORE_THEME_COLOR_TEXT_MUTED,
                                                CORE_FONT_ROLE_UI_REGULAR,
                                                CORE_FONT_TEXT_SIZE_CAPTION);
}

static CoreResult relationship_draw_add_controls(KitRenderContext *render_ctx,
                                                 KitUiContext *ui_ctx,
                                                 KitRenderFrame *frame,
                                                 MemConsoleState *state,
                                                 const KitUiInputState *input,
                                                 KitRenderRect bounds,
                                                 int navigation_enabled,
                                                 MemConsoleAction *io_action) {
    KitRenderRect label_rect;
    KitRenderRect input_rect;
    KitRenderRect add_rect;
    KitUiButtonResult button;
    CoreResult result;
    float add_w = 52.0f;
    float label_w = 66.0f;
    float gap = 4.0f;

    if (!render_ctx || !ui_ctx || !frame || !state || !input) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid relationship controls draw request" };
    }
    if (bounds.width <= 40.0f || bounds.height <= 8.0f) {
        return core_result_ok();
    }

    if (bounds.width < 170.0f) {
        label_w = 0.0f;
    }
    if (bounds.width < 120.0f) {
        add_w = 44.0f;
    }

    label_rect = (KitRenderRect){ bounds.x, bounds.y, label_w, bounds.height };
    input_rect = (KitRenderRect){
        bounds.x + label_w + (label_w > 0.0f ? gap : 0.0f),
        bounds.y,
        bounds.width - label_w - add_w - gap - (label_w > 0.0f ? gap : 0.0f),
        bounds.height
    };
    add_rect = (KitRenderRect){
        bounds.x + bounds.width - add_w,
        bounds.y,
        add_w,
        bounds.height
    };
    if (input_rect.width < 38.0f) {
        input_rect.width = 38.0f;
    }

    if (label_w > 0.0f) {
        result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                      frame,
                                                      label_rect,
                                                      "TARGET",
                                                      CORE_THEME_COLOR_TEXT_MUTED,
                                                      CORE_FONT_ROLE_UI_MEDIUM,
                                                      CORE_FONT_TEXT_SIZE_CAPTION);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             input_rect,
                                             5.0f,
                                             CORE_THEME_COLOR_SURFACE_2);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_draw_editable_line(ui_ctx,
                                               render_ctx,
                                               frame,
                                               input_rect,
                                               state->relationship_target_text,
                                               CORE_THEME_COLOR_TEXT_PRIMARY,
                                               CORE_FONT_ROLE_UI_REGULAR,
                                               CORE_FONT_TEXT_SIZE_CAPTION,
                                               state->input_target == MEM_CONSOLE_INPUT_RELATIONSHIP_TARGET,
                                               state->relationship_target_cursor);
    if (result.code != CORE_OK) {
        return result;
    }
    if (input->mouse_released && kit_ui_point_in_rect(input_rect, input->mouse_x, input->mouse_y)) {
        float text_origin_x = input_rect.x + ui_ctx->style.padding;
        state->input_target = MEM_CONSOLE_INPUT_RELATIONSHIP_TARGET;
        state->relationship_target_cursor = mem_console_ui_cursor_index_for_click(state->relationship_target_text,
                                                                                  render_ctx,
                                                                                  input->mouse_x,
                                                                                  text_origin_x,
                                                                                  CORE_FONT_ROLE_UI_REGULAR,
                                                                                  CORE_FONT_TEXT_SIZE_CAPTION);
    }

    button = kit_ui_eval_button(add_rect,
                                input,
                                navigation_enabled && state->selected_item_id > 0 &&
                                    state->relationship_target_text[0] != '\0');
    if (button.clicked && io_action && *io_action == MEM_CONSOLE_ACTION_NONE) {
        *io_action = MEM_CONSOLE_ACTION_ADD_RELATIONSHIP;
    }
    result = mem_console_ui_draw_button_custom(ui_ctx,
                                               frame,
                                               add_rect,
                                               "ADD",
                                               button.state,
                                               CORE_FONT_ROLE_UI_MEDIUM,
                                               CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }

    return core_result_ok();
}

CoreResult mem_console_ui_detail_draw_relationships(KitRenderContext *render_ctx,
                                                    KitUiContext *ui_ctx,
                                                    KitRenderFrame *frame,
                                                    MemConsoleState *state,
                                                    const KitUiInputState *input,
                                                    int wheel_y,
                                                    const MemConsoleLayoutConfig *layout_cfg,
                                                    MemConsoleAction *io_action) {
    KitRenderRect panel;
    KitRenderRect header_rect;
    KitRenderRect content_viewport;
    KitRenderRect draw_rect;
    const float group_h = 18.0f;
    const float row_h = 24.0f;
    const float top_pad = 5.0f;
    const float controls_h = 22.0f;
    float content_height;
    float scroll;
    int group_total;
    int group_index = 0;
    int has_scrollbar;
    int navigation_enabled;
    const MemConsoleRelationshipItem *prev = 0;
    CoreResult result;
    int i;

    if (!render_ctx || !ui_ctx || !frame || !state || !input || !layout_cfg) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid relationship inspector draw request" };
    }

    panel = (KitRenderRect){
        state->pane_right_detail_connections.x + layout_cfg->panel_inner_padding,
        state->pane_right_detail_connections.y + layout_cfg->panel_inner_padding,
        state->pane_right_detail_connections.width - (layout_cfg->panel_inner_padding * 2.0f),
        state->pane_right_detail_connections.height - (layout_cfg->panel_inner_padding * 2.0f)
    };

    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             panel,
                                             8.0f,
                                             CORE_THEME_COLOR_SURFACE_1);
    if (result.code != CORE_OK) {
        return result;
    }

    header_rect = (KitRenderRect){
        panel.x + 8.0f,
        panel.y + 5.0f,
        panel.width - 16.0f,
        18.0f
    };
    result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                  frame,
                                                  header_rect,
                                                  state->detail_relationship_summary_line[0]
                                                      ? state->detail_relationship_summary_line
                                                      : "RELATIONSHIPS",
                                                  CORE_THEME_COLOR_TEXT_MUTED,
                                                  CORE_FONT_ROLE_UI_MEDIUM,
                                                  CORE_FONT_TEXT_SIZE_CAPTION);
    if (result.code != CORE_OK) {
        return result;
    }

    navigation_enabled = !state->title_edit_mode && !state->body_edit_mode && !state->db_modal_open;
    result = relationship_draw_add_controls(render_ctx,
                                            ui_ctx,
                                            frame,
                                            state,
                                            input,
                                            (KitRenderRect){
                                                panel.x + 8.0f,
                                                panel.y + 27.0f,
                                                panel.width - 16.0f,
                                                controls_h
                                            },
                                            navigation_enabled,
                                            io_action);
    if (result.code != CORE_OK) {
        return result;
    }

    content_viewport = (KitRenderRect){
        panel.x + 6.0f,
        panel.y + 54.0f,
        panel.width - 12.0f,
        panel.height - 59.0f
    };
    if (content_viewport.width <= 4.0f || content_viewport.height <= 4.0f) {
        return core_result_ok();
    }

    if (state->detail_relationship_count <= 0) {
        return relationship_draw_empty(ui_ctx,
                                       frame,
                                       content_viewport,
                                       state);
    }

    group_total = relationship_group_count(state);
    content_height = top_pad +
                     ((float)group_total * group_h) +
                     ((float)state->detail_relationship_count * row_h) +
                     8.0f;
    if (content_height < content_viewport.height) {
        content_height = content_viewport.height;
    }

    scroll = state->detail_connection_scroll;
    if (scroll < 0.0f) {
        scroll = 0.0f;
    }
    if (scroll > content_height - content_viewport.height) {
        scroll = content_height - content_viewport.height;
    }
    if (scroll < 0.0f) {
        scroll = 0.0f;
    }

    if (wheel_y != 0 && kit_ui_point_in_rect(content_viewport, input->mouse_x, input->mouse_y)) {
        KitUiScrollResult scroll_result = kit_ui_eval_scroll(content_viewport,
                                                             scroll,
                                                             content_height,
                                                             (float)wheel_y);
        if (scroll_result.changed) {
            scroll = scroll_result.offset_y;
        }
    }
    state->detail_connection_scroll = scroll;

    has_scrollbar = (content_height - content_viewport.height) > 0.5f;
    if (has_scrollbar) {
        content_viewport.width -= 10.0f;
        if (content_viewport.width < 20.0f) {
            content_viewport.width = 20.0f;
        }
    }

    result = kit_ui_clip_push(ui_ctx, frame, content_viewport);
    if (result.code != CORE_OK) {
        return result;
    }

    draw_rect = (KitRenderRect){
        content_viewport.x,
        content_viewport.y + top_pad - scroll,
        content_viewport.width,
        group_h
    };
    for (i = 0; i < state->detail_relationship_count; ++i) {
        const MemConsoleRelationshipItem *item = &state->detail_relationships[i];

        if (relationship_group_changed(prev, item)) {
            relationship_format_group_label(state, group_index, item);
            draw_rect.height = group_h;
            if (draw_rect.y + draw_rect.height >= content_viewport.y &&
                draw_rect.y <= content_viewport.y + content_viewport.height) {
                result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                              frame,
                                                              draw_rect,
                                                              state->detail_relationship_group_labels[group_index],
                                                              CORE_THEME_COLOR_TEXT_MUTED,
                                                              CORE_FONT_ROLE_UI_MEDIUM,
                                                              CORE_FONT_TEXT_SIZE_CAPTION);
                if (result.code != CORE_OK) {
                    (void)kit_ui_clip_pop(ui_ctx, frame);
                    return result;
                }
            }
            draw_rect.y += group_h;
            group_index += 1;
        }

        relationship_format_row_label(state, i, item);
        draw_rect.height = row_h;
        if (draw_rect.y + draw_rect.height >= content_viewport.y &&
            draw_rect.y <= content_viewport.y + content_viewport.height) {
            KitRenderRect nav_rect = draw_rect;
            KitRenderRect kind_rect;
            KitRenderRect delete_rect;
            KitUiButtonResult button;
            KitUiButtonResult kind_button;
            KitUiButtonResult delete_button;
            float edit_w = 44.0f;
            float edit_gap = 4.0f;

            if (draw_rect.width > 190.0f) {
                delete_rect = (KitRenderRect){
                    draw_rect.x + draw_rect.width - edit_w,
                    draw_rect.y,
                    edit_w,
                    draw_rect.height
                };
                kind_rect = (KitRenderRect){
                    delete_rect.x - edit_gap - edit_w,
                    draw_rect.y,
                    edit_w,
                    draw_rect.height
                };
                nav_rect.width = kind_rect.x - edit_gap - nav_rect.x;
                if (nav_rect.width < 60.0f) {
                    nav_rect.width = draw_rect.width;
                    kind_rect.width = 0.0f;
                    delete_rect.width = 0.0f;
                }
            } else {
                kind_rect = (KitRenderRect){0};
                delete_rect = (KitRenderRect){0};
            }

            button = kit_ui_eval_button(nav_rect,
                                        input,
                                        navigation_enabled);
            if (item->neighbor_item_id == state->selected_item_id) {
                button.state = KIT_UI_STATE_ACTIVE;
            }
            if (button.clicked && (!io_action || *io_action == MEM_CONSOLE_ACTION_NONE)) {
                mem_console_select_item_for_navigation(state,
                                                       item->neighbor_item_id,
                                                       1,
                                                       1,
                                                       io_action);
            }

            result = mem_console_ui_draw_button_custom(ui_ctx,
                                                       frame,
                                                       nav_rect,
                                                       state->detail_relationship_row_labels[i],
                                                       button.state,
                                                       CORE_FONT_ROLE_UI_REGULAR,
                                                       CORE_FONT_TEXT_SIZE_CAPTION);
            if (result.code != CORE_OK) {
                (void)kit_ui_clip_pop(ui_ctx, frame);
                return result;
            }
            if (kind_rect.width > 0.0f && delete_rect.width > 0.0f) {
                kind_button = kit_ui_eval_button(kind_rect, input, navigation_enabled);
                if (kind_button.clicked && io_action && *io_action == MEM_CONSOLE_ACTION_NONE) {
                    state->relationship_action_link_id = item->link_id;
                    *io_action = MEM_CONSOLE_ACTION_CYCLE_RELATIONSHIP_KIND;
                }
                result = mem_console_ui_draw_button_custom(ui_ctx,
                                                           frame,
                                                           kind_rect,
                                                           "KIND",
                                                           kind_button.state,
                                                           CORE_FONT_ROLE_UI_MEDIUM,
                                                           CORE_FONT_TEXT_SIZE_CAPTION);
                if (result.code != CORE_OK) {
                    (void)kit_ui_clip_pop(ui_ctx, frame);
                    return result;
                }

                delete_button = kit_ui_eval_button(delete_rect, input, navigation_enabled);
                if (delete_button.clicked && io_action && *io_action == MEM_CONSOLE_ACTION_NONE) {
                    state->relationship_action_link_id = item->link_id;
                    *io_action = MEM_CONSOLE_ACTION_REMOVE_RELATIONSHIP;
                }
                result = mem_console_ui_draw_button_custom(ui_ctx,
                                                           frame,
                                                           delete_rect,
                                                           "DEL",
                                                           delete_button.state,
                                                           CORE_FONT_ROLE_UI_MEDIUM,
                                                           CORE_FONT_TEXT_SIZE_CAPTION);
                if (result.code != CORE_OK) {
                    (void)kit_ui_clip_pop(ui_ctx, frame);
                    return result;
                }
            }
        }

        draw_rect.y += row_h;
        prev = item;
    }

    result = kit_ui_clip_pop(ui_ctx, frame);
    if (result.code != CORE_OK) {
        return result;
    }

    if (has_scrollbar) {
        result = kit_ui_draw_scrollbar(ui_ctx,
                                       frame,
                                       (KitRenderRect){
                                           panel.x + panel.width - 10.0f,
                                           content_viewport.y,
                                           8.0f,
                                           content_viewport.height
                                       },
                                       scroll,
                                       content_height);
        if (result.code != CORE_OK) {
            return result;
        }
    }

    return core_result_ok();
}
