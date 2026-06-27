#include "mem_console_ui_graph_panel.h"

#include "mem_console_ui_common.h"
#include "mem_console_ui_graph_internal.h"

#include <SDL2/SDL.h>
#include <string.h>

static int graph_panel_is_anchor_stable_id(const char *stable_id) {
    if (!stable_id || !stable_id[0]) {
        return 0;
    }
    return strncmp(stable_id, "scope-", 6u) == 0 ||
           strncmp(stable_id, "plans-", 6u) == 0 ||
           strncmp(stable_id, "decisions-", 10u) == 0 ||
           strncmp(stable_id, "issues-", 7u) == 0 ||
           strncmp(stable_id, "misc-", 5u) == 0;
}

static int handle_graph_node_click(MemConsoleState *state,
                                   int64_t hit_item_id,
                                   MemConsoleAction *io_action) {
    uint64_t now_ms;
    int is_double_click = 0;
    int i;
    int hit_node_index = -1;
    int hit_node_is_anchor = 0;

    if (!state || !io_action || hit_item_id == 0) {
        return 0;
    }
    for (i = 0; i < state->graph_node_count; ++i) {
        if (state->graph_nodes[i].item_id == hit_item_id) {
            hit_node_index = i;
            hit_node_is_anchor = graph_panel_is_anchor_stable_id(state->graph_nodes[i].stable_id);
            break;
        }
    }

    now_ms = SDL_GetTicks64();
    if (state->graph_last_click_item_id == hit_item_id &&
        now_ms >= state->graph_last_click_ms &&
        (now_ms - state->graph_last_click_ms) <= 300u) {
        is_double_click = 1;
    }

    state->graph_last_click_item_id = hit_item_id;
    state->graph_last_click_ms = now_ms;

    if (is_double_click) {
        state->list_last_click_item_id = 0;
        state->list_last_click_ms = 0u;
        mem_console_select_item_for_navigation(state, hit_item_id, 1, 1, io_action);
        return 1;
    }

    if (hit_node_index >= 0 && hit_node_is_anchor) {
        int now_hidden = 0;
        int changed = mem_console_graph_anchor_hidden_toggle(state, hit_item_id, &now_hidden);
        if (changed) {
            state->graph_layout_valid = 0;
            mem_console_pane_prefs_mark_dirty(state);
            graph_status_format_anchor_visibility_line(state,
                                                       &state->graph_nodes[hit_node_index],
                                                       now_hidden);
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_LAYOUT | MEM_CONSOLE_REDRAW_REASON_CONTENT);
            if (*io_action == MEM_CONSOLE_ACTION_NONE) {
                *io_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
            }
        }
        return 1;
    }

    if (hit_item_id != state->selected_item_id) {
        state->list_last_click_item_id = 0;
        state->list_last_click_ms = 0u;
        mem_console_select_item_for_inspection(state, hit_item_id, 1, io_action);
        return 1;
    }

    return 0;
}

CoreResult mem_console_ui_draw_graph_panel(KitRenderContext *render_ctx,
                                           KitUiContext *ui_ctx,
                                           KitRenderFrame *frame,
                                           MemConsoleState *state,
                                           const KitUiInputState *input,
                                           const MemConsoleLayoutConfig *layout_cfg,
                                           KitUiStackLayout *right_layout,
                                           int wheel_y,
                                           MemConsoleAction *io_action) {
    CoreResult result;
    KitRenderRect graph_panel;
    KitRenderRect graph_view;
    int suppress_graph_click_on_release = 0;
    int legend_click_consumed = 0;
    int graph_filter_changed = 0;

    if (!render_ctx || !ui_ctx || !frame || !state || !input || !layout_cfg || !right_layout || !io_action) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid graph panel draw request" };
    }
    (void)right_layout;

    graph_panel = state->pane_right_graph;
    if (graph_panel.width <= 1.0f || graph_panel.height <= 1.0f) {
        return core_result_ok();
    }

    graph_view = (KitRenderRect){
        graph_panel.x + 1.0f,
        graph_panel.y + 1.0f,
        graph_panel.width - 2.0f,
        graph_panel.height - 2.0f
    };
    if (graph_view.width <= 2.0f || graph_view.height <= 2.0f) {
        return core_result_ok();
    }
    if (graph_view.width <= 4.0f || graph_view.height <= 4.0f) {
        return core_result_ok();
    }

    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             graph_view,
                                             0.0f,
                                             CORE_THEME_COLOR_SURFACE_0);
    if (result.code != CORE_OK) {
        return result;
    }

    state->graph_mode_enabled = 1;
    {
        KitRenderRect graph_bounds = {
            graph_view.x + 6.0f,
            graph_view.y + 6.0f,
            graph_view.width - 12.0f,
            graph_view.height - 12.0f
        };
        if (graph_bounds.width <= 4.0f || graph_bounds.height <= 4.0f) {
            return core_result_ok();
        }

        suppress_graph_click_on_release = mem_console_ui_graph_handle_viewport_interaction(state,
                                                                                            input,
                                                                                            wheel_y,
                                                                                            graph_bounds);

        result = kit_ui_clip_push(ui_ctx, frame, graph_bounds);
        if (result.code != CORE_OK) {
            return result;
        }

        result = mem_console_ui_graph_draw_preview(render_ctx,
                                                   ui_ctx,
                                                   input,
                                                   frame,
                                                   graph_bounds,
                                                   state,
                                                   &legend_click_consumed,
                                                   &graph_filter_changed);
        if (result.code != CORE_OK) {
            (void)kit_ui_clip_pop(ui_ctx, frame);
            return result;
        }

        result = kit_ui_clip_pop(ui_ctx, frame);
        if (result.code != CORE_OK) {
            return result;
        }

        if (graph_filter_changed) {
            state->graph_layout_valid = 0;
            mem_console_redraw_mark(state,
                                    MEM_CONSOLE_REDRAW_REASON_LAYOUT |
                                        MEM_CONSOLE_REDRAW_REASON_CONTENT);
        }

        if (input->mouse_released &&
            state->graph_click_armed &&
            !suppress_graph_click_on_release &&
            !legend_click_consumed) {
            if (state->graph_node_count > 0 &&
                kit_ui_point_in_rect(graph_bounds, input->mouse_x, input->mouse_y)) {
                KitGraphStructHit hit = {0};
                uint32_t rect_hit_index = 0u;
                int node_selected = 0;
                int64_t next_item_id = 0;

                result = mem_console_ui_graph_ensure_layout_cache(render_ctx, state, graph_bounds);
                if (result.code == CORE_OK &&
                    state->graph_layout_has_graph_data &&
                    state->graph_layout_node_count > 0u) {
                    if (mem_console_ui_graph_find_node_index_at_point(state,
                                                                      input->mouse_x,
                                                                      input->mouse_y,
                                                                      &rect_hit_index) &&
                        rect_hit_index < (uint32_t)state->graph_node_count) {
                        int64_t hit_item_id = state->graph_nodes[rect_hit_index].item_id;
                        node_selected = handle_graph_node_click(state, hit_item_id, io_action);
                    }
                    if (!node_selected) {
                        result = kit_graph_struct_hit_test(state->graph_layout_node_layouts,
                                                           state->graph_layout_node_count,
                                                           input->mouse_x,
                                                           input->mouse_y,
                                                           &hit);
                        if (result.code == CORE_OK &&
                            hit.active &&
                            hit.node_index < (uint32_t)state->graph_node_count) {
                            int64_t hit_item_id = state->graph_nodes[hit.node_index].item_id;
                            node_selected = handle_graph_node_click(state, hit_item_id, io_action);
                        } else {
                            result = core_result_ok();
                        }
                    }
                    if (!node_selected &&
                        mem_console_ui_graph_select_neighbor_from_edge_click(state,
                                                                             input->mouse_x,
                                                                             input->mouse_y,
                                                                             &next_item_id)) {
                        state->list_last_click_item_id = 0;
                        state->list_last_click_ms = 0u;
                        mem_console_select_item_for_inspection(state, next_item_id, 1, io_action);
                    }
                } else if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
        if (input->mouse_released) {
            state->graph_click_armed = 0;
        }
        return core_result_ok();
    }
}
