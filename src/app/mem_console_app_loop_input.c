#include "mem_console_app_loop_internal.h"

#include <stdio.h>
#include <string.h>

static int mem_console_ir1_diag_enabled(void) {
    return mem_console_app_loop_env_flag_enabled("MEM_CONSOLE_IR1_DIAG");
}

void mem_console_input_frame_begin(MemConsoleInputFrame *frame) {
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

static void mem_console_loop_wait_input_phase(const MemConsoleAppLoopContext *ctx,
                                              MemConsoleLoopRunState *run_state,
                                              MemConsoleLoopFrameState *frame) {
    if (!ctx || !run_state || !frame) {
        return;
    }

    frame->idle_wait_ms = mem_console_runtime_idle_wait_ms(ctx->runtime, ctx->state, SDL_GetTicks64());
    if (frame->idle_wait_ms > 0u) {
        uint64_t wait_begin_ms = SDL_GetTicks64();
        uint64_t wait_end_ms = 0u;
        uint64_t wait_elapsed_ms = 0u;

        frame->wait_performed = 1;
        frame->wait_call_count = 1u;
        if (SDL_WaitEventTimeout(&frame->event, (int)frame->idle_wait_ms)) {
            frame->waited_event = 1;
            mem_console_input_apply_event(&frame->input_frame, &frame->event);
            mem_console_app_process_sdl_event(&frame->event,
                                              &run_state->running,
                                              ctx->render_ctx,
                                              ctx->ui_ctx,
                                              ctx->state,
                                              (*ctx->prefs_path_valid) ? ctx->prefs_path : "",
                                              &run_state->input,
                                              &frame->wheel_y,
                                              &frame->keyboard_action);
        }
        wait_end_ms = SDL_GetTicks64();
        if (wait_end_ms > wait_begin_ms) {
            wait_elapsed_ms = wait_end_ms - wait_begin_ms;
        }
        if (wait_elapsed_ms > (uint64_t)UINT32_MAX) {
            frame->wait_blocked_ms = UINT32_MAX;
        } else {
            frame->wait_blocked_ms = (uint32_t)wait_elapsed_ms;
        }
    }
}

static void mem_console_loop_poll_input_phase(const MemConsoleAppLoopContext *ctx,
                                              MemConsoleLoopRunState *run_state,
                                              MemConsoleLoopFrameState *frame) {
    if (!ctx || !run_state || !frame) {
        return;
    }
    while (SDL_PollEvent(&frame->event)) {
        mem_console_input_apply_event(&frame->input_frame, &frame->event);
        mem_console_app_process_sdl_event(&frame->event,
                                          &run_state->running,
                                          ctx->render_ctx,
                                          ctx->ui_ctx,
                                          ctx->state,
                                          (*ctx->prefs_path_valid) ? ctx->prefs_path : "",
                                          &run_state->input,
                                          &frame->wheel_y,
                                          &frame->keyboard_action);
    }
}

static void mem_console_loop_note_input_diag(MemConsoleLoopRunState *run_state,
                                             const MemConsoleLoopFrameState *frame) {
    if (!run_state || !frame) {
        return;
    }
    run_state->ir1_diag_totals.frame_count += 1u;
    run_state->ir1_diag_totals.event_count_total += frame->input_frame.raw.sdl_event_count;
    run_state->ir1_diag_totals.routed_global_total += frame->input_frame.route.routed_global_count;
    run_state->ir1_diag_totals.routed_ui_total += frame->input_frame.route.routed_ui_count;
    run_state->ir1_diag_totals.routed_fallback_total += frame->input_frame.route.routed_fallback_count;
    run_state->ir1_diag_totals.invalidation_reason_bits_total += frame->input_frame.invalidation.invalidation_reason_bits;
    if (mem_console_ir1_diag_enabled()) {
        printf("[ir1] mem_console frame=%llu events=%u route(global=%u ui=%u fallback=%u target=%d) "
               "invalidate(bits=0x%x target=%u full=%u) totals(frames=%llu events=%llu global=%llu ui=%llu fallback=%llu invalid_bits_sum=%llu)\n",
               (unsigned long long)run_state->ir1_diag_totals.frame_count,
               (unsigned int)frame->input_frame.raw.sdl_event_count,
               (unsigned int)frame->input_frame.route.routed_global_count,
               (unsigned int)frame->input_frame.route.routed_ui_count,
               (unsigned int)frame->input_frame.route.routed_fallback_count,
               (int)frame->input_frame.route.target_policy,
               (unsigned int)frame->input_frame.invalidation.invalidation_reason_bits,
               (unsigned int)frame->input_frame.invalidation.target_invalidation_count,
               (unsigned int)frame->input_frame.invalidation.full_invalidation_count,
               (unsigned long long)run_state->ir1_diag_totals.frame_count,
               (unsigned long long)run_state->ir1_diag_totals.event_count_total,
               (unsigned long long)run_state->ir1_diag_totals.routed_global_total,
               (unsigned long long)run_state->ir1_diag_totals.routed_ui_total,
               (unsigned long long)run_state->ir1_diag_totals.routed_fallback_total,
               (unsigned long long)run_state->ir1_diag_totals.invalidation_reason_bits_total);
    }
}

MemConsoleLoopInputPhaseResult mem_console_loop_input_phase(const MemConsoleAppLoopContext *ctx,
                                                           MemConsoleLoopRunState *run_state,
                                                           MemConsoleLoopFrameState *frame) {
    if (!ctx || !run_state || !frame) {
        return MEM_CONSOLE_LOOP_INPUT_PHASE_EXIT;
    }
    mem_console_loop_wait_input_phase(ctx, run_state, frame);
    if (!run_state->running) {
        return MEM_CONSOLE_LOOP_INPUT_PHASE_EXIT;
    }
    mem_console_loop_poll_input_phase(ctx, run_state, frame);
    if (!run_state->running) {
        return MEM_CONSOLE_LOOP_INPUT_PHASE_EXIT;
    }
    mem_console_loop_note_input_diag(run_state, frame);
    return MEM_CONSOLE_LOOP_INPUT_PHASE_OK;
}
