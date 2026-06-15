#include "mem_console_ui_left_browse_filters.h"

#include "mem_console_ui_common.h"

#include <stdio.h>

static CoreResult draw_browse_filter_button(KitUiContext *ui_ctx,
                                            KitRenderFrame *frame,
                                            const KitUiInputState *input,
                                            KitRenderRect rect,
                                            const char *label,
                                            int active,
                                            int enabled,
                                            int widget_id,
                                            MemConsoleAction action,
                                            MemConsoleAction *io_action) {
    KitUiButtonResult button = kit_ui_eval_button(rect, input, enabled);
    KitUiWidgetState draw_state = button.state;

    if (active) {
        draw_state = KIT_UI_STATE_ACTIVE;
    }
    if (button.clicked && io_action && *io_action == MEM_CONSOLE_ACTION_NONE) {
        *io_action = action;
    }
    (void)widget_id;
    return mem_console_ui_draw_button_custom(ui_ctx,
                                             frame,
                                             rect,
                                             label,
                                             draw_state,
                                             CORE_FONT_ROLE_UI_MEDIUM,
                                             CORE_FONT_TEXT_SIZE_CAPTION);
}

CoreResult mem_console_ui_left_draw_browse_filters(KitRenderContext *render_ctx,
                                                   KitUiContext *ui_ctx,
                                                   KitRenderFrame *frame,
                                                   MemConsoleState *state,
                                                   const KitUiInputState *input,
                                                   KitRenderRect bounds,
                                                   int controls_enabled,
                                                   MemConsoleAction *io_action) {
    const float gap = 6.0f;
    KitRenderRect inner;
    KitRenderRect row;
    KitRenderRect summary;
    KitRenderRect pin_rect;
    KitRenderRect can_rect;
    KitRenderRect kind_rect;
    float button_w;
    const char *kind;
    char kind_label[64];
    CoreResult result;

    if (!render_ctx || !ui_ctx || !frame || !state || !input || !io_action) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid browse filter draw request" };
    }

    result = mem_console_ui_push_themed_rect(render_ctx,
                                             frame,
                                             bounds,
                                             8.0f,
                                             CORE_THEME_COLOR_SURFACE_0);
    if (result.code != CORE_OK) {
        return result;
    }

    inner = (KitRenderRect){
        bounds.x + 4.0f,
        bounds.y + 3.0f,
        bounds.width - 8.0f,
        bounds.height - 6.0f
    };
    row = (KitRenderRect){
        inner.x,
        inner.y,
        inner.width,
        22.0f
    };
    summary = (KitRenderRect){
        inner.x,
        row.y + row.height + 2.0f,
        inner.width,
        inner.height - row.height - 2.0f
    };

    if (row.width < 1.0f) {
        return core_result_ok();
    }
    button_w = (row.width - (gap * 2.0f)) / 3.0f;
    if (button_w < 42.0f) {
        button_w = row.width / 3.0f;
    }
    pin_rect = (KitRenderRect){ row.x, row.y, button_w, row.height };
    can_rect = (KitRenderRect){ pin_rect.x + pin_rect.width + gap, row.y, button_w, row.height };
    kind_rect = (KitRenderRect){ can_rect.x + can_rect.width + gap, row.y, row.x + row.width - (can_rect.x + can_rect.width + gap), row.height };
    if (kind_rect.width < 0.0f) {
        kind_rect.width = 0.0f;
    }

    kind = mem_console_browse_kind_for_index(state->browse_kind_index);
    (void)snprintf(kind_label, sizeof(kind_label), "KIND %s", kind[0] ? kind : "ALL");

    result = draw_browse_filter_button(ui_ctx,
                                       frame,
                                       input,
                                       pin_rect,
                                       "PINNED",
                                       state->browse_pinned_only,
                                       controls_enabled,
                                       1201,
                                       MEM_CONSOLE_ACTION_TOGGLE_BROWSE_PINNED,
                                       io_action);
    if (result.code != CORE_OK) return result;
    result = draw_browse_filter_button(ui_ctx,
                                       frame,
                                       input,
                                       can_rect,
                                       "CANON",
                                       state->browse_canonical_only,
                                       controls_enabled,
                                       1202,
                                       MEM_CONSOLE_ACTION_TOGGLE_BROWSE_CANONICAL,
                                       io_action);
    if (result.code != CORE_OK) return result;
    result = draw_browse_filter_button(ui_ctx,
                                       frame,
                                       input,
                                       kind_rect,
                                       kind_label,
                                       state->browse_kind_index != 0,
                                       controls_enabled,
                                       1203,
                                       MEM_CONSOLE_ACTION_CYCLE_BROWSE_KIND,
                                       io_action);
    if (result.code != CORE_OK) return result;

    (void)snprintf(state->browse_filter_summary_line,
                   sizeof(state->browse_filter_summary_line),
                   "Browse: %s%s%s%s",
                   state->browse_pinned_only ? "pinned" : "all",
                   state->browse_canonical_only ? " | canonical" : "",
                   kind[0] ? " | kind=" : "",
                   kind[0] ? kind : "");
    return mem_console_ui_draw_info_line_custom(ui_ctx,
                                                frame,
                                                summary,
                                                state->browse_filter_summary_line,
                                                CORE_THEME_COLOR_TEXT_MUTED,
                                                CORE_FONT_ROLE_UI_REGULAR,
                                                CORE_FONT_TEXT_SIZE_CAPTION);
}
