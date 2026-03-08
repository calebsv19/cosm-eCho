#include "mem_console_ui_chrome.h"

#include "mem_console_ui_common.h"

static CoreResult draw_splitter_line(const KitRenderContext *render_ctx,
                                     KitRenderFrame *frame,
                                     KitRenderRect rect,
                                     float thickness,
                                     CoreThemeColorToken token) {
    if (!render_ctx || !frame) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid splitter draw request" };
    }
    if (rect.width <= 0.0f || rect.height <= 0.0f) {
        return core_result_ok();
    }

    if (rect.width > rect.height) {
        rect.y += (rect.height * 0.5f) - (thickness * 0.5f);
        rect.height = thickness;
    } else {
        rect.x += (rect.width * 0.5f) - (thickness * 0.5f);
        rect.width = thickness;
    }

    return mem_console_ui_push_themed_rect(render_ctx,
                                           frame,
                                           rect,
                                           0.0f,
                                           token);
}

CoreResult mem_console_ui_draw_root_chrome(const KitRenderContext *render_ctx,
                                           KitRenderFrame *frame,
                                           const MemConsoleState *state,
                                           const MemConsoleLayoutConfig *layout_cfg) {
    CoreResult result;
    KitRenderRect seam_rect;
    float seam_thickness = 1.0f;

    if (!render_ctx || !frame || !state || !layout_cfg) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid root chrome draw request" };
    }

    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             state->left_pane,
                                             0.0f,
                                             CORE_THEME_COLOR_SURFACE_1);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             state->right_pane,
                                             0.0f,
                                             CORE_THEME_COLOR_SURFACE_0);
    if (result.code != CORE_OK) {
        return result;
    }

    if (state->pane_drag_active) {
        seam_thickness = 2.0f;
    }

    seam_rect = (KitRenderRect){
        state->left_pane.x + state->left_pane.width - 0.5f,
        state->left_pane.y,
        1.0f,
        state->left_pane.height
    };
    result = draw_splitter_line(render_ctx,
                                frame,
                                seam_rect,
                                seam_thickness,
                                CORE_THEME_COLOR_SURFACE_2);
    if (result.code != CORE_OK) {
        return result;
    }

    seam_rect = (KitRenderRect){
        state->left_pane.x + state->left_pane.width,
        state->pane_right_detail.y + state->pane_right_detail.height - 0.5f,
        state->right_pane.x + state->right_pane.width - (state->left_pane.x + state->left_pane.width),
        1.0f
    };
    result = draw_splitter_line(render_ctx,
                                frame,
                                seam_rect,
                                seam_thickness,
                                CORE_THEME_COLOR_SURFACE_2);
    if (result.code != CORE_OK) {
        return result;
    }

    return core_result_ok();
}
