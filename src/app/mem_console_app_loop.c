#include "mem_console_app_internal.h"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mem_console_prefs.h"
#include "mem_console_ui.h"

typedef enum MemConsoleInputRouteTargetPolicy {
    MEM_CONSOLE_INPUT_ROUTE_TARGET_FALLBACK = 0,
    MEM_CONSOLE_INPUT_ROUTE_TARGET_GLOBAL = 1,
    MEM_CONSOLE_INPUT_ROUTE_TARGET_UI = 2
} MemConsoleInputRouteTargetPolicy;

typedef enum MemConsoleInputInvalidateReasonBits {
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_QUIT = 1u << 0,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_WINDOW = 1u << 1,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_KEYBOARD = 1u << 2,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_POINTER = 1u << 3,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_WHEEL = 1u << 4,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_TEXT = 1u << 5
} MemConsoleInputInvalidateReasonBits;

typedef struct MemConsoleInputEventRaw {
    uint32_t sdl_event_count;
    uint32_t quit_event_count;
    uint32_t window_event_count;
    uint32_t keyboard_event_count;
    uint32_t pointer_event_count;
    uint32_t wheel_event_count;
    uint32_t text_event_count;
    uint32_t other_event_count;
    uint8_t quit_requested;
} MemConsoleInputEventRaw;

typedef struct MemConsoleInputEventNormalized {
    uint8_t has_quit_action;
    uint8_t has_window_action;
    uint8_t has_keyboard_action;
    uint8_t has_pointer_action;
    uint8_t has_wheel_action;
    uint8_t has_text_action;
    uint32_t action_count;
} MemConsoleInputEventNormalized;

typedef struct MemConsoleInputRouteResult {
    uint8_t consumed;
    MemConsoleInputRouteTargetPolicy target_policy;
    uint32_t routed_global_count;
    uint32_t routed_ui_count;
    uint32_t routed_fallback_count;
} MemConsoleInputRouteResult;

typedef struct MemConsoleInputInvalidationResult {
    uint8_t full_invalidate;
    uint32_t invalidation_reason_bits;
    uint32_t target_invalidation_count;
    uint32_t full_invalidation_count;
} MemConsoleInputInvalidationResult;

typedef struct MemConsoleInputFrame {
    MemConsoleInputEventRaw raw;
    MemConsoleInputRouteResult route;
    MemConsoleInputInvalidationResult invalidation;
} MemConsoleInputFrame;

typedef struct MemConsoleInputDiagTotals {
    uint64_t frame_count;
    uint64_t event_count_total;
    uint64_t routed_global_total;
    uint64_t routed_ui_total;
    uint64_t routed_fallback_total;
    uint64_t invalidation_reason_bits_total;
} MemConsoleInputDiagTotals;

static int mem_console_env_flag_enabled(const char *name) {
    const char *value = NULL;
    if (!name) {
        return 0;
    }
    value = getenv(name);
    if (!value || !value[0]) {
        return 0;
    }
    return strcmp(value, "1") == 0 ||
           strcmp(value, "true") == 0 ||
           strcmp(value, "TRUE") == 0 ||
           strcmp(value, "yes") == 0 ||
           strcmp(value, "on") == 0;
}

static int mem_console_ir1_diag_enabled(void) {
    return mem_console_env_flag_enabled("MEM_CONSOLE_IR1_DIAG");
}

static void mem_console_input_frame_begin(MemConsoleInputFrame *frame) {
    if (!frame) {
        return;
    }
    memset(frame, 0, sizeof(*frame));
}

static void mem_console_input_intake(const SDL_Event *event,
                                     MemConsoleInputEventRaw *out_raw) {
    if (!event || !out_raw) {
        return;
    }
    out_raw->sdl_event_count += 1u;
    switch (event->type) {
        case SDL_QUIT:
            out_raw->quit_event_count += 1u;
            out_raw->quit_requested = 1u;
            break;
        case SDL_WINDOWEVENT:
            out_raw->window_event_count += 1u;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            out_raw->keyboard_event_count += 1u;
            break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            out_raw->pointer_event_count += 1u;
            break;
        case SDL_MOUSEWHEEL:
            out_raw->wheel_event_count += 1u;
            break;
        case SDL_TEXTINPUT:
            out_raw->text_event_count += 1u;
            break;
        default:
            out_raw->other_event_count += 1u;
            break;
    }
}

static void mem_console_input_normalize(const SDL_Event *event,
                                        MemConsoleInputEventNormalized *out_normalized) {
    if (!event || !out_normalized) {
        return;
    }
    memset(out_normalized, 0, sizeof(*out_normalized));
    switch (event->type) {
        case SDL_QUIT:
            out_normalized->has_quit_action = 1u;
            out_normalized->action_count = 1u;
            break;
        case SDL_WINDOWEVENT:
            out_normalized->has_window_action = 1u;
            out_normalized->action_count = 1u;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            out_normalized->has_keyboard_action = 1u;
            out_normalized->action_count = 1u;
            break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            out_normalized->has_pointer_action = 1u;
            out_normalized->action_count = 1u;
            break;
        case SDL_MOUSEWHEEL:
            out_normalized->has_wheel_action = 1u;
            out_normalized->action_count = 1u;
            break;
        case SDL_TEXTINPUT:
            out_normalized->has_text_action = 1u;
            out_normalized->action_count = 1u;
            break;
        default:
            break;
    }
}

static void mem_console_input_route(const MemConsoleInputEventNormalized *normalized,
                                    MemConsoleInputRouteResult *out_route) {
    if (!normalized || !out_route) {
        return;
    }
    memset(out_route, 0, sizeof(*out_route));
    out_route->target_policy = MEM_CONSOLE_INPUT_ROUTE_TARGET_FALLBACK;

    if (normalized->has_quit_action ||
        normalized->has_window_action ||
        normalized->has_keyboard_action) {
        out_route->target_policy = MEM_CONSOLE_INPUT_ROUTE_TARGET_GLOBAL;
        out_route->routed_global_count = normalized->action_count;
        out_route->consumed = normalized->action_count > 0u;
        return;
    }

    if (normalized->has_pointer_action ||
        normalized->has_wheel_action ||
        normalized->has_text_action) {
        out_route->target_policy = MEM_CONSOLE_INPUT_ROUTE_TARGET_UI;
        out_route->routed_ui_count = normalized->action_count;
        out_route->consumed = normalized->action_count > 0u;
        return;
    }

    out_route->routed_fallback_count = normalized->action_count;
    out_route->consumed = normalized->action_count > 0u;
}

static void mem_console_input_invalidate(const MemConsoleInputEventNormalized *normalized,
                                         const MemConsoleInputRouteResult *route,
                                         MemConsoleInputInvalidationResult *out_invalidation) {
    if (!normalized || !route || !out_invalidation) {
        return;
    }
    memset(out_invalidation, 0, sizeof(*out_invalidation));

    if (normalized->has_quit_action) {
        out_invalidation->invalidation_reason_bits |= MEM_CONSOLE_INPUT_INVALIDATE_REASON_QUIT;
        out_invalidation->full_invalidation_count += 1u;
        out_invalidation->full_invalidate = 1u;
    }
    if (normalized->has_window_action) {
        out_invalidation->invalidation_reason_bits |= MEM_CONSOLE_INPUT_INVALIDATE_REASON_WINDOW;
        out_invalidation->full_invalidation_count += 1u;
        out_invalidation->full_invalidate = 1u;
    }
    if (normalized->has_keyboard_action) {
        out_invalidation->invalidation_reason_bits |= MEM_CONSOLE_INPUT_INVALIDATE_REASON_KEYBOARD;
    }
    if (normalized->has_pointer_action) {
        out_invalidation->invalidation_reason_bits |= MEM_CONSOLE_INPUT_INVALIDATE_REASON_POINTER;
    }
    if (normalized->has_wheel_action) {
        out_invalidation->invalidation_reason_bits |= MEM_CONSOLE_INPUT_INVALIDATE_REASON_WHEEL;
    }
    if (normalized->has_text_action) {
        out_invalidation->invalidation_reason_bits |= MEM_CONSOLE_INPUT_INVALIDATE_REASON_TEXT;
    }
    out_invalidation->target_invalidation_count += route->routed_global_count;
    out_invalidation->target_invalidation_count += route->routed_ui_count;
    out_invalidation->target_invalidation_count += route->routed_fallback_count;
}

static void mem_console_input_apply_event(MemConsoleInputFrame *frame,
                                          const SDL_Event *event) {
    MemConsoleInputEventNormalized normalized;
    MemConsoleInputRouteResult route;
    MemConsoleInputInvalidationResult invalidation;
    if (!frame || !event) {
        return;
    }
    mem_console_input_intake(event, &frame->raw);
    mem_console_input_normalize(event, &normalized);
    mem_console_input_route(&normalized, &route);
    mem_console_input_invalidate(&normalized, &route, &invalidation);

    frame->route.routed_global_count += route.routed_global_count;
    frame->route.routed_ui_count += route.routed_ui_count;
    frame->route.routed_fallback_count += route.routed_fallback_count;
    if (route.consumed) {
        frame->route.consumed = 1u;
        frame->route.target_policy = route.target_policy;
    }

    frame->invalidation.invalidation_reason_bits |= invalidation.invalidation_reason_bits;
    frame->invalidation.target_invalidation_count += invalidation.target_invalidation_count;
    frame->invalidation.full_invalidation_count += invalidation.full_invalidation_count;
    if (invalidation.full_invalidate) {
        frame->invalidation.full_invalidate = 1u;
    }
}

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
    MemConsoleInputDiagTotals ir1_diag_totals = {0};

    if (!ctx || !ctx->window || !ctx->renderer || !ctx->render_ctx || !ctx->ui_ctx ||
        !ctx->db || !ctx->state || !ctx->runtime || !ctx->kernel_bridge ||
        !ctx->prefs_path_valid || !ctx->prefs_signature_valid || !ctx->prefs_last_saved_signature ||
        !ctx->app_prefs_path || !ctx->prefs_path || ctx->prefs_path_cap == 0u) {
        return 1;
    }

    while (running) {
        MemConsoleInputFrame input_frame;
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

        mem_console_input_frame_begin(&input_frame);
        input.mouse_pressed = 0;
        input.mouse_released = 0;

        idle_wait_ms = mem_console_runtime_idle_wait_ms(ctx->runtime, ctx->state, SDL_GetTicks64());
        if (idle_wait_ms > 0u) {
            if (SDL_WaitEventTimeout(&event, (int)idle_wait_ms)) {
                waited_event = 1;
                mem_console_input_apply_event(&input_frame, &event);
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
            mem_console_input_apply_event(&input_frame, &event);
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

        ir1_diag_totals.frame_count += 1u;
        ir1_diag_totals.event_count_total += input_frame.raw.sdl_event_count;
        ir1_diag_totals.routed_global_total += input_frame.route.routed_global_count;
        ir1_diag_totals.routed_ui_total += input_frame.route.routed_ui_count;
        ir1_diag_totals.routed_fallback_total += input_frame.route.routed_fallback_count;
        ir1_diag_totals.invalidation_reason_bits_total += input_frame.invalidation.invalidation_reason_bits;
        if (mem_console_ir1_diag_enabled()) {
            printf("[ir1] mem_console frame=%llu events=%u route(global=%u ui=%u fallback=%u target=%d) "
                   "invalidate(bits=0x%x target=%u full=%u) totals(frames=%llu events=%llu global=%llu ui=%llu fallback=%llu invalid_bits_sum=%llu)\n",
                   (unsigned long long)ir1_diag_totals.frame_count,
                   (unsigned int)input_frame.raw.sdl_event_count,
                   (unsigned int)input_frame.route.routed_global_count,
                   (unsigned int)input_frame.route.routed_ui_count,
                   (unsigned int)input_frame.route.routed_fallback_count,
                   (int)input_frame.route.target_policy,
                   (unsigned int)input_frame.invalidation.invalidation_reason_bits,
                   (unsigned int)input_frame.invalidation.target_invalidation_count,
                   (unsigned int)input_frame.invalidation.full_invalidation_count,
                   (unsigned long long)ir1_diag_totals.frame_count,
                   (unsigned long long)ir1_diag_totals.event_count_total,
                   (unsigned long long)ir1_diag_totals.routed_global_total,
                   (unsigned long long)ir1_diag_totals.routed_ui_total,
                   (unsigned long long)ir1_diag_totals.routed_fallback_total,
                   (unsigned long long)ir1_diag_totals.invalidation_reason_bits_total);
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
                                                      ctx->render_ctx,
                                                      ctx->ui_ctx,
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
