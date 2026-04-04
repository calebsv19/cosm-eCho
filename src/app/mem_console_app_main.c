/*
 * mem_console_app_main.c
 * Part of the CodeWork Shared Libraries
 * Copyright (c) 2026 Caleb S. V.
 * Licensed under the Apache License, Version 2.0
 */

#include "mem_console_app_main.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#include "mem_console_app_internal.h"
#include "mem_console_prefs.h"

typedef enum MemConsoleAppStage {
    MEM_CONSOLE_APP_STAGE_INIT = 0,
    MEM_CONSOLE_APP_STAGE_BOOTSTRAPPED,
    MEM_CONSOLE_APP_STAGE_CONFIG_LOADED,
    MEM_CONSOLE_APP_STAGE_STATE_SEEDED,
    MEM_CONSOLE_APP_STAGE_SUBSYSTEMS_READY,
    MEM_CONSOLE_APP_STAGE_RUNTIME_STARTED,
    MEM_CONSOLE_APP_STAGE_LOOP_COMPLETED,
    MEM_CONSOLE_APP_STAGE_SHUTDOWN_COMPLETED
} MemConsoleAppStage;

typedef enum MemConsoleWrapperError {
    MEM_CONSOLE_WRAPPER_ERROR_NONE = 0,
    MEM_CONSOLE_WRAPPER_ERROR_BOOTSTRAP_FAILED = 1,
    MEM_CONSOLE_WRAPPER_ERROR_CONFIG_LOAD_FAILED = 2,
    MEM_CONSOLE_WRAPPER_ERROR_STATE_SEED_FAILED = 3,
    MEM_CONSOLE_WRAPPER_ERROR_SUBSYSTEMS_INIT_FAILED = 4,
    MEM_CONSOLE_WRAPPER_ERROR_RUNTIME_START_FAILED = 5,
    MEM_CONSOLE_WRAPPER_ERROR_RUN_LOOP_FAILED = 6
} MemConsoleWrapperError;

typedef struct MemConsoleAppMainContext {
    int argc;
    char **argv;
    const char *db_path;
    const char *db_flag;
    char app_prefs_path[1200];
    char app_prefs_db_path[1024];
    char default_db_path[1024];
    char prefs_path[1200];
    SDL_Window *window;
    VkRenderer renderer;
    VkRendererConfig config;
    KitRenderContext render_ctx;
    KitUiContext ui_ctx;
    CoreMemDb db;
    CoreResult result;
    MemConsoleState state;
    MemConsoleRuntime runtime;
    MemConsoleKernelBridge kernel_bridge;
    MemConsoleAppLoopContext loop_ctx;
    int kernel_bridge_requested;
    int exit_code;
    int app_prefs_path_valid;
    int prefs_path_valid;
    int prefs_signature_valid;
    uint64_t prefs_last_saved_signature;
    int db_open;
    int sdl_initialized;
    int window_created;
    int renderer_initialized;
    int render_ctx_initialized;
    int runtime_initialized;
    int kernel_bridge_initialized;
    int text_input_started;
    int dispatch_ran;
    int dispatch_succeeded;
    MemConsoleWrapperError wrapper_error;
    MemConsoleAppStage stage;
} MemConsoleAppMainContext;

static void mem_console_log_wrapper_error(const char *fn_name,
                                          MemConsoleWrapperError wrapper_error,
                                          MemConsoleAppStage stage,
                                          int exit_code) {
    fprintf(stderr,
            "mem_console: wrapper error fn=%s code=%d stage=%d exit_code=%d\n",
            fn_name ? fn_name : "unknown",
            (int)wrapper_error,
            (int)stage,
            exit_code);
}

static int mem_console_app_stage_transition(MemConsoleAppMainContext *ctx,
                                            MemConsoleAppStage expected,
                                            MemConsoleAppStage next,
                                            const char *stage_name,
                                            const char *fn_name) {
    if (!ctx) {
        return 0;
    }
    if (ctx->stage != expected) {
        fprintf(stderr,
                "mem_console: lifecycle stage order violation fn=%s stage=%s expected=%d actual=%d next=%d\n",
                fn_name ? fn_name : "unknown",
                stage_name ? stage_name : "unknown",
                (int)expected,
                (int)ctx->stage,
                (int)next);
        return 0;
    }
    ctx->stage = next;
    return 1;
}

static int mem_console_app_bootstrap(MemConsoleAppMainContext *ctx,
                                     int argc,
                                     char **argv) {
    if (!ctx) {
        return 0;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->argc = argc;
    ctx->argv = argv;
    ctx->db_path = k_mem_console_default_db_path;
    ctx->exit_code = 1;

    return mem_console_app_stage_transition(ctx,
                                            MEM_CONSOLE_APP_STAGE_INIT,
                                            MEM_CONSOLE_APP_STAGE_BOOTSTRAPPED,
                                            "mem_console_app_bootstrap",
                                            __func__);
}

static int mem_console_app_config_load(MemConsoleAppMainContext *ctx) {
    if (!ctx) {
        return 0;
    }

    if (!mem_console_app_stage_transition(ctx,
                                          MEM_CONSOLE_APP_STAGE_BOOTSTRAPPED,
                                          MEM_CONSOLE_APP_STAGE_BOOTSTRAPPED,
                                          "mem_console_app_config_load.pre",
                                          __func__)) {
        return 0;
    }

    if (has_unknown_flag(ctx->argc, ctx->argv)) {
        print_usage(ctx->argv[0]);
        return 0;
    }

    ctx->db_flag = find_flag_value(ctx->argc, ctx->argv, "--db");
    ctx->kernel_bridge_requested = has_flag(ctx->argc, ctx->argv, "--kernel-bridge");
    if (ctx->db_flag) {
        ctx->db_path = ctx->db_flag;
    } else if (mem_console_build_app_prefs_path(ctx->app_prefs_path, sizeof(ctx->app_prefs_path))) {
        ctx->app_prefs_path_valid = 1;
        ctx->result = mem_console_app_prefs_load(ctx->app_prefs_path,
                                                 ctx->app_prefs_db_path,
                                                 sizeof(ctx->app_prefs_db_path));
        if (ctx->result.code == CORE_OK &&
            ctx->result.message &&
            strcmp(ctx->result.message, "app prefs loaded") == 0) {
            ctx->db_path = ctx->app_prefs_db_path;
        } else if (resolve_default_db_path(ctx->default_db_path, sizeof(ctx->default_db_path))) {
            ctx->db_path = ctx->default_db_path;
        }
    } else if (resolve_default_db_path(ctx->default_db_path, sizeof(ctx->default_db_path))) {
        ctx->db_path = ctx->default_db_path;
    }

    if (!mem_console_ensure_parent_directory(ctx->db_path)) {
        fprintf(stderr,
                "mem_console: failed to create parent directory for DB path: %s\n",
                ctx->db_path);
        return 0;
    }

    return mem_console_app_stage_transition(ctx,
                                            MEM_CONSOLE_APP_STAGE_BOOTSTRAPPED,
                                            MEM_CONSOLE_APP_STAGE_CONFIG_LOADED,
                                            "mem_console_app_config_load",
                                            __func__);
}

static int mem_console_app_state_seed(MemConsoleAppMainContext *ctx) {
    if (!ctx) {
        return 0;
    }
    if (!mem_console_app_stage_transition(ctx,
                                          MEM_CONSOLE_APP_STAGE_CONFIG_LOADED,
                                          MEM_CONSOLE_APP_STAGE_CONFIG_LOADED,
                                          "mem_console_app_state_seed.pre",
                                          __func__)) {
        return 0;
    }

    seed_state(&ctx->state, ctx->db_path);
    ctx->prefs_path_valid = mem_console_build_prefs_path(ctx->db_path,
                                                         ctx->prefs_path,
                                                         sizeof(ctx->prefs_path));
    if (ctx->prefs_path_valid) {
        ctx->result = mem_console_prefs_load(ctx->prefs_path, &ctx->state);
        if (ctx->result.code != CORE_OK) {
            (void)snprintf(ctx->state.status_line,
                           sizeof(ctx->state.status_line),
                           "UI prefs load failed.");
        } else if (ctx->result.message && strcmp(ctx->result.message, "prefs loaded") == 0) {
            (void)snprintf(ctx->state.status_line,
                           sizeof(ctx->state.status_line),
                           "UI prefs restored.");
        }
    }
    ctx->state.kernel_bridge_enabled = ctx->kernel_bridge_requested ? 1 : 0;

    ctx->result = core_memdb_open(ctx->state.db_path, &ctx->db);
    if (ctx->result.code != CORE_OK) {
        fprintf(stderr, "mem_console: open failed: %s\n", ctx->result.message);
        return 0;
    }
    ctx->db_open = 1;

    ctx->result = refresh_state_from_db(&ctx->db, &ctx->state);
    if (ctx->result.code != CORE_OK) {
        fprintf(stderr, "mem_console: refresh failed: %s\n", ctx->result.message);
        return 0;
    }

    sync_edit_buffers_from_selection(&ctx->state);
    if (ctx->prefs_path_valid) {
        ctx->prefs_last_saved_signature = mem_console_prefs_state_signature(&ctx->state);
        ctx->prefs_signature_valid = 1;
        ctx->state.pane_prefs_dirty = 0;
    }
    if (ctx->app_prefs_path_valid) {
        ctx->result = mem_console_app_prefs_save(ctx->app_prefs_path, ctx->state.db_path);
        if (ctx->result.code != CORE_OK) {
            (void)snprintf(ctx->state.status_line,
                           sizeof(ctx->state.status_line),
                           "App prefs save failed.");
        }
    }

    mem_console_redraw_mark(&ctx->state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
    return mem_console_app_stage_transition(ctx,
                                            MEM_CONSOLE_APP_STAGE_CONFIG_LOADED,
                                            MEM_CONSOLE_APP_STAGE_STATE_SEEDED,
                                            "mem_console_app_state_seed",
                                            __func__);
}

static int mem_console_app_subsystems_init(MemConsoleAppMainContext *ctx) {
    if (!ctx) {
        return 0;
    }
    if (!mem_console_app_stage_transition(ctx,
                                          MEM_CONSOLE_APP_STAGE_STATE_SEEDED,
                                          MEM_CONSOLE_APP_STAGE_STATE_SEEDED,
                                          "mem_console_app_subsystems_init.pre",
                                          __func__)) {
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "mem_console: SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }
    ctx->sdl_initialized = 1;

    ctx->window = SDL_CreateWindow("mem_console",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   1440,
                                   900,
                                   SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!ctx->window) {
        fprintf(stderr, "mem_console: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 0;
    }
    ctx->window_created = 1;

    vk_renderer_config_set_defaults(&ctx->config);
    ctx->config.enable_validation = VK_FALSE;
    if (vk_renderer_init(&ctx->renderer, ctx->window, &ctx->config) != VK_SUCCESS) {
        fprintf(stderr, "mem_console: vk_renderer_init failed\n");
        return 0;
    }
    ctx->renderer_initialized = 1;

    ctx->result = kit_render_context_init(&ctx->render_ctx,
                                          KIT_RENDER_BACKEND_VULKAN,
                                          ctx->state.theme_preset_id,
                                          ctx->state.font_preset_id);
    if (ctx->result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_render_context_init failed: %d\n",
                (int)ctx->result.code);
        return 0;
    }
    ctx->render_ctx_initialized = 1;

    ctx->result = kit_render_set_text_zoom_step(&ctx->render_ctx, ctx->state.text_zoom_step);
    if (ctx->result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_render_set_text_zoom_step failed: %d\n",
                (int)ctx->result.code);
        return 0;
    }

    ctx->result = kit_render_attach_external_backend(&ctx->render_ctx, &ctx->renderer);
    if (ctx->result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_render_attach_external_backend failed: %d\n",
                (int)ctx->result.code);
        return 0;
    }

    ctx->result = kit_ui_context_init(&ctx->ui_ctx, &ctx->render_ctx);
    if (ctx->result.code != CORE_OK) {
        fprintf(stderr,
                "mem_console: kit_ui_context_init failed: %d\n",
                (int)ctx->result.code);
        return 0;
    }
    (void)kit_ui_style_apply_theme_scale(&ctx->ui_ctx);
    mem_console_app_apply_compact_ui_density(&ctx->ui_ctx, &ctx->render_ctx);

    return mem_console_app_stage_transition(ctx,
                                            MEM_CONSOLE_APP_STAGE_STATE_SEEDED,
                                            MEM_CONSOLE_APP_STAGE_SUBSYSTEMS_READY,
                                            "mem_console_app_subsystems_init",
                                            __func__);
}

static int mem_console_runtime_start(MemConsoleAppMainContext *ctx) {
    if (!ctx) {
        return 0;
    }
    if (!mem_console_app_stage_transition(ctx,
                                          MEM_CONSOLE_APP_STAGE_SUBSYSTEMS_READY,
                                          MEM_CONSOLE_APP_STAGE_SUBSYSTEMS_READY,
                                          "mem_console_runtime_start.pre",
                                          __func__)) {
        return 0;
    }

    ctx->result = mem_console_runtime_init(&ctx->runtime, SDL_GetTicks64());
    if (ctx->result.code != CORE_OK) {
        fprintf(stderr, "mem_console: runtime init failed: %s\n", ctx->result.message);
        return 0;
    }
    ctx->runtime_initialized = 1;

    if (ctx->kernel_bridge_requested) {
        ctx->result = mem_console_kernel_bridge_init(&ctx->kernel_bridge);
        if (ctx->result.code != CORE_OK) {
            fprintf(stderr, "mem_console: kernel bridge init failed: %s\n", ctx->result.message);
            return 0;
        }
        ctx->kernel_bridge_initialized = 1;
    } else {
        (void)snprintf(ctx->state.kernel_summary_line,
                       sizeof(ctx->state.kernel_summary_line),
                       "Kernel off");
    }

    SDL_StartTextInput();
    ctx->text_input_started = 1;

    ctx->loop_ctx.window = ctx->window;
    ctx->loop_ctx.renderer = &ctx->renderer;
    ctx->loop_ctx.render_ctx = &ctx->render_ctx;
    ctx->loop_ctx.ui_ctx = &ctx->ui_ctx;
    ctx->loop_ctx.db = &ctx->db;
    ctx->loop_ctx.state = &ctx->state;
    ctx->loop_ctx.runtime = &ctx->runtime;
    ctx->loop_ctx.kernel_bridge = &ctx->kernel_bridge;
    ctx->loop_ctx.kernel_bridge_requested = ctx->kernel_bridge_requested;
    ctx->loop_ctx.app_prefs_path = ctx->app_prefs_path;
    ctx->loop_ctx.app_prefs_path_valid = ctx->app_prefs_path_valid;
    ctx->loop_ctx.prefs_path = ctx->prefs_path;
    ctx->loop_ctx.prefs_path_cap = sizeof(ctx->prefs_path);
    ctx->loop_ctx.prefs_path_valid = &ctx->prefs_path_valid;
    ctx->loop_ctx.prefs_signature_valid = &ctx->prefs_signature_valid;
    ctx->loop_ctx.prefs_last_saved_signature = &ctx->prefs_last_saved_signature;

    return mem_console_app_stage_transition(ctx,
                                            MEM_CONSOLE_APP_STAGE_SUBSYSTEMS_READY,
                                            MEM_CONSOLE_APP_STAGE_RUNTIME_STARTED,
                                            "mem_console_runtime_start",
                                            __func__);
}

static int mem_console_app_run_loop_stage(MemConsoleAppMainContext *ctx) {
    if (!ctx) {
        return 0;
    }
    if (!mem_console_app_stage_transition(ctx,
                                          MEM_CONSOLE_APP_STAGE_RUNTIME_STARTED,
                                          MEM_CONSOLE_APP_STAGE_RUNTIME_STARTED,
                                          "mem_console_app_run_loop.pre",
                                          __func__)) {
        return 0;
    }

    ctx->dispatch_ran = 1;
    ctx->exit_code = mem_console_app_run_loop(&ctx->loop_ctx);
    ctx->dispatch_succeeded = (ctx->exit_code == 0);

    if (ctx->prefs_path_valid && ctx->state.pane_prefs_dirty) {
        ctx->result = mem_console_prefs_save(ctx->prefs_path, &ctx->state);
        if (ctx->result.code == CORE_OK) {
            ctx->state.pane_prefs_dirty = 0;
            ctx->prefs_last_saved_signature = mem_console_prefs_state_signature(&ctx->state);
            ctx->prefs_signature_valid = 1;
        }
    }

    return mem_console_app_stage_transition(ctx,
                                            MEM_CONSOLE_APP_STAGE_RUNTIME_STARTED,
                                            MEM_CONSOLE_APP_STAGE_LOOP_COMPLETED,
                                            "mem_console_app_run_loop",
                                            __func__);
}

static void mem_console_app_shutdown(MemConsoleAppMainContext *ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->text_input_started) {
        SDL_StopTextInput();
        ctx->text_input_started = 0;
    }
    if (ctx->kernel_bridge_initialized) {
        mem_console_kernel_bridge_shutdown(&ctx->kernel_bridge);
        ctx->kernel_bridge_initialized = 0;
    }
    if (ctx->runtime_initialized) {
        mem_console_runtime_shutdown(&ctx->runtime);
        ctx->runtime_initialized = 0;
    }
    if (ctx->render_ctx_initialized) {
        kit_render_context_shutdown(&ctx->render_ctx);
        ctx->render_ctx_initialized = 0;
    }
    if (ctx->renderer_initialized) {
        vk_renderer_shutdown(&ctx->renderer);
        ctx->renderer_initialized = 0;
    }
    if (ctx->window_created) {
        SDL_DestroyWindow(ctx->window);
        ctx->window = 0;
        ctx->window_created = 0;
    }
    if (ctx->sdl_initialized) {
        SDL_Quit();
        ctx->sdl_initialized = 0;
    }
    if (ctx->db_open) {
        (void)core_memdb_close(&ctx->db);
        ctx->db_open = 0;
    }

    ctx->stage = MEM_CONSOLE_APP_STAGE_SHUTDOWN_COMPLETED;
}

int mem_console_app_main(int argc, char **argv) {
    MemConsoleAppMainContext ctx;

    if (!mem_console_app_bootstrap(&ctx, argc, argv)) {
        mem_console_log_wrapper_error(__func__,
                                      MEM_CONSOLE_WRAPPER_ERROR_BOOTSTRAP_FAILED,
                                      MEM_CONSOLE_APP_STAGE_INIT,
                                      1);
        return 1;
    }
    if (!mem_console_app_config_load(&ctx)) {
        ctx.wrapper_error = MEM_CONSOLE_WRAPPER_ERROR_CONFIG_LOAD_FAILED;
        mem_console_log_wrapper_error(__func__, ctx.wrapper_error, ctx.stage, ctx.exit_code);
        mem_console_app_shutdown(&ctx);
        return ctx.exit_code;
    }
    if (!mem_console_app_state_seed(&ctx)) {
        ctx.wrapper_error = MEM_CONSOLE_WRAPPER_ERROR_STATE_SEED_FAILED;
        mem_console_log_wrapper_error(__func__, ctx.wrapper_error, ctx.stage, ctx.exit_code);
        mem_console_app_shutdown(&ctx);
        return ctx.exit_code;
    }
    if (!mem_console_app_subsystems_init(&ctx)) {
        ctx.wrapper_error = MEM_CONSOLE_WRAPPER_ERROR_SUBSYSTEMS_INIT_FAILED;
        mem_console_log_wrapper_error(__func__, ctx.wrapper_error, ctx.stage, ctx.exit_code);
        mem_console_app_shutdown(&ctx);
        return ctx.exit_code;
    }
    if (!mem_console_runtime_start(&ctx)) {
        ctx.wrapper_error = MEM_CONSOLE_WRAPPER_ERROR_RUNTIME_START_FAILED;
        mem_console_log_wrapper_error(__func__, ctx.wrapper_error, ctx.stage, ctx.exit_code);
        mem_console_app_shutdown(&ctx);
        return ctx.exit_code;
    }

    if (!mem_console_app_run_loop_stage(&ctx)) {
        ctx.wrapper_error = MEM_CONSOLE_WRAPPER_ERROR_RUN_LOOP_FAILED;
        if (ctx.exit_code == 0) {
            ctx.exit_code = 1;
        }
        mem_console_log_wrapper_error(__func__, ctx.wrapper_error, ctx.stage, ctx.exit_code);
        mem_console_app_shutdown(&ctx);
        return ctx.exit_code;
    }

    mem_console_app_shutdown(&ctx);
    fprintf(stderr,
            "mem_console: wrapper exit stage=%d exit_code=%d dispatch_ran=%d dispatch_ok=%d wrapper_error=%d\n",
            (int)ctx.stage,
            ctx.exit_code,
            ctx.dispatch_ran,
            ctx.dispatch_succeeded,
            (int)ctx.wrapper_error);
    return ctx.exit_code;
}
