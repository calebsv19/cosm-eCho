#include "mem_console_state.h"

#include <dirent.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int path_has_sqlite_suffix(const char *path) {
    size_t len;

    if (!path) {
        return 0;
    }
    len = strlen(path);
    return len >= 7u && strcmp(path + len - 7u, ".sqlite") == 0;
}

static int path_has_directory_separator(const char *path) {
    if (!path || !path[0]) {
        return 0;
    }
    return strchr(path, '/') != 0;
}

static void sanitize_db_label(const char *src, char *dst, size_t dst_cap) {
    size_t read_i = 0u;
    size_t write_i = 0u;

    if (!dst || dst_cap == 0u) {
        return;
    }
    dst[0] = '\0';
    if (!src) {
        return;
    }

    while (src[read_i] != '\0' && write_i + 1u < dst_cap) {
        unsigned char c = (unsigned char)src[read_i];
        dst[write_i] = (c >= 32u && c <= 126u) ? (char)c : '_';
        read_i += 1u;
        write_i += 1u;
    }
    dst[write_i] = '\0';
}

static void db_picker_set_text_from_path(MemConsoleState *state,
                                         const char *path,
                                         int strip_sqlite_suffix) {
    size_t len;

    if (!state) {
        return;
    }

    state->db_modal_text[0] = '\0';
    if (!path || !path[0]) {
        return;
    }

    len = strlen(path);
    if (strip_sqlite_suffix && path_has_sqlite_suffix(path)) {
        len -= 7u;
    }
    if (len >= sizeof(state->db_modal_text)) {
        len = sizeof(state->db_modal_text) - 1u;
    }
    memcpy(state->db_modal_text, path, len);
    state->db_modal_text[len] = '\0';
}

static void db_picker_set_text_from_selected_entry(MemConsoleState *state) {
    if (!state) {
        return;
    }
    if (state->db_picker_selected_index < 0 || state->db_picker_selected_index >= state->db_picker_entry_count) {
        return;
    }
    db_picker_set_text_from_path(state,
                                 state->db_picker_entry_paths[state->db_picker_selected_index],
                                 state->db_modal_create_mode ? 1 : 0);
    state->db_modal_cursor = (int)strlen(state->db_modal_text);
    state->db_modal_selection_anchor = 0;
    state->db_modal_selection_start = 0;
    state->db_modal_selection_end = state->db_modal_cursor;
    state->db_modal_drag_select_active = 0;
}

static int clamp_cursor_to_text(const char *text, int cursor) {
    int len;

    if (!text) {
        return 0;
    }

    len = (int)strlen(text);
    if (cursor < 0) {
        return 0;
    }
    if (cursor > len) {
        return len;
    }
    return cursor;
}

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

void begin_db_picker_mode(MemConsoleState *state, int create_mode) {
    char default_db_path[1024];

    if (!state) {
        return;
    }

    state->title_edit_mode = 0;
    state->body_edit_mode = 0;
    state->db_modal_open = 1;
    state->db_modal_create_mode = create_mode ? 1 : 0;
    state->db_modal_input_root_mode = 0;
    state->pending_db_path[0] = '\0';
    state->db_modal_resolved_path[0] = '\0';
    state->input_target = MEM_CONSOLE_INPUT_DB_PATH;

    mem_console_db_picker_rescan_entries(state);
    if (!state->db_modal_create_mode && state->db_picker_entry_count > 0) {
        db_picker_set_text_from_selected_entry(state);
    } else if (state->db_modal_create_mode) {
        if (state->input_root[0] &&
            snprintf(default_db_path, sizeof(default_db_path), "%s/default.sqlite", state->input_root) > 0 &&
            strlen(default_db_path) < sizeof(default_db_path)) {
            db_picker_set_text_from_path(state, default_db_path, 1);
        } else if (resolve_default_db_path(default_db_path, sizeof(default_db_path))) {
            db_picker_set_text_from_path(state, default_db_path, 1);
        } else {
            db_picker_set_text_from_path(state, state->db_path, 1);
        }
    } else {
        db_picker_set_text_from_path(state, state->db_path, 0);
    }
    state->db_modal_cursor = (int)strlen(state->db_modal_text);
    state->db_modal_selection_anchor = 0;
    state->db_modal_selection_start = 0;
    state->db_modal_selection_end = state->db_modal_cursor;
    state->db_modal_drag_select_active = 0;
}

void begin_input_root_picker_mode(MemConsoleState *state) {
    if (!state) {
        return;
    }

    state->title_edit_mode = 0;
    state->body_edit_mode = 0;
    state->db_modal_open = 1;
    state->db_modal_create_mode = 0;
    state->db_modal_input_root_mode = 1;
    state->pending_db_path[0] = '\0';
    state->db_modal_resolved_path[0] = '\0';
    state->input_target = MEM_CONSOLE_INPUT_DB_PATH;
    db_picker_set_text_from_path(state, state->input_root[0] ? state->input_root : state->output_root, 0);
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
    state->db_modal_input_root_mode = 0;
    state->db_modal_text[0] = '\0';
    state->db_modal_resolved_path[0] = '\0';
    state->db_modal_visible_text[0] = '\0';
    state->db_modal_resolved_line[0] = '\0';
    state->db_modal_active_line[0] = '\0';
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

    if (state->db_modal_input_root_mode) {
        return 1;
    }

    if (!state->db_modal_create_mode) {
        return 1;
    }

    if (!path_has_directory_separator(out_path)) {
        char resolved_create_path[1024];
        const char *create_root = state->input_root[0] ? state->input_root : state->output_root;
        int written = 0;

        if (!create_root || !create_root[0]) {
            out_path[0] = '\0';
            return 0;
        }
        written = snprintf(resolved_create_path,
                           sizeof(resolved_create_path),
                           path_has_sqlite_suffix(out_path) ? "%s/%s" : "%s/%s.sqlite",
                           create_root,
                           out_path);
        if (written <= 0 || (size_t)written >= sizeof(resolved_create_path) ||
            strlen(resolved_create_path) >= out_cap) {
            out_path[0] = '\0';
            return 0;
        }
        (void)snprintf(out_path, out_cap, "%s", resolved_create_path);
        return 1;
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

void mem_console_db_picker_rescan_entries(MemConsoleState *state) {
    DIR *dir = 0;
    struct dirent *entry = 0;
    int write_index = 0;
    size_t input_root_len = 0u;

    if (!state) {
        return;
    }

    state->db_picker_entry_count = 0;
    state->db_picker_selected_index = -1;
    if (!state->input_root[0] || !mem_console_path_is_directory(state->input_root)) {
        return;
    }

    dir = opendir(state->input_root);
    if (!dir) {
        return;
    }
    input_root_len = strlen(state->input_root);

    while ((entry = readdir(dir)) != 0 && write_index < MEM_CONSOLE_DB_PICKER_LIST_LIMIT) {
        size_t name_len = 0u;
        int path_written = 0;
        int name_written = 0;

        if (entry->d_name[0] == '.') {
            continue;
        }
        name_len = strlen(entry->d_name);
        if (name_len < 7u || strcmp(entry->d_name + name_len - 7u, ".sqlite") != 0) {
            continue;
        }
        path_written = snprintf(state->db_picker_entry_paths[write_index],
                                sizeof(state->db_picker_entry_paths[write_index]),
                                (input_root_len > 0u && state->input_root[input_root_len - 1u] == '/') ? "%s%s" : "%s/%s",
                                state->input_root,
                                entry->d_name);
        if (path_written <= 0 || (size_t)path_written >= sizeof(state->db_picker_entry_paths[write_index])) {
            continue;
        }
        name_written = snprintf(state->db_picker_entry_names[write_index],
                                sizeof(state->db_picker_entry_names[write_index]),
                                "%.*s",
                                (int)(name_len - 7u),
                                entry->d_name);
        if (name_written <= 0 || (size_t)name_written >= sizeof(state->db_picker_entry_names[write_index])) {
            continue;
        }
        sanitize_db_label(state->db_picker_entry_names[write_index],
                          state->db_picker_entry_names[write_index],
                          sizeof(state->db_picker_entry_names[write_index]));
        if (state->db_path && strcmp(state->db_picker_entry_paths[write_index], state->db_path) == 0) {
            state->db_picker_selected_index = write_index;
        }
        write_index += 1;
    }

    closedir(dir);
    state->db_picker_entry_count = write_index;
    if (state->db_picker_entry_count > 0 && state->db_picker_selected_index < 0) {
        state->db_picker_selected_index = 0;
    }
}

int mem_console_db_picker_move_selection(MemConsoleState *state, int delta) {
    int next_index = 0;

    if (!state || state->db_picker_entry_count <= 0 || delta == 0) {
        return 0;
    }
    if (state->db_picker_selected_index < 0 || state->db_picker_selected_index >= state->db_picker_entry_count) {
        state->db_picker_selected_index = 0;
    }
    next_index = state->db_picker_selected_index + delta;
    if (next_index < 0) {
        next_index = 0;
    }
    if (next_index >= state->db_picker_entry_count) {
        next_index = state->db_picker_entry_count - 1;
    }
    if (next_index == state->db_picker_selected_index) {
        return 0;
    }
    state->db_picker_selected_index = next_index;
    db_picker_set_text_from_selected_entry(state);
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
