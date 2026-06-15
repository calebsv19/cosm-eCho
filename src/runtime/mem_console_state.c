#include "mem_console_state.h"
#include "mem_console_layout_config.h"
#include "mem_console_pane_layout.h"

#include <stdio.h>
#include <string.h>

static void append_redraw_reason(char *out_text,
                                 size_t out_cap,
                                 const char *reason_label,
                                 int *io_any_written) {
    size_t current_len;
    int written;

    if (!out_text || out_cap == 0u || !reason_label || !reason_label[0] || !io_any_written) {
        return;
    }

    current_len = strlen(out_text);
    if (current_len >= out_cap - 1u) {
        return;
    }

    written = snprintf(out_text + current_len,
                       out_cap - current_len,
                       "%s%s",
                       *io_any_written ? "|" : "",
                       reason_label);
    if (written > 0) {
        *io_any_written = 1;
    }
}

void mem_console_redraw_mark(MemConsoleState *state, uint32_t reasons) {
    if (!state || reasons == 0u) {
        return;
    }
    state->redraw_pending_reasons |= reasons;
}

uint32_t mem_console_redraw_pending(const MemConsoleState *state) {
    if (!state) {
        return 0u;
    }
    return state->redraw_pending_reasons;
}

uint32_t mem_console_redraw_take_pending(MemConsoleState *state) {
    uint32_t reasons;

    if (!state) {
        return 0u;
    }

    reasons = state->redraw_pending_reasons;
    state->redraw_pending_reasons = 0u;
    return reasons;
}

void mem_console_redraw_note_frame(MemConsoleState *state, uint32_t reasons, uint64_t now_ms) {
    char reason_text[48];
    int any_reason = 0;

    if (!state) {
        return;
    }

    reason_text[0] = '\0';
    if (reasons == MEM_CONSOLE_REDRAW_REASON_NONE) {
        (void)snprintf(reason_text, sizeof(reason_text), "idle");
    } else {
        if ((reasons & MEM_CONSOLE_REDRAW_REASON_INPUT) != 0u) {
            append_redraw_reason(reason_text, sizeof(reason_text), "input", &any_reason);
        }
        if ((reasons & MEM_CONSOLE_REDRAW_REASON_LAYOUT) != 0u) {
            append_redraw_reason(reason_text, sizeof(reason_text), "layout", &any_reason);
        }
        if ((reasons & MEM_CONSOLE_REDRAW_REASON_THEME) != 0u) {
            append_redraw_reason(reason_text, sizeof(reason_text), "theme", &any_reason);
        }
        if ((reasons & MEM_CONSOLE_REDRAW_REASON_CONTENT) != 0u) {
            append_redraw_reason(reason_text, sizeof(reason_text), "content", &any_reason);
        }
        if ((reasons & MEM_CONSOLE_REDRAW_REASON_BACKGROUND) != 0u) {
            append_redraw_reason(reason_text, sizeof(reason_text), "background", &any_reason);
        }
        if (!any_reason) {
            (void)snprintf(reason_text, sizeof(reason_text), "other");
        }
    }

    state->redraw_frame_count += 1u;
    state->redraw_last_reasons = reasons;
    state->redraw_last_frame_ms = now_ms;
    (void)snprintf(state->redraw_summary_line,
                   sizeof(state->redraw_summary_line),
                   "Render #%llu %s",
                   (unsigned long long)state->redraw_frame_count,
                   reason_text);
}

void compute_layout(MemConsoleState *state, int frame_width, int frame_height) {
    const MemConsoleLayoutConfig *layout_cfg = mem_console_layout_config_get();
    CoreResult result;

    if (!state) {
        return;
    }

    result = mem_console_pane_layout_compute(state, layout_cfg, frame_width, frame_height);
    if (result.code != CORE_OK) {
        float outer_margin = layout_cfg->outer_margin;
        float pane_gap = layout_cfg->pane_gap;
        float left_width = layout_cfg->left_pane_width;
        float pane_height;

        if (frame_width < layout_cfg->min_frame_width) {
            frame_width = layout_cfg->min_frame_width;
        }
        if (frame_height < layout_cfg->min_frame_height) {
            frame_height = layout_cfg->min_frame_height;
        }
        pane_height = (float)frame_height - (outer_margin * 2.0f);
        state->left_pane = (KitRenderRect){ outer_margin, outer_margin, left_width, pane_height };
        state->right_pane = (KitRenderRect){
            state->left_pane.x + state->left_pane.width + pane_gap,
            outer_margin,
            (float)frame_width - (state->left_pane.x + state->left_pane.width + pane_gap + outer_margin),
            pane_height
        };
        state->pane_right_detail = state->right_pane;
        state->pane_right_detail_meta = state->right_pane;
        state->pane_right_detail_connections = state->right_pane;
        state->pane_right_detail_body = state->right_pane;
        state->pane_right_graph = state->right_pane;
    }
}

static int estimate_char_width_px(CoreFontTextSizeTier text_tier) {
    switch (text_tier) {
        case CORE_FONT_TEXT_SIZE_HEADER:
            return 13;
        case CORE_FONT_TEXT_SIZE_TITLE:
            return 11;
        case CORE_FONT_TEXT_SIZE_PARAGRAPH:
            return 9;
        case CORE_FONT_TEXT_SIZE_CAPTION:
            return 8;
        case CORE_FONT_TEXT_SIZE_BASIC:
        default:
            return 9;
    }
}

void format_text_for_width(char *out_text,
                           size_t out_cap,
                           const char *source_text,
                           float width_px,
                           CoreFontTextSizeTier text_tier) {
    size_t source_len;
    int char_width;
    int max_chars;
    size_t keep_len;

    if (!out_text || out_cap == 0u) {
        return;
    }
    out_text[0] = '\0';

    if (!source_text) {
        return;
    }

    char_width = estimate_char_width_px(text_tier);
    if (char_width < 1) {
        char_width = 8;
    }
    max_chars = (int)(width_px / (float)char_width);
    if (max_chars < 4) {
        max_chars = 4;
    }

    source_len = strlen(source_text);
    if ((int)source_len <= max_chars) {
        (void)snprintf(out_text, out_cap, "%s", source_text);
        return;
    }

    keep_len = (size_t)(max_chars - 3);
    if (keep_len >= out_cap) {
        keep_len = out_cap - 1u;
    }

    if (keep_len > 0u) {
        memcpy(out_text, source_text, keep_len);
    }

    if (keep_len + 3u < out_cap) {
        memcpy(out_text + keep_len, "...", 3u);
        out_text[keep_len + 3u] = '\0';
        return;
    }

    out_text[out_cap - 1u] = '\0';
}

void mem_console_select_item_for_navigation(MemConsoleState *state,
                                            int64_t item_id,
                                            int enable_graph_mode,
                                            int reset_graph_viewport,
                                            MemConsoleAction *io_action) {
    if (!state || item_id == 0) {
        return;
    }

    state->selected_item_id = item_id;
    state->graph_center_item_id = item_id;
    state->selected_created_ns = 0;
    state->title_edit_mode = 0;
    state->body_edit_mode = 0;
    state->detail_connection_scroll = 0.0f;
    state->relationship_action_link_id = 0;
    state->input_target = MEM_CONSOLE_INPUT_SEARCH;
    if (enable_graph_mode) {
        state->graph_mode_enabled = 1;
    }
    if (reset_graph_viewport) {
        mem_console_graph_view_mode_reset_viewport(state);
    }
    state->graph_layout_valid = 0;

    if (io_action && *io_action == MEM_CONSOLE_ACTION_NONE) {
        *io_action = MEM_CONSOLE_ACTION_REFRESH;
    }
}

void begin_title_edit_mode(MemConsoleState *state) {
    if (!state || state->selected_item_id == 0) {
        return;
    }
    state->body_edit_mode = 0;
    state->title_edit_mode = 1;
    state->input_target = MEM_CONSOLE_INPUT_TITLE_EDIT;
    (void)snprintf(state->title_edit_text,
                   sizeof(state->title_edit_text),
                   "%s",
                   state->selected_title);
    state->title_edit_cursor = (int)strlen(state->title_edit_text);
}

void cancel_title_edit_mode(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->title_edit_mode = 0;
    state->input_target = MEM_CONSOLE_INPUT_SEARCH;
    (void)snprintf(state->title_edit_text,
                   sizeof(state->title_edit_text),
                   "%s",
                   state->selected_title);
    state->title_edit_cursor = (int)strlen(state->title_edit_text);
}

void begin_body_edit_mode(MemConsoleState *state) {
    if (!state || state->selected_item_id == 0) {
        return;
    }
    state->title_edit_mode = 0;
    state->body_edit_mode = 1;
    state->input_target = MEM_CONSOLE_INPUT_BODY_EDIT;
    (void)snprintf(state->body_edit_text,
                   sizeof(state->body_edit_text),
                   "%s",
                   state->selected_body);
    state->body_edit_cursor = (int)strlen(state->body_edit_text);
}

void cancel_body_edit_mode(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->body_edit_mode = 0;
    state->input_target = MEM_CONSOLE_INPUT_SEARCH;
    (void)snprintf(state->body_edit_text,
                   sizeof(state->body_edit_text),
                   "%s",
                   state->selected_body);
    state->body_edit_cursor = (int)strlen(state->body_edit_text);
}
