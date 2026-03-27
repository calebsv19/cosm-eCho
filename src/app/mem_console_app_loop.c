#include "mem_console_app_internal.h"

#include <SDL2/SDL.h>
#include <stdio.h>

#include "mem_console_prefs.h"
#include "mem_console_ui.h"

static int mem_console_app_recreate_swapchain_and_mark(VkRenderer *renderer,
                                                       SDL_Window *window,
                                                       MemConsoleState *state,
                                                       const char *reason) {
    VkResult vk_result;

    if (!renderer || !window || !state) {
        return 0;
    }

    vk_result = vk_renderer_recreate_swapchain(renderer, window);
    if (vk_result != VK_SUCCESS) {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "%s (vk=%d).",
                       reason ? reason : "Swapchain recreate failed",
                       (int)vk_result);
        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        return 0;
    }

    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_LAYOUT | MEM_CONSOLE_REDRAW_REASON_BACKGROUND);
    return 1;
}

int mem_console_app_run_loop(MemConsoleAppLoopContext *ctx) {
    SDL_Event event;
    bool running = true;
    int frame_width = 0;
    int frame_height = 0;
    int last_frame_width = -1;
    int last_frame_height = -1;
    KitUiInputState input = {0};

    if (!ctx || !ctx->window || !ctx->renderer || !ctx->render_ctx || !ctx->ui_ctx ||
        !ctx->db || !ctx->state || !ctx->runtime || !ctx->kernel_bridge ||
        !ctx->prefs_path_valid || !ctx->prefs_signature_valid || !ctx->prefs_last_saved_signature ||
        !ctx->app_prefs_path || !ctx->prefs_path || ctx->prefs_path_cap == 0u) {
        return 1;
    }

    while (running) {
        MemConsoleAction keyboard_action = MEM_CONSOLE_ACTION_NONE;
        MemConsoleAction ui_action = MEM_CONSOLE_ACTION_NONE;
        MemConsoleAction pending_action = MEM_CONSOLE_ACTION_NONE;
        int frame_result = MEM_CONSOLE_FRAME_OK;
        uint32_t frame_reasons = 0u;
        uint32_t idle_wait_ms = 0u;
        uint64_t runtime_applied_before;
        uint64_t runtime_dropped_before;
        uint64_t runtime_errors_before;
        uint64_t runtime_coalesced_before;
        int runtime_in_flight_before;
        int runtime_pending_before;
        int waited_event = 0;
        int wheel_y = 0;

        input.mouse_pressed = 0;
        input.mouse_released = 0;

        idle_wait_ms = mem_console_runtime_idle_wait_ms(ctx->runtime, ctx->state, SDL_GetTicks64());
        if (idle_wait_ms > 0u) {
            if (SDL_WaitEventTimeout(&event, (int)idle_wait_ms)) {
                waited_event = 1;
                mem_console_app_process_sdl_event(&event,
                                                  &running,
                                                  ctx->render_ctx,
                                                  ctx->ui_ctx,
                                                  ctx->state,
                                                  (*ctx->prefs_path_valid) ? ctx->prefs_path : "",
                                                  &input,
                                                  &wheel_y,
                                                  &keyboard_action);
            }
        }

        if (!running) {
            break;
        }

        while (SDL_PollEvent(&event)) {
            mem_console_app_process_sdl_event(&event,
                                              &running,
                                              ctx->render_ctx,
                                              ctx->ui_ctx,
                                              ctx->state,
                                              (*ctx->prefs_path_valid) ? ctx->prefs_path : "",
                                              &input,
                                              &wheel_y,
                                              &keyboard_action);
        }

        if (!running) {
            break;
        }

        if (ctx->state->search_refresh_pending) {
            Uint64 now_ms = SDL_GetTicks64();
            if (now_ms >= ctx->state->search_last_input_ms &&
                (now_ms - ctx->state->search_last_input_ms) >= 150u) {
                ctx->state->search_refresh_pending = 0;
                mem_console_app_refresh_and_report(ctx->db, ctx->state, "Search refresh failed");
            }
        }

        runtime_applied_before = ctx->state->runtime_refresh_applied;
        runtime_dropped_before = ctx->state->runtime_refresh_dropped;
        runtime_errors_before = ctx->state->runtime_refresh_errors;
        runtime_coalesced_before = ctx->state->runtime_refresh_coalesced;
        runtime_in_flight_before = ctx->state->runtime_refresh_in_flight;
        runtime_pending_before = ctx->state->runtime_pending_intent;
        mem_console_runtime_tick(ctx->runtime, ctx->state, SDL_GetTicks64());
        if (ctx->state->runtime_refresh_applied != runtime_applied_before ||
            ctx->state->runtime_refresh_dropped != runtime_dropped_before ||
            ctx->state->runtime_refresh_errors != runtime_errors_before ||
            ctx->state->runtime_refresh_coalesced != runtime_coalesced_before ||
            ctx->state->runtime_refresh_in_flight != runtime_in_flight_before ||
            ctx->state->runtime_pending_intent != runtime_pending_before) {
            mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        }
        if (ctx->kernel_bridge_requested) {
            uint64_t now_ns = (uint64_t)SDL_GetTicks64() * 1000000ULL;
            mem_console_kernel_bridge_tick(ctx->kernel_bridge, ctx->state, now_ns);
            if (ctx->state->kernel_last_render_requested) {
                mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_BACKGROUND);
            }
        }

        SDL_GetWindowSize(ctx->window, &frame_width, &frame_height);
        if (frame_width != last_frame_width || frame_height != last_frame_height) {
            (void)mem_console_app_recreate_swapchain_and_mark(ctx->renderer,
                                                              ctx->window,
                                                              ctx->state,
                                                              "Swapchain refresh failed after resize");
            mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_LAYOUT);
            last_frame_width = frame_width;
            last_frame_height = frame_height;
        }

        frame_reasons = mem_console_redraw_pending(ctx->state);
        if (frame_reasons != 0u) {
            frame_reasons = mem_console_redraw_take_pending(ctx->state);
            frame_result = run_frame(ctx->render_ctx,
                                     ctx->ui_ctx,
                                     ctx->state,
                                     &input,
                                     frame_width,
                                     frame_height,
                                     wheel_y,
                                     &ui_action);
            if (frame_result == MEM_CONSOLE_FRAME_RECOVERABLE) {
                if (!mem_console_app_recreate_swapchain_and_mark(ctx->renderer,
                                                                 ctx->window,
                                                                 ctx->state,
                                                                 "Swapchain recover failed")) {
                    return 1;
                }
                continue;
            }
            if (frame_result == MEM_CONSOLE_FRAME_FATAL) {
                return 1;
            }
            mem_console_redraw_note_frame(ctx->state, frame_reasons, SDL_GetTicks64());
        }

        if (keyboard_action != MEM_CONSOLE_ACTION_NONE) {
            pending_action = keyboard_action;
        } else {
            pending_action = ui_action;
        }
        mem_console_app_apply_pending_action(ctx->db, ctx->state, ctx->runtime, pending_action);

        if (ctx->state->pending_db_path[0] != '\0') {
            CoreResult result;
            char next_db_path[1024];

            (void)snprintf(next_db_path, sizeof(next_db_path), "%s", ctx->state->pending_db_path);
            ctx->state->pending_db_path[0] = '\0';
            result = mem_console_app_switch_active_db(ctx->db,
                                                      ctx->state,
                                                      next_db_path,
                                                      ctx->app_prefs_path,
                                                      ctx->app_prefs_path_valid,
                                                      ctx->prefs_path,
                                                      ctx->prefs_path_cap,
                                                      ctx->prefs_path_valid,
                                                      ctx->prefs_signature_valid,
                                                      ctx->prefs_last_saved_signature);
            if (result.code != CORE_OK) {
                (void)snprintf(ctx->state->status_line,
                               sizeof(ctx->state->status_line),
                               "DB switch failed: %s",
                               result.message ? result.message : "error");
                mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            }
        }

        if (*ctx->prefs_path_valid) {
            uint64_t current_signature = mem_console_prefs_state_signature(ctx->state);
            if (!*ctx->prefs_signature_valid) {
                *ctx->prefs_last_saved_signature = current_signature;
                *ctx->prefs_signature_valid = 1;
            } else if (current_signature != *ctx->prefs_last_saved_signature) {
                ctx->state->pane_prefs_dirty = 1;
            }
        }

        if (*ctx->prefs_path_valid &&
            ctx->state->pane_prefs_dirty &&
            !ctx->state->pane_drag_active &&
            !ctx->state->left_panel_drag_active &&
            !ctx->state->search_refresh_pending) {
            CoreResult result = mem_console_prefs_save(ctx->prefs_path, ctx->state);
            if (result.code == CORE_OK) {
                ctx->state->pane_prefs_dirty = 0;
                *ctx->prefs_last_saved_signature = mem_console_prefs_state_signature(ctx->state);
                *ctx->prefs_signature_valid = 1;
            } else {
                (void)snprintf(ctx->state->status_line,
                               sizeof(ctx->state->status_line),
                               "Pane prefs save failed.");
                mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            }
        }

        if (waited_event == 0 &&
            pending_action == MEM_CONSOLE_ACTION_NONE &&
            mem_console_redraw_pending(ctx->state) == 0u) {
            SDL_Delay(1u);
        }
    }

    return 0;
}
