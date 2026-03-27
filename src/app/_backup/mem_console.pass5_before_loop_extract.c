/*
 * mem_console.c
 * Part of the CodeWork Shared Libraries
 * Copyright (c) 2026 Caleb S. V.
 * Licensed under the Apache License, Version 2.0
 */

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "kit_render.h"
#include "kit_ui.h"
#include "mem_console_db.h"
#include "mem_console_kernel_bridge.h"
#include "mem_console_prefs.h"
#include "mem_console_runtime.h"
#include "mem_console_state.h"
#include "mem_console_ui.h"
#include "mem_console_ui_graph.h"
#include "vk_renderer.h"

#include "mem_console_app_internal.h"

static int recreate_swapchain_and_mark(VkRenderer *renderer,
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

int main(int argc, char **argv) {
    const char *db_path = k_mem_console_default_db_path;
    const char *db_flag = 0;
    char app_prefs_path[1200];
    char app_prefs_db_path[1024];
    char default_db_path[1024];
    char prefs_path[1200];
    SDL_Window *window = 0;
    SDL_Event event;
    bool running = true;
    VkRenderer renderer;
    VkRendererConfig config;
    KitRenderContext render_ctx;
    KitUiContext ui_ctx;
    KitUiInputState input = {0};
    CoreMemDb db = {0};
    CoreResult result;
    MemConsoleState state;
    MemConsoleRuntime runtime;
    MemConsoleKernelBridge kernel_bridge;
    int kernel_bridge_requested = 0;
    int frame_width = 0;
    int frame_height = 0;
    int last_frame_width = -1;
    int last_frame_height = -1;
    int wheel_y = 0;
    int exit_code = 1;
    int app_prefs_path_valid = 0;
    int prefs_path_valid = 0;
    int prefs_signature_valid = 0;
    uint64_t prefs_last_saved_signature = 0u;

    if (has_unknown_flag(argc, argv)) {
        print_usage(argv[0]);
        return 1;
    }

    db_flag = find_flag_value(argc, argv, "--db");
    kernel_bridge_requested = has_flag(argc, argv, "--kernel-bridge");
    if (db_flag) {
        db_path = db_flag;
    } else if (mem_console_build_app_prefs_path(app_prefs_path, sizeof(app_prefs_path))) {
        app_prefs_path_valid = 1;
        result = mem_console_app_prefs_load(app_prefs_path, app_prefs_db_path, sizeof(app_prefs_db_path));
        if (result.code == CORE_OK && result.message && strcmp(result.message, "app prefs loaded") == 0) {
            db_path = app_prefs_db_path;
        } else if (resolve_default_db_path(default_db_path, sizeof(default_db_path))) {
            db_path = default_db_path;
        }
    } else if (resolve_default_db_path(default_db_path, sizeof(default_db_path))) {
        db_path = default_db_path;
    }

    if (!mem_console_ensure_parent_directory(db_path)) {
        fprintf(stderr, "mem_console: failed to create parent directory for DB path: %s\n", db_path);
        return 1;
    }

    seed_state(&state, db_path);
    prefs_path_valid = mem_console_build_prefs_path(db_path, prefs_path, sizeof(prefs_path));
    if (prefs_path_valid) {
        result = mem_console_prefs_load(prefs_path, &state);
        if (result.code != CORE_OK) {
            (void)snprintf(state.status_line, sizeof(state.status_line), "UI prefs load failed.");
        } else if (result.message && strcmp(result.message, "prefs loaded") == 0) {
            (void)snprintf(state.status_line, sizeof(state.status_line), "UI prefs restored.");
        }
    }
    memset(&runtime, 0, sizeof(runtime));
    memset(&kernel_bridge, 0, sizeof(kernel_bridge));
    state.kernel_bridge_enabled = kernel_bridge_requested ? 1 : 0;

    result = core_memdb_open(state.db_path, &db);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: open failed: %s\n", result.message);
        return 1;
    }

    result = refresh_state_from_db(&db, &state);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: refresh failed: %s\n", result.message);
        goto cleanup_db;
    }
    sync_edit_buffers_from_selection(&state);
    if (prefs_path_valid) {
        prefs_last_saved_signature = mem_console_prefs_state_signature(&state);
        prefs_signature_valid = 1;
        state.pane_prefs_dirty = 0;
    }
    if (app_prefs_path_valid) {
        result = mem_console_app_prefs_save(app_prefs_path, state.db_path);
        if (result.code != CORE_OK) {
            (void)snprintf(state.status_line,
                           sizeof(state.status_line),
                           "App prefs save failed.");
        }
    }
    mem_console_redraw_mark(&state, MEM_CONSOLE_REDRAW_REASON_CONTENT);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "mem_console: SDL_Init failed: %s\n", SDL_GetError());
        goto cleanup_db;
    }

    window = SDL_CreateWindow("mem_console",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              1440,
                              900,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "mem_console: SDL_CreateWindow failed: %s\n", SDL_GetError());
        goto cleanup_sdl;
    }

    vk_renderer_config_set_defaults(&config);
    config.enable_validation = VK_FALSE;
    if (vk_renderer_init(&renderer, window, &config) != VK_SUCCESS) {
        fprintf(stderr, "mem_console: vk_renderer_init failed\n");
        goto cleanup_window;
    }

    result = kit_render_context_init(&render_ctx,
                                     KIT_RENDER_BACKEND_VULKAN,
                                     state.theme_preset_id,
                                     state.font_preset_id);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: kit_render_context_init failed: %d\n", (int)result.code);
        goto cleanup_renderer;
    }

    result = kit_render_attach_external_backend(&render_ctx, &renderer);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: kit_render_attach_external_backend failed: %d\n", (int)result.code);
        goto cleanup_render_ctx;
    }

    result = kit_ui_context_init(&ui_ctx, &render_ctx);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: kit_ui_context_init failed: %d\n", (int)result.code);
        goto cleanup_render_ctx;
    }
    (void)kit_ui_style_apply_theme_scale(&ui_ctx);
    apply_compact_ui_density(&ui_ctx);

    result = mem_console_runtime_init(&runtime, SDL_GetTicks64());
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: runtime init failed: %s\n", result.message);
        goto cleanup_runtime;
    }

    if (kernel_bridge_requested) {
        result = mem_console_kernel_bridge_init(&kernel_bridge);
        if (result.code != CORE_OK) {
            fprintf(stderr, "mem_console: kernel bridge init failed: %s\n", result.message);
            goto cleanup_kernel_bridge;
        }
    } else {
        (void)snprintf(state.kernel_summary_line,
                       sizeof(state.kernel_summary_line),
                       "Kernel off");
    }

    SDL_StartTextInput();

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

        input.mouse_pressed = 0;
        input.mouse_released = 0;
        wheel_y = 0;

        idle_wait_ms = mem_console_runtime_idle_wait_ms(&runtime, &state, SDL_GetTicks64());
        if (idle_wait_ms > 0u) {
            if (SDL_WaitEventTimeout(&event, (int)idle_wait_ms)) {
                waited_event = 1;
                process_sdl_event(&event,
                                  &running,
                                  &render_ctx,
                                  &ui_ctx,
                                  &state,
                                  prefs_path_valid ? prefs_path : "",
                                  &input,
                                  &wheel_y,
                                  &keyboard_action);
            }
        }

        if (!running) {
            break;
        }

        while (SDL_PollEvent(&event)) {
            process_sdl_event(&event,
                              &running,
                              &render_ctx,
                              &ui_ctx,
                              &state,
                              prefs_path_valid ? prefs_path : "",
                              &input,
                              &wheel_y,
                              &keyboard_action);
        }

        if (!running) {
            break;
        }

        if (state.search_refresh_pending) {
            Uint64 now_ms = SDL_GetTicks64();
            if (now_ms >= state.search_last_input_ms &&
                (now_ms - state.search_last_input_ms) >= 150u) {
                state.search_refresh_pending = 0;
                refresh_and_report(&db, &state, "Search refresh failed");
            }
        }

        runtime_applied_before = state.runtime_refresh_applied;
        runtime_dropped_before = state.runtime_refresh_dropped;
        runtime_errors_before = state.runtime_refresh_errors;
        runtime_coalesced_before = state.runtime_refresh_coalesced;
        runtime_in_flight_before = state.runtime_refresh_in_flight;
        runtime_pending_before = state.runtime_pending_intent;
        mem_console_runtime_tick(&runtime, &state, SDL_GetTicks64());
        if (state.runtime_refresh_applied != runtime_applied_before ||
            state.runtime_refresh_dropped != runtime_dropped_before ||
            state.runtime_refresh_errors != runtime_errors_before ||
            state.runtime_refresh_coalesced != runtime_coalesced_before ||
            state.runtime_refresh_in_flight != runtime_in_flight_before ||
            state.runtime_pending_intent != runtime_pending_before) {
            mem_console_redraw_mark(&state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
        }
        if (kernel_bridge_requested) {
            uint64_t now_ns = (uint64_t)SDL_GetTicks64() * 1000000ULL;
            mem_console_kernel_bridge_tick(&kernel_bridge, &state, now_ns);
            if (state.kernel_last_render_requested) {
                mem_console_redraw_mark(&state, MEM_CONSOLE_REDRAW_REASON_BACKGROUND);
            }
        }

        SDL_GetWindowSize(window, &frame_width, &frame_height);
        if (frame_width != last_frame_width || frame_height != last_frame_height) {
            (void)recreate_swapchain_and_mark(&renderer,
                                              window,
                                              &state,
                                              "Swapchain refresh failed after resize");
            mem_console_redraw_mark(&state, MEM_CONSOLE_REDRAW_REASON_LAYOUT);
            last_frame_width = frame_width;
            last_frame_height = frame_height;
        }

        frame_reasons = mem_console_redraw_pending(&state);
        if (frame_reasons != 0u) {
            frame_reasons = mem_console_redraw_take_pending(&state);
            frame_result = run_frame(&render_ctx,
                                     &ui_ctx,
                                     &state,
                                     &input,
                                     frame_width,
                                     frame_height,
                                     wheel_y,
                                     &ui_action);
            if (frame_result == MEM_CONSOLE_FRAME_RECOVERABLE) {
                if (!recreate_swapchain_and_mark(&renderer,
                                                 window,
                                                 &state,
                                                 "Swapchain recover failed")) {
                    goto cleanup_text_input;
                }
                continue;
            }
            if (frame_result == MEM_CONSOLE_FRAME_FATAL) {
                goto cleanup_text_input;
            }
            mem_console_redraw_note_frame(&state, frame_reasons, SDL_GetTicks64());
        }

        if (keyboard_action != MEM_CONSOLE_ACTION_NONE) {
            pending_action = keyboard_action;
        } else {
            pending_action = ui_action;
        }
        apply_pending_action(&db, &state, &runtime, pending_action);

        if (state.pending_db_path[0] != '\0') {
            char next_db_path[1024];
            (void)snprintf(next_db_path, sizeof(next_db_path), "%s", state.pending_db_path);
            state.pending_db_path[0] = '\0';
            result = switch_active_db(&db,
                                      &state,
                                      next_db_path,
                                      app_prefs_path,
                                      app_prefs_path_valid,
                                      prefs_path,
                                      sizeof(prefs_path),
                                      &prefs_path_valid,
                                      &prefs_signature_valid,
                                      &prefs_last_saved_signature);
            if (result.code != CORE_OK) {
                (void)snprintf(state.status_line,
                               sizeof(state.status_line),
                               "DB switch failed: %s",
                               result.message ? result.message : "error");
                mem_console_redraw_mark(&state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            }
        }

        if (prefs_path_valid) {
            uint64_t current_signature = mem_console_prefs_state_signature(&state);
            if (!prefs_signature_valid) {
                prefs_last_saved_signature = current_signature;
                prefs_signature_valid = 1;
            } else if (current_signature != prefs_last_saved_signature) {
                state.pane_prefs_dirty = 1;
            }
        }

        if (prefs_path_valid &&
            state.pane_prefs_dirty &&
            !state.pane_drag_active &&
            !state.search_refresh_pending) {
            result = mem_console_prefs_save(prefs_path, &state);
            if (result.code == CORE_OK) {
                state.pane_prefs_dirty = 0;
                prefs_last_saved_signature = mem_console_prefs_state_signature(&state);
                prefs_signature_valid = 1;
            } else {
                (void)snprintf(state.status_line,
                               sizeof(state.status_line),
                               "Pane prefs save failed.");
                mem_console_redraw_mark(&state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
            }
        }

        if (waited_event == 0 &&
            pending_action == MEM_CONSOLE_ACTION_NONE &&
            mem_console_redraw_pending(&state) == 0u) {
            SDL_Delay(1u);
        }
    }

    exit_code = 0;

cleanup_text_input:
    if (prefs_path_valid && state.pane_prefs_dirty) {
        result = mem_console_prefs_save(prefs_path, &state);
        if (result.code == CORE_OK) {
            state.pane_prefs_dirty = 0;
            prefs_last_saved_signature = mem_console_prefs_state_signature(&state);
            prefs_signature_valid = 1;
        }
    }
    SDL_StopTextInput();
cleanup_kernel_bridge:
    mem_console_kernel_bridge_shutdown(&kernel_bridge);
cleanup_runtime:
    mem_console_runtime_shutdown(&runtime);
cleanup_render_ctx:
    kit_render_context_shutdown(&render_ctx);
cleanup_renderer:
    vk_renderer_shutdown(&renderer);
cleanup_window:
    SDL_DestroyWindow(window);
cleanup_sdl:
    SDL_Quit();
cleanup_db:
    (void)core_memdb_close(&db);
    return exit_code;
}
