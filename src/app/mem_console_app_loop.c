#include "mem_console_app_loop_internal.h"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mem_console_prefs.h"
#include "mem_console_ui.h"

typedef struct MemConsoleRenderDeriveFrame {
    KitUiInputState input;
    int frame_width;
    int frame_height;
    int wheel_y;
    uint32_t frame_reasons;
} MemConsoleRenderDeriveFrame;

typedef struct MemConsoleRenderSubmitOutcome {
    int frame_result;
    MemConsoleAction ui_action;
} MemConsoleRenderSubmitOutcome;

typedef struct MemConsoleLoopDiagState {
    int initialized;
    int enabled;
    int json_output;
    uint64_t period_start_ms;
    uint64_t frames;
    uint64_t wait_calls;
    uint64_t blocked_ms;
    uint64_t active_ms;
} MemConsoleLoopDiagState;

static MemConsoleLoopDiagState s_mem_console_loop_diag = {0};

int mem_console_app_loop_env_flag_enabled(const char *name) {
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

static int mem_console_rs1_diag_enabled(void) {
    return mem_console_app_loop_env_flag_enabled("MEM_CONSOLE_RS1_DIAG");
}

static void mem_console_loop_diag_init_once(void) {
    const char *format_env = NULL;
    if (s_mem_console_loop_diag.initialized) {
        return;
    }

    if (mem_console_app_loop_env_flag_enabled("MEM_CONSOLE_LOOP_DIAG_LOG") ||
        mem_console_app_loop_env_flag_enabled("MEM_CONSOLE_LOOP_DIAG_JSON")) {
        s_mem_console_loop_diag.enabled = 1;
    }

    format_env = getenv("MEM_CONSOLE_LOOP_DIAG_FORMAT");
    if (format_env && format_env[0] && strcmp(format_env, "json") == 0) {
        s_mem_console_loop_diag.enabled = 1;
        s_mem_console_loop_diag.json_output = 1;
    }
    if (mem_console_app_loop_env_flag_enabled("MEM_CONSOLE_LOOP_DIAG_JSON")) {
        s_mem_console_loop_diag.enabled = 1;
        s_mem_console_loop_diag.json_output = 1;
    }
    s_mem_console_loop_diag.initialized = 1;
}

static void mem_console_loop_diag_emit(uint64_t period_ms) {
    uint64_t total_ms = s_mem_console_loop_diag.blocked_ms + s_mem_console_loop_diag.active_ms;
    double blocked_pct = (total_ms > 0u)
                             ? (100.0 * (double)s_mem_console_loop_diag.blocked_ms / (double)total_ms)
                             : 0.0;
    double active_pct = (total_ms > 0u)
                            ? (100.0 * (double)s_mem_console_loop_diag.active_ms / (double)total_ms)
                            : 0.0;

    if (s_mem_console_loop_diag.json_output) {
        printf("{\"tag\":\"LoopDiag\",\"schema\":1,\"period_ms\":%llu,\"frames\":%llu,"
               "\"wait_calls\":%llu,\"blocked_ms\":%llu,\"blocked_pct\":%.1f,"
               "\"active_ms\":%llu,\"active_pct\":%.1f,\"wakes\":0,\"timers\":0,"
               "\"results_queue\":{\"last\":0,\"peak\":0},"
               "\"jobs\":{\"scheduled\":0,\"coalesced\":0},"
               "\"results\":{\"applied\":0,\"stale_dropped\":0},"
               "\"edit_txn\":{\"starts\":0,\"commits\":0,\"debounce_commits\":0,\"boundary_commits\":0},"
               "\"events\":{\"queue_last\":0,\"queue_peak\":0,\"enqueued\":0,\"processed\":0,\"deferred\":0,\"dropped\":0,"
               "\"emit\":{\"symbols\":0,\"diagnostics\":0,\"analysis_progress\":0,\"analysis_status\":0,\"library_index\":0,\"analysis_finished\":0},"
               "\"dispatch\":{\"symbols\":0,\"diagnostics\":0,\"analysis_progress\":0,\"analysis_status\":0,\"library_index\":0,\"analysis_finished\":0}},"
               "\"stale_by_kind\":{\"symbols\":0,\"diagnostics\":0,\"analysis_progress\":0,\"analysis_status\":0,\"analysis_finished\":0}}\n",
               (unsigned long long)period_ms,
               (unsigned long long)s_mem_console_loop_diag.frames,
               (unsigned long long)s_mem_console_loop_diag.wait_calls,
               (unsigned long long)s_mem_console_loop_diag.blocked_ms,
               blocked_pct,
               (unsigned long long)s_mem_console_loop_diag.active_ms,
               active_pct);
    } else {
        printf("[LoopDiag] period=%llums frames=%llu waits=%llu blocked=%llums(%.1f%%) active=%llums(%.1f%%)\n",
               (unsigned long long)period_ms,
               (unsigned long long)s_mem_console_loop_diag.frames,
               (unsigned long long)s_mem_console_loop_diag.wait_calls,
               (unsigned long long)s_mem_console_loop_diag.blocked_ms,
               blocked_pct,
               (unsigned long long)s_mem_console_loop_diag.active_ms,
               active_pct);
    }
}

static void mem_console_loop_diag_tick(uint64_t frame_begin_ms,
                                       uint64_t frame_end_ms,
                                       uint32_t wait_call_count,
                                       uint32_t wait_blocked_ms) {
    uint64_t frame_elapsed_ms = 0u;
    uint64_t blocked_ms = 0u;
    uint64_t active_ms = 0u;
    uint64_t elapsed_ms = 0u;

    mem_console_loop_diag_init_once();
    if (!s_mem_console_loop_diag.enabled) {
        return;
    }
    if (frame_end_ms <= frame_begin_ms) {
        return;
    }

    if (s_mem_console_loop_diag.period_start_ms == 0u) {
        s_mem_console_loop_diag.period_start_ms = frame_begin_ms;
    }

    frame_elapsed_ms = frame_end_ms - frame_begin_ms;
    blocked_ms = wait_blocked_ms;
    if (blocked_ms > frame_elapsed_ms) {
        blocked_ms = frame_elapsed_ms;
    }
    active_ms = frame_elapsed_ms - blocked_ms;

    s_mem_console_loop_diag.frames += 1u;
    s_mem_console_loop_diag.wait_calls += (uint64_t)wait_call_count;
    s_mem_console_loop_diag.blocked_ms += blocked_ms;
    s_mem_console_loop_diag.active_ms += active_ms;

    if (frame_end_ms < s_mem_console_loop_diag.period_start_ms) {
        s_mem_console_loop_diag.period_start_ms = frame_end_ms;
        return;
    }
    elapsed_ms = frame_end_ms - s_mem_console_loop_diag.period_start_ms;
    if (elapsed_ms < 1000u) {
        return;
    }

    mem_console_loop_diag_emit(elapsed_ms);
    s_mem_console_loop_diag.period_start_ms = frame_end_ms;
    s_mem_console_loop_diag.frames = 0u;
    s_mem_console_loop_diag.wait_calls = 0u;
    s_mem_console_loop_diag.blocked_ms = 0u;
    s_mem_console_loop_diag.active_ms = 0u;
}

static void mem_console_render_derive_frame(MemConsoleRenderDeriveFrame *out_derive,
                                            const KitUiInputState *input,
                                            int frame_width,
                                            int frame_height,
                                            int wheel_y,
                                            uint32_t frame_reasons) {
    if (!out_derive || !input) {
        return;
    }
    memset(out_derive, 0, sizeof(*out_derive));
    out_derive->input = *input;
    out_derive->frame_width = frame_width;
    out_derive->frame_height = frame_height;
    out_derive->wheel_y = wheel_y;
    out_derive->frame_reasons = frame_reasons;
}

static void mem_console_render_submit_frame(const MemConsoleAppLoopContext *ctx,
                                            const MemConsoleRenderDeriveFrame *derive,
                                            MemConsoleRenderSubmitOutcome *out_submit) {
    if (!out_submit) {
        return;
    }
    memset(out_submit, 0, sizeof(*out_submit));
    out_submit->ui_action = MEM_CONSOLE_ACTION_NONE;
    out_submit->frame_result = MEM_CONSOLE_FRAME_FATAL;
    if (!ctx || !ctx->render_ctx || !ctx->ui_ctx || !ctx->state || !derive) {
        return;
    }

    out_submit->frame_result = run_frame(ctx->render_ctx,
                                         ctx->ui_ctx,
                                         ctx->state,
                                         &derive->input,
                                         derive->frame_width,
                                         derive->frame_height,
                                         derive->wheel_y,
                                         &out_submit->ui_action);
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
        mem_console_app_set_statusf(state,
                                    "%s (vk=%d).",
                                    reason ? reason : "Swapchain recreate failed",
                                    (int)vk_result);
        return 0;
    }

    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_LAYOUT | MEM_CONSOLE_REDRAW_REASON_BACKGROUND);
    return 1;
}

typedef enum MemConsoleLoopRenderPhaseResult {
    MEM_CONSOLE_LOOP_RENDER_PHASE_OK = 0,
    MEM_CONSOLE_LOOP_RENDER_PHASE_CONTINUE = 1,
    MEM_CONSOLE_LOOP_RENDER_PHASE_FATAL = 2
} MemConsoleLoopRenderPhaseResult;

typedef enum MemConsoleLoopStepResult {
    MEM_CONSOLE_LOOP_STEP_OK = 0,
    MEM_CONSOLE_LOOP_STEP_EXIT = 1,
    MEM_CONSOLE_LOOP_STEP_FATAL = 2
} MemConsoleLoopStepResult;

typedef struct MemConsoleLoopAsyncOutcome {
    bool runtime_content_changed;
    bool runtime_graph_content_changed;
    bool runtime_metrics_changed;
    bool kernel_render_requested;
} MemConsoleLoopAsyncOutcome;

typedef struct MemConsoleLoopRenderDecision {
    uint32_t frame_reasons;
    bool should_submit;
} MemConsoleLoopRenderDecision;

static int mem_console_loop_context_valid(const MemConsoleAppLoopContext *ctx) {
    if (!ctx || !ctx->window || !ctx->renderer || !ctx->render_ctx || !ctx->ui_ctx ||
        !ctx->db || !ctx->state || !ctx->runtime || !ctx->kernel_bridge ||
        !ctx->prefs_path_valid || !ctx->prefs_signature_valid || !ctx->prefs_last_saved_signature ||
        !ctx->app_prefs_path || !ctx->prefs_path || ctx->prefs_path_cap == 0u) {
        return 0;
    }
    return 1;
}

static void mem_console_loop_frame_begin(MemConsoleLoopFrameState *frame,
                                         MemConsoleLoopRunState *run_state) {
    if (!frame || !run_state) {
        return;
    }
    memset(frame, 0, sizeof(*frame));
    frame->keyboard_action = MEM_CONSOLE_ACTION_NONE;
    frame->ui_action = MEM_CONSOLE_ACTION_NONE;
    frame->pending_action = MEM_CONSOLE_ACTION_NONE;
    frame->frame_result = MEM_CONSOLE_FRAME_OK;
    frame->frame_begin_ms = SDL_GetTicks64();
    mem_console_input_frame_begin(&frame->input_frame);
    run_state->input.mouse_pressed = 0;
    run_state->input.mouse_released = 0;
}

static MemConsoleLoopAsyncOutcome mem_console_loop_async_phase(const MemConsoleAppLoopContext *ctx) {
    MemConsoleLoopAsyncOutcome outcome;
    MemConsoleRuntimeTickOutcome runtime_outcome;

    memset(&outcome, 0, sizeof(outcome));
    memset(&runtime_outcome, 0, sizeof(runtime_outcome));
    if (!ctx || !ctx->state) {
        return outcome;
    }
    if (ctx->state->search_refresh_pending) {
        Uint64 now_ms = SDL_GetTicks64();
        if (now_ms >= ctx->state->search_last_input_ms &&
            (now_ms - ctx->state->search_last_input_ms) >= 150u) {
            ctx->state->search_refresh_pending = 0;
            mem_console_app_refresh_and_report(ctx->db, ctx->state, "Search refresh failed");
        }
    }

    mem_console_runtime_tick(ctx->runtime, ctx->state, SDL_GetTicks64(), &runtime_outcome);
    outcome.runtime_content_changed = runtime_outcome.content_changed != 0;
    outcome.runtime_graph_content_changed = runtime_outcome.graph_content_changed != 0;
    outcome.runtime_metrics_changed = runtime_outcome.metrics_changed != 0;

    if (ctx->kernel_bridge_requested) {
        uint64_t now_ns = (uint64_t)SDL_GetTicks64() * 1000000ULL;
        mem_console_kernel_bridge_tick(ctx->kernel_bridge, ctx->state, now_ns);
        if (ctx->state->kernel_last_render_requested) {
            outcome.kernel_render_requested = true;
        }
    }
    return outcome;
}

static void mem_console_loop_apply_async_outcome(const MemConsoleAppLoopContext *ctx,
                                                 const MemConsoleLoopAsyncOutcome *outcome) {
    if (!ctx || !ctx->state || !outcome) {
        return;
    }
    if (outcome->runtime_graph_content_changed && !outcome->runtime_content_changed) {
        mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
    }
    if (outcome->runtime_content_changed) {
        mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
    }
    if (outcome->runtime_metrics_changed && !outcome->runtime_content_changed) {
        /* Keep metrics updates passive; they render on the next content-driven frame. */
    }
    if (outcome->kernel_render_requested) {
        mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_BACKGROUND);
    }
}

static void mem_console_loop_resize_phase(const MemConsoleAppLoopContext *ctx,
                                          MemConsoleLoopRunState *run_state) {
    if (!ctx || !run_state) {
        return;
    }
    SDL_GetWindowSize(ctx->window, &run_state->frame_width, &run_state->frame_height);
    if (run_state->frame_width != run_state->last_frame_width ||
        run_state->frame_height != run_state->last_frame_height) {
        (void)mem_console_app_recreate_swapchain_and_mark(ctx->renderer,
                                                          ctx->window,
                                                          ctx->state,
                                                          "Swapchain refresh failed after resize");
        mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_LAYOUT);
        run_state->last_frame_width = run_state->frame_width;
        run_state->last_frame_height = run_state->frame_height;
    }
}

static MemConsoleLoopRenderDecision mem_console_loop_render_decide(const MemConsoleAppLoopContext *ctx) {
    MemConsoleLoopRenderDecision decision;

    memset(&decision, 0, sizeof(decision));
    if (!ctx || !ctx->state) {
        return decision;
    }
    decision.frame_reasons = mem_console_redraw_take_pending(ctx->state);
    decision.should_submit = decision.frame_reasons != 0u;
    return decision;
}

static void mem_console_loop_note_render_submit_diag(MemConsoleLoopRunState *run_state,
                                                     uint32_t frame_reasons,
                                                     int frame_result) {
    if (!run_state) {
        return;
    }
    run_state->rs1_diag_totals.frame_count += 1u;
    if (frame_result == MEM_CONSOLE_FRAME_OK) {
        run_state->rs1_diag_totals.submit_ok_count += 1u;
    } else if (frame_result == MEM_CONSOLE_FRAME_RECOVERABLE) {
        run_state->rs1_diag_totals.submit_recoverable_count += 1u;
    } else {
        run_state->rs1_diag_totals.submit_fatal_count += 1u;
    }
    if (mem_console_rs1_diag_enabled()) {
        printf("[rs1] mem_console frame=%llu reasons=0x%x submit=%d totals(frames=%llu ok=%llu recoverable=%llu fatal=%llu)\n",
               (unsigned long long)run_state->rs1_diag_totals.frame_count,
               (unsigned int)frame_reasons,
               frame_result,
               (unsigned long long)run_state->rs1_diag_totals.frame_count,
               (unsigned long long)run_state->rs1_diag_totals.submit_ok_count,
               (unsigned long long)run_state->rs1_diag_totals.submit_recoverable_count,
               (unsigned long long)run_state->rs1_diag_totals.submit_fatal_count);
    }
}

static MemConsoleLoopRenderPhaseResult mem_console_loop_handle_render_submit_result(const MemConsoleAppLoopContext *ctx,
                                                                                    MemConsoleLoopFrameState *frame) {
    if (!ctx || !frame) {
        return MEM_CONSOLE_LOOP_RENDER_PHASE_FATAL;
    }
    if (frame->frame_result == MEM_CONSOLE_FRAME_RECOVERABLE) {
        if (!mem_console_app_recreate_swapchain_and_mark(ctx->renderer,
                                                         ctx->window,
                                                         ctx->state,
                                                         "Swapchain recover failed")) {
            return MEM_CONSOLE_LOOP_RENDER_PHASE_FATAL;
        }
        return MEM_CONSOLE_LOOP_RENDER_PHASE_CONTINUE;
    }
    if (frame->frame_result == MEM_CONSOLE_FRAME_FATAL) {
        return MEM_CONSOLE_LOOP_RENDER_PHASE_FATAL;
    }
    mem_console_redraw_note_frame(ctx->state, frame->frame_reasons, SDL_GetTicks64());
    return MEM_CONSOLE_LOOP_RENDER_PHASE_OK;
}

static MemConsoleLoopRenderPhaseResult mem_console_loop_render_phase(const MemConsoleAppLoopContext *ctx,
                                                                     MemConsoleLoopRunState *run_state,
                                                                     MemConsoleLoopFrameState *frame) {
    MemConsoleLoopRenderDecision render_decision;
    MemConsoleRenderDeriveFrame render_derive;
    MemConsoleRenderSubmitOutcome render_submit;

    if (!ctx || !run_state || !frame) {
        return MEM_CONSOLE_LOOP_RENDER_PHASE_FATAL;
    }

    render_decision = mem_console_loop_render_decide(ctx);
    frame->frame_reasons = render_decision.frame_reasons;
    if (!render_decision.should_submit) {
        return MEM_CONSOLE_LOOP_RENDER_PHASE_OK;
    }

    mem_console_render_derive_frame(&render_derive,
                                    &run_state->input,
                                    run_state->frame_width,
                                    run_state->frame_height,
                                    frame->wheel_y,
                                    frame->frame_reasons);
    mem_console_render_submit_frame(ctx, &render_derive, &render_submit);
    frame->frame_result = render_submit.frame_result;
    frame->ui_action = render_submit.ui_action;

    mem_console_loop_note_render_submit_diag(run_state,
                                             render_derive.frame_reasons,
                                             frame->frame_result);
    return mem_console_loop_handle_render_submit_result(ctx, frame);
}

static void mem_console_loop_post_action_phase(const MemConsoleAppLoopContext *ctx,
                                               MemConsoleLoopFrameState *frame) {
    if (!ctx || !frame) {
        return;
    }
    if (frame->keyboard_action != MEM_CONSOLE_ACTION_NONE) {
        frame->pending_action = frame->keyboard_action;
    } else {
        frame->pending_action = frame->ui_action;
    }
    mem_console_app_apply_pending_action(ctx->db,
                                         ctx->state,
                                         ctx->runtime,
                                         ctx->app_prefs_path,
                                         ctx->app_prefs_path_valid,
                                         frame->pending_action);

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
            mem_console_redraw_mark(ctx->state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        }
    }

    if (!mem_console_workspace_authoring_host_active(&ctx->state->workspace_authoring) &&
        *ctx->prefs_path_valid) {
        uint64_t current_signature = mem_console_prefs_state_signature(ctx->state);
        if (!*ctx->prefs_signature_valid) {
            *ctx->prefs_last_saved_signature = current_signature;
            *ctx->prefs_signature_valid = 1;
        } else if (current_signature != *ctx->prefs_last_saved_signature) {
            mem_console_pane_prefs_mark_dirty(ctx->state);
        }
    }

    if (*ctx->prefs_path_valid &&
        !mem_console_workspace_authoring_host_active(&ctx->state->workspace_authoring) &&
        ctx->state->pane_prefs_dirty &&
        !ctx->state->pane_drag_active &&
        !ctx->state->left_panel_drag_active &&
        !ctx->state->search_refresh_pending) {
        CoreResult result = mem_console_prefs_save(ctx->prefs_path, ctx->state);
        if (result.code == CORE_OK) {
            mem_console_pane_prefs_mark_clean(ctx->state);
            *ctx->prefs_last_saved_signature = mem_console_prefs_state_signature(ctx->state);
            *ctx->prefs_signature_valid = 1;
        } else {
            mem_console_app_set_path_result_status(ctx->state, "Pane prefs save", ctx->prefs_path, result);
        }
    }
}

static void mem_console_loop_idle_tail_phase(const MemConsoleAppLoopContext *ctx,
                                             const MemConsoleLoopFrameState *frame) {
    if (!ctx || !frame) {
        return;
    }
    if (frame->waited_event == 0 &&
        frame->wait_performed == 0 &&
        frame->pending_action == MEM_CONSOLE_ACTION_NONE &&
        mem_console_redraw_pending(ctx->state) == 0u) {
        SDL_Delay(1u);
    }
}

static void mem_console_loop_frame_finalize(const MemConsoleLoopFrameState *frame) {
    if (!frame) {
        return;
    }
    mem_console_loop_diag_tick(frame->frame_begin_ms,
                               SDL_GetTicks64(),
                               frame->wait_call_count,
                               frame->wait_blocked_ms);
}

static MemConsoleLoopStepResult mem_console_loop_frame_step(const MemConsoleAppLoopContext *ctx,
                                                            MemConsoleLoopRunState *run_state) {
    MemConsoleLoopFrameState frame;
    MemConsoleLoopAsyncOutcome async_outcome;
    MemConsoleLoopRenderPhaseResult render_phase_result;
    MemConsoleLoopStepResult step_result = MEM_CONSOLE_LOOP_STEP_OK;
    bool run_post_action = true;
    bool run_idle_tail = true;

    if (!ctx || !run_state) {
        return MEM_CONSOLE_LOOP_STEP_FATAL;
    }

    mem_console_loop_frame_begin(&frame, run_state);
    if (mem_console_loop_input_phase(ctx, run_state, &frame) != MEM_CONSOLE_LOOP_INPUT_PHASE_OK) {
        step_result = MEM_CONSOLE_LOOP_STEP_EXIT;
        run_post_action = false;
        run_idle_tail = false;
        goto finalize_frame;
    }

    async_outcome = mem_console_loop_async_phase(ctx);
    mem_console_loop_apply_async_outcome(ctx, &async_outcome);
    mem_console_loop_resize_phase(ctx, run_state);
    render_phase_result = mem_console_loop_render_phase(ctx, run_state, &frame);
    if (render_phase_result == MEM_CONSOLE_LOOP_RENDER_PHASE_FATAL) {
        step_result = MEM_CONSOLE_LOOP_STEP_FATAL;
        run_post_action = false;
        run_idle_tail = false;
        goto finalize_frame;
    }
    if (render_phase_result == MEM_CONSOLE_LOOP_RENDER_PHASE_CONTINUE) {
        run_post_action = false;
        run_idle_tail = false;
    }

    if (run_post_action) {
        mem_console_loop_post_action_phase(ctx, &frame);
    }
    if (run_idle_tail) {
        mem_console_loop_idle_tail_phase(ctx, &frame);
    }

finalize_frame:
    mem_console_loop_frame_finalize(&frame);
    return step_result;
}

int mem_console_app_run_loop(MemConsoleAppLoopContext *ctx) {
    MemConsoleLoopRunState run_state;

    if (!mem_console_loop_context_valid(ctx)) {
        return 1;
    }
    memset(&run_state, 0, sizeof(run_state));
    run_state.running = true;
    run_state.last_frame_width = -1;
    run_state.last_frame_height = -1;

    while (run_state.running) {
        MemConsoleLoopStepResult step_result = mem_console_loop_frame_step(ctx, &run_state);
        if (step_result == MEM_CONSOLE_LOOP_STEP_FATAL) {
            mem_console_workspace_authoring_host_cancel_active_preview(&ctx->state->workspace_authoring,
                                                                       ctx->state,
                                                                       ctx->render_ctx,
                                                                       ctx->ui_ctx);
            return 1;
        }
        if (step_result == MEM_CONSOLE_LOOP_STEP_EXIT) {
            break;
        }
    }

    mem_console_workspace_authoring_host_cancel_active_preview(&ctx->state->workspace_authoring,
                                                               ctx->state,
                                                               ctx->render_ctx,
                                                               ctx->ui_ctx);

    return 0;
}
