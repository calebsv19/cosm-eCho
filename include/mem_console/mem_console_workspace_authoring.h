#ifndef MEM_CONSOLE_WORKSPACE_AUTHORING_H
#define MEM_CONSOLE_WORKSPACE_AUTHORING_H

#include <stdint.h>

#include <SDL2/SDL.h>

#include "kit_render.h"
#include "kit_ui.h"
#include "kit_workspace_authoring.h"
#include "kit_workspace_authoring_ui.h"

struct MemConsoleState;

typedef enum MemConsoleWorkspaceAuthoringOverlayMode {
    MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES = 0,
    MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_FONT_THEME = 1
} MemConsoleWorkspaceAuthoringOverlayMode;

typedef struct MemConsoleWorkspaceAuthoringHost {
    uint8_t active;
    uint8_t key_c_down;
    uint8_t key_v_down;
    uint8_t last_event_consumed;
    uint8_t last_event_entered;
    uint8_t last_event_exited;
    uint8_t last_event_accepted;
    uint8_t last_event_canceled;
    uint8_t font_theme_baseline_valid;
    uint8_t font_theme_pending_changes;
    uint8_t entry_chord_armed_key;
    MemConsoleWorkspaceAuthoringOverlayMode overlay_mode;
    uint32_t viewport_width;
    uint32_t viewport_height;
    uint32_t consumed_event_count;
    uint32_t captured_runtime_event_count;
    uint32_t overlay_cycle_count;
    uint32_t overlay_button_click_count;
    uint32_t font_theme_button_click_count;
    uint32_t add_stub_count;
    uint32_t last_overlay_button_id;
    uint32_t last_font_theme_button_id;
    int baseline_text_zoom_step;
    int baseline_theme_preset_id;
    int baseline_font_preset_id;
    char status_text[160];
    char text_size_line[96];
    char font_preset_line[96];
    char theme_preset_line[96];
    char custom_preset_line[160];
} MemConsoleWorkspaceAuthoringHost;

void mem_console_workspace_authoring_host_reset(MemConsoleWorkspaceAuthoringHost *host);
void mem_console_workspace_authoring_host_set_viewport(MemConsoleWorkspaceAuthoringHost *host,
                                                       uint32_t width,
                                                       uint32_t height);
int mem_console_workspace_authoring_host_active(const MemConsoleWorkspaceAuthoringHost *host);
int mem_console_workspace_authoring_host_pane_overlay_active(const MemConsoleWorkspaceAuthoringHost *host);
int mem_console_workspace_authoring_host_font_theme_overlay_active(const MemConsoleWorkspaceAuthoringHost *host);
int mem_console_workspace_authoring_host_handle_sdl_event(MemConsoleWorkspaceAuthoringHost *host,
                                                          struct MemConsoleState *state,
                                                          KitRenderContext *render_ctx,
                                                          KitUiContext *ui_ctx,
                                                          const SDL_Event *event,
                                                          int text_entry_active);
void mem_console_workspace_authoring_host_cancel_active_preview(MemConsoleWorkspaceAuthoringHost *host,
                                                               struct MemConsoleState *state,
                                                               KitRenderContext *render_ctx,
                                                               KitUiContext *ui_ctx);

CoreResult mem_console_workspace_authoring_overlay_render(KitRenderContext *render_ctx,
                                                          KitUiContext *ui_ctx,
                                                          KitRenderFrame *frame,
                                                          struct MemConsoleState *state,
                                                          int frame_width,
                                                          int frame_height);

#endif
