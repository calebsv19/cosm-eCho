#include "mem_console_ui.h"
#include "mem_console_layout_config.h"
#include "mem_console_pane_layout.h"
#include "mem_console_ui_common.h"
#include "mem_console_ui_chrome.h"
#include "mem_console_ui_detail_section.h"
#include "mem_console_ui_graph_controls.h"
#include "mem_console_ui_graph_panel.h"
#include "mem_console_ui_left_section.h"

#include <stdio.h>

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
    const MemConsoleLayoutConfig *layout_cfg;

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

    if (input->mouse_pressed) {
        (void)mem_console_pane_layout_begin_drag(state,
                                                 layout_cfg,
                                                 draw_width,
                                                 draw_height,
                                                 input->mouse_x,
                                                 input->mouse_y);
    }
    if (state->pane_drag_active && input->mouse_down) {
        (void)mem_console_pane_layout_update_drag(state,
                                                  layout_cfg,
                                                  draw_width,
                                                  draw_height,
                                                  input->mouse_x,
                                                  input->mouse_y);
    }
    if (state->pane_drag_active && input->mouse_released) {
        mem_console_pane_layout_end_drag(state);
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
        return 1;
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

    has_any_edit_mode = state->title_edit_mode || state->body_edit_mode;

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
                                              input,
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
                                                input,
                                                wheel_y,
                                                layout_cfg,
                                                &right_layout);
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
                                                input,
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
                                             input,
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

    result = kit_render_end_frame(render_ctx, &frame);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: kit_render_end_frame failed: %d\n", (int)result.code);
        return 1;
    }

    return 0;
}
