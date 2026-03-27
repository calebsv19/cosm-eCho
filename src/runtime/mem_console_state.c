#include "mem_console_state.h"
#include "mem_console_layout_config.h"
#include "mem_console_pane_layout.h"

#include <SDL2/SDL.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>






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

static int path_has_sqlite_suffix(const char *path) {
    size_t len;

    if (!path) {
        return 0;
    }
    len = strlen(path);
    return len >= 7u && strcmp(path + len - 7u, ".sqlite") == 0;
}

static void db_picker_set_text_from_path(MemConsoleState *state, const char *path) {
    size_t len;

    if (!state) {
        return;
    }

    state->db_modal_text[0] = '\0';
    if (!path || !path[0]) {
        return;
    }

    len = strlen(path);
    if (path_has_sqlite_suffix(path)) {
        len -= 7u;
    }
    if (len >= sizeof(state->db_modal_text)) {
        len = sizeof(state->db_modal_text) - 1u;
    }
    memcpy(state->db_modal_text, path, len);
    state->db_modal_text[len] = '\0';
}

static int clamp_cursor_to_text(const char *text, int cursor);

static void db_picker_normalize_selection(MemConsoleState *state) {
    int start;
    int end;

    if (!state) {
        return;
    }

    start = clamp_cursor_to_text(state->db_modal_text, state->db_modal_selection_start);
    end = clamp_cursor_to_text(state->db_modal_text, state->db_modal_selection_end);
    if (start > end) {
        int tmp = start;
        start = end;
        end = tmp;
    }
    state->db_modal_selection_start = start;
    state->db_modal_selection_end = end;
}

static void db_picker_clear_selection(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->db_modal_selection_anchor = state->db_modal_cursor;
    state->db_modal_selection_start = state->db_modal_cursor;
    state->db_modal_selection_end = state->db_modal_cursor;
}

static void db_picker_delete_selection(MemConsoleState *state) {
    size_t len;

    if (!state) {
        return;
    }
    db_picker_normalize_selection(state);
    if (state->db_modal_selection_start == state->db_modal_selection_end) {
        return;
    }

    len = strlen(state->db_modal_text);
    memmove(state->db_modal_text + state->db_modal_selection_start,
            state->db_modal_text + state->db_modal_selection_end,
            len - (size_t)state->db_modal_selection_end + 1u);
    state->db_modal_cursor = state->db_modal_selection_start;
    db_picker_clear_selection(state);
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


static int clamp_cursor_to_text(const char *text, int cursor) {
    int len;

    if (!text) {
        return 0;
    }

    len = (int)strlen(text);
    if (cursor < 0) return 0;
    if (cursor > len) return len;
    return cursor;
}

static void resolve_active_input_buffer(MemConsoleState *state,
                                        char **out_text,
                                        size_t *out_cap,
                                        int **out_cursor) {
    if (!state || !out_text || !out_cap || !out_cursor) {
        return;
    }

    if (state->input_target == MEM_CONSOLE_INPUT_TITLE_EDIT) {
        *out_text = state->title_edit_text;
        *out_cap = sizeof(state->title_edit_text);
        *out_cursor = &state->title_edit_cursor;
    } else if (state->input_target == MEM_CONSOLE_INPUT_BODY_EDIT) {
        *out_text = state->body_edit_text;
        *out_cap = sizeof(state->body_edit_text);
        *out_cursor = &state->body_edit_cursor;
    } else if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH) {
        *out_text = state->db_modal_text;
        *out_cap = sizeof(state->db_modal_text);
        *out_cursor = &state->db_modal_cursor;
    } else if (state->input_target == MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT) {
        *out_text = state->graph_edge_limit_text;
        *out_cap = sizeof(state->graph_edge_limit_text);
        *out_cursor = &state->graph_edge_limit_cursor;
    } else {
        *out_text = state->search_text;
        *out_cap = sizeof(state->search_text);
        *out_cursor = &state->search_cursor;
    }
}

void sync_edit_buffers_from_selection(MemConsoleState *state) {
    if (!state) {
        return;
    }
    if (!state->title_edit_mode) {
        (void)snprintf(state->title_edit_text,
                       sizeof(state->title_edit_text),
                       "%s",
                       state->selected_title);
        state->title_edit_cursor = (int)strlen(state->title_edit_text);
    }
    if (!state->body_edit_mode) {
        (void)snprintf(state->body_edit_text,
                       sizeof(state->body_edit_text),
                       "%s",
                       state->selected_body);
        state->body_edit_cursor = (int)strlen(state->body_edit_text);
    }
    state->search_cursor = clamp_cursor_to_text(state->search_text, state->search_cursor);
    state->db_modal_cursor = clamp_cursor_to_text(state->db_modal_text, state->db_modal_cursor);
    db_picker_normalize_selection(state);
    state->graph_edge_limit_cursor = clamp_cursor_to_text(state->graph_edge_limit_text,
                                                          state->graph_edge_limit_cursor);
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

void begin_db_picker_mode(MemConsoleState *state, int create_mode) {
    char default_db_path[1024];

    if (!state) {
        return;
    }

    state->title_edit_mode = 0;
    state->body_edit_mode = 0;
    state->db_modal_open = 1;
    state->db_modal_create_mode = create_mode ? 1 : 0;
    state->pending_db_path[0] = '\0';
    state->db_modal_resolved_path[0] = '\0';
    state->input_target = MEM_CONSOLE_INPUT_DB_PATH;

    if (state->db_modal_create_mode && resolve_default_db_path(default_db_path, sizeof(default_db_path))) {
        db_picker_set_text_from_path(state, default_db_path);
    } else {
        db_picker_set_text_from_path(state, state->db_path);
    }
    state->db_modal_cursor = (int)strlen(state->db_modal_text);
    state->db_modal_selection_anchor = 0;
    state->db_modal_selection_start = 0;
    state->db_modal_selection_end = state->db_modal_cursor;
    state->db_modal_drag_select_active = 0;
}

void cancel_db_picker_mode(MemConsoleState *state) {
    if (!state) {
        return;
    }

    state->db_modal_open = 0;
    state->db_modal_create_mode = 0;
    state->db_modal_text[0] = '\0';
    state->db_modal_resolved_path[0] = '\0';
    state->db_modal_visible_text[0] = '\0';
    state->db_modal_resolved_line[0] = '\0';
    state->pending_db_path[0] = '\0';
    state->db_modal_cursor = 0;
    state->db_modal_selection_anchor = 0;
    state->db_modal_selection_start = 0;
    state->db_modal_selection_end = 0;
    state->db_modal_drag_select_active = 0;
    state->input_target = MEM_CONSOLE_INPUT_SEARCH;
}

int mem_console_db_picker_build_path(const MemConsoleState *state,
                                     char *out_path,
                                     size_t out_cap) {
    const char *source = 0;
    const char *home_path = 0;
    size_t start = 0u;
    size_t end = 0u;

    if (!state || !out_path || out_cap == 0u) {
        return 0;
    }

    out_path[0] = '\0';
    source = state->db_modal_text;
    while (source[start] == ' ') {
        start += 1u;
    }
    end = strlen(source);
    while (end > start && source[end - 1u] == ' ') {
        end -= 1u;
    }
    if (end <= start) {
        return 0;
    }
    if ((end - start) >= out_cap) {
        return 0;
    }

    memcpy(out_path, source + start, end - start);
    out_path[end - start] = '\0';

    if (out_path[0] == '~' && out_path[1] == '/') {
        char expanded[1024];
        home_path = getenv("HOME");
        if (!home_path || !home_path[0]) {
            out_path[0] = '\0';
            return 0;
        }
        if (snprintf(expanded, sizeof(expanded), "%s/%s", home_path, out_path + 2) <= 0 ||
            strlen(expanded) >= sizeof(expanded) ||
            strlen(expanded) >= out_cap) {
            out_path[0] = '\0';
            return 0;
        }
        (void)snprintf(out_path, out_cap, "%s", expanded);
    }

    if (!path_has_sqlite_suffix(out_path)) {
        size_t len = strlen(out_path);
        if (len + 7u >= out_cap) {
            out_path[0] = '\0';
            return 0;
        }
        memcpy(out_path + len, ".sqlite", 8u);
    }

    return 1;
}

int mem_console_db_picker_has_selection(const MemConsoleState *state) {
    if (!state) {
        return 0;
    }
    return state->db_modal_selection_start != state->db_modal_selection_end;
}

void mem_console_db_picker_select_all(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->db_modal_cursor = (int)strlen(state->db_modal_text);
    state->db_modal_selection_anchor = 0;
    state->db_modal_selection_start = 0;
    state->db_modal_selection_end = state->db_modal_cursor;
}

void mem_console_db_picker_begin_selection(MemConsoleState *state, int cursor_index) {
    if (!state) {
        return;
    }
    state->db_modal_cursor = clamp_cursor_to_text(state->db_modal_text, cursor_index);
    state->db_modal_selection_anchor = state->db_modal_cursor;
    state->db_modal_selection_start = state->db_modal_cursor;
    state->db_modal_selection_end = state->db_modal_cursor;
    state->db_modal_drag_select_active = 1;
}

void mem_console_db_picker_update_selection(MemConsoleState *state, int cursor_index) {
    if (!state) {
        return;
    }
    state->db_modal_cursor = clamp_cursor_to_text(state->db_modal_text, cursor_index);
    state->db_modal_selection_start = state->db_modal_selection_anchor;
    state->db_modal_selection_end = state->db_modal_cursor;
    db_picker_normalize_selection(state);
}

void mem_console_db_picker_end_selection(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->db_modal_drag_select_active = 0;
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

int active_input_is_search(const MemConsoleState *state) {
    if (!state) {
        return 1;
    }
    return state->input_target == MEM_CONSOLE_INPUT_SEARCH;
}

void append_active_input_text(MemConsoleState *state, const char *text) {
    char *target = 0;
    size_t target_cap = 0;
    int *cursor_ptr = 0;
    size_t current_len;
    size_t append_len;
    size_t available;
    int cursor;

    if (!state || !text) {
        return;
    }

    if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH && mem_console_db_picker_has_selection(state)) {
        db_picker_delete_selection(state);
    }

    resolve_active_input_buffer(state, &target, &target_cap, &cursor_ptr);
    if (!target || !cursor_ptr || target_cap == 0u) {
        return;
    }

    current_len = strlen(target);
    if (current_len >= target_cap - 1u) {
        return;
    }
    cursor = clamp_cursor_to_text(target, *cursor_ptr);

    append_len = strlen(text);
    available = (target_cap - 1u) - current_len;
    if (append_len > available) {
        append_len = available;
    }
    if (append_len == 0u) {
        return;
    }

    memmove(target + cursor + (int)append_len,
            target + cursor,
            current_len - (size_t)cursor + 1u);
    memcpy(target + cursor, text, append_len);
    *cursor_ptr = cursor + (int)append_len;
    if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH) {
        db_picker_clear_selection(state);
    }
}

void erase_active_input_char(MemConsoleState *state) {
    char *target = 0;
    size_t target_cap = 0;
    int *cursor_ptr = 0;
    size_t current_len;
    int cursor;

    if (!state) {
        return;
    }

    if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH && mem_console_db_picker_has_selection(state)) {
        db_picker_delete_selection(state);
        return;
    }

    resolve_active_input_buffer(state, &target, &target_cap, &cursor_ptr);
    if (!target || !cursor_ptr || target_cap == 0u) {
        return;
    }

    current_len = strlen(target);
    if (current_len == 0u) {
        return;
    }
    cursor = clamp_cursor_to_text(target, *cursor_ptr);
    if (cursor <= 0) {
        return;
    }

    memmove(target + cursor - 1,
            target + cursor,
            current_len - (size_t)cursor + 1u);
    *cursor_ptr = cursor - 1;
    if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH) {
        db_picker_clear_selection(state);
    }
}

void delete_active_input_char(MemConsoleState *state) {
    char *target = 0;
    size_t target_cap = 0;
    int *cursor_ptr = 0;
    size_t current_len;
    int cursor;

    if (!state) {
        return;
    }

    if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH && mem_console_db_picker_has_selection(state)) {
        db_picker_delete_selection(state);
        return;
    }

    resolve_active_input_buffer(state, &target, &target_cap, &cursor_ptr);
    if (!target || !cursor_ptr || target_cap == 0u) {
        return;
    }

    current_len = strlen(target);
    if (current_len == 0u) {
        return;
    }
    cursor = clamp_cursor_to_text(target, *cursor_ptr);
    if (cursor >= (int)current_len) {
        return;
    }

    memmove(target + cursor,
            target + cursor + 1,
            current_len - (size_t)cursor);
    if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH) {
        db_picker_clear_selection(state);
    }
}

void move_active_input_cursor(MemConsoleState *state, int delta) {
    char *target = 0;
    size_t target_cap = 0;
    int *cursor_ptr = 0;

    if (!state) {
        return;
    }

    resolve_active_input_buffer(state, &target, &target_cap, &cursor_ptr);
    if (!target || !cursor_ptr || target_cap == 0u) {
        return;
    }

    *cursor_ptr = clamp_cursor_to_text(target, *cursor_ptr + delta);
    if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH) {
        db_picker_clear_selection(state);
    }
}

void move_active_input_cursor_home(MemConsoleState *state) {
    char *target = 0;
    size_t target_cap = 0;
    int *cursor_ptr = 0;

    if (!state) {
        return;
    }
    resolve_active_input_buffer(state, &target, &target_cap, &cursor_ptr);
    if (!target || !cursor_ptr || target_cap == 0u) {
        return;
    }
    *cursor_ptr = 0;
    if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH) {
        db_picker_clear_selection(state);
    }
}

void move_active_input_cursor_end(MemConsoleState *state) {
    char *target = 0;
    size_t target_cap = 0;
    int *cursor_ptr = 0;

    if (!state) {
        return;
    }
    resolve_active_input_buffer(state, &target, &target_cap, &cursor_ptr);
    if (!target || !cursor_ptr || target_cap == 0u) {
        return;
    }
    *cursor_ptr = (int)strlen(target);
    if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH) {
        db_picker_clear_selection(state);
    }
}
