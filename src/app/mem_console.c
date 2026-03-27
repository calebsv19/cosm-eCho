/*
 * mem_console.c
 * Part of the CodeWork Shared Libraries
 * Copyright (c) 2026 Caleb S. V.
 * Licensed under the Apache License, Version 2.0
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#include "mem_console_prefs.h"
#include "mem_console_app_internal.h"

int main(int argc, char **argv) {
    const char *db_path = k_mem_console_default_db_path;
    const char *db_flag = 0;
    char app_prefs_path[1200];
    char app_prefs_db_path[1024];
    char default_db_path[1024];
    char prefs_path[1200];
    SDL_Window *window = 0;
    VkRenderer renderer;
    VkRendererConfig config;
    KitRenderContext render_ctx;
    KitUiContext ui_ctx;
    CoreMemDb db = {0};
    CoreResult result;
    MemConsoleState state;
    MemConsoleRuntime runtime;
    MemConsoleKernelBridge kernel_bridge;
    MemConsoleAppLoopContext loop_ctx;
    int kernel_bridge_requested = 0;
    int exit_code = 1;
    int app_prefs_path_valid = 0;
    int prefs_path_valid = 0;
    int prefs_signature_valid = 0;
    uint64_t prefs_last_saved_signature = 0u;

    memset(&renderer, 0, sizeof(renderer));
    memset(&config, 0, sizeof(config));
    memset(&render_ctx, 0, sizeof(render_ctx));
    memset(&ui_ctx, 0, sizeof(ui_ctx));
    memset(&runtime, 0, sizeof(runtime));
    memset(&kernel_bridge, 0, sizeof(kernel_bridge));
    memset(&loop_ctx, 0, sizeof(loop_ctx));

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
    mem_console_app_apply_compact_ui_density(&ui_ctx);

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

    loop_ctx.window = window;
    loop_ctx.renderer = &renderer;
    loop_ctx.render_ctx = &render_ctx;
    loop_ctx.ui_ctx = &ui_ctx;
    loop_ctx.db = &db;
    loop_ctx.state = &state;
    loop_ctx.runtime = &runtime;
    loop_ctx.kernel_bridge = &kernel_bridge;
    loop_ctx.kernel_bridge_requested = kernel_bridge_requested;
    loop_ctx.app_prefs_path = app_prefs_path;
    loop_ctx.app_prefs_path_valid = app_prefs_path_valid;
    loop_ctx.prefs_path = prefs_path;
    loop_ctx.prefs_path_cap = sizeof(prefs_path);
    loop_ctx.prefs_path_valid = &prefs_path_valid;
    loop_ctx.prefs_signature_valid = &prefs_signature_valid;
    loop_ctx.prefs_last_saved_signature = &prefs_last_saved_signature;

    exit_code = mem_console_app_run_loop(&loop_ctx);

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
