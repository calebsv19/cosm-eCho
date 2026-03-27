#ifndef MEM_CONSOLE_APP_INTERNAL_H
#define MEM_CONSOLE_APP_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include "kit_render.h"
#include "kit_ui.h"
#include "mem_console_db.h"
#include "mem_console_kernel_bridge.h"
#include "mem_console_runtime.h"
#include "mem_console_state.h"
#include "vk_renderer.h"

void mem_console_app_set_action_error_status(MemConsoleState *state,
                                             const char *prefix,
                                             CoreResult result);

void mem_console_app_refresh_and_report(CoreMemDb *db,
                                        MemConsoleState *state,
                                        const char *error_prefix);

CoreResult mem_console_app_switch_active_db(CoreMemDb *db,
                                            MemConsoleState *state,
                                            const char *next_db_path,
                                            const char *app_prefs_path,
                                            int app_prefs_path_valid,
                                            char *prefs_path,
                                            size_t prefs_path_cap,
                                            int *prefs_path_valid,
                                            int *prefs_signature_valid,
                                            uint64_t *prefs_last_saved_signature);

void mem_console_app_apply_compact_ui_density(KitUiContext *ui_ctx);

int mem_console_app_handle_theme_shortcut(KitRenderContext *render_ctx,
                                          KitUiContext *ui_ctx,
                                          MemConsoleState *state,
                                          const char *prefs_path,
                                          SDL_Keycode keycode);

int mem_console_app_handle_font_shortcut(KitRenderContext *render_ctx,
                                         MemConsoleState *state,
                                         const char *prefs_path,
                                         SDL_Keycode keycode);

void mem_console_app_apply_pending_action(CoreMemDb *db,
                                          MemConsoleState *state,
                                          MemConsoleRuntime *runtime,
                                          MemConsoleAction action);

void mem_console_app_process_sdl_event(const SDL_Event *event,
                                       bool *running,
                                       KitRenderContext *render_ctx,
                                       KitUiContext *ui_ctx,
                                       MemConsoleState *state,
                                       const char *prefs_path,
                                       KitUiInputState *input,
                                       int *wheel_y,
                                       MemConsoleAction *keyboard_action);

typedef struct MemConsoleAppLoopContext {
    SDL_Window *window;
    VkRenderer *renderer;
    KitRenderContext *render_ctx;
    KitUiContext *ui_ctx;
    CoreMemDb *db;
    MemConsoleState *state;
    MemConsoleRuntime *runtime;
    MemConsoleKernelBridge *kernel_bridge;
    int kernel_bridge_requested;
    const char *app_prefs_path;
    int app_prefs_path_valid;
    char *prefs_path;
    size_t prefs_path_cap;
    int *prefs_path_valid;
    int *prefs_signature_valid;
    uint64_t *prefs_last_saved_signature;
} MemConsoleAppLoopContext;

int mem_console_app_run_loop(MemConsoleAppLoopContext *ctx);

#endif
