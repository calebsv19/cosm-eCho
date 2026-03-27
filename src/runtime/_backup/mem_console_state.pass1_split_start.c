#include "mem_console_state.h"
#include "mem_console_layout_config.h"
#include "mem_console_pane_layout.h"

#include <SDL2/SDL.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

const char *k_mem_console_default_db_path =
    "mem_console/data/default.sqlite";

int mem_console_path_is_directory(const char *path) {
    struct stat st;

    if (!path || !path[0]) {
        return 0;
    }
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

int mem_console_path_exists(const char *path) {
    struct stat st;

    if (!path || !path[0]) {
        return 0;
    }
    return stat(path, &st) == 0 ? 1 : 0;
}

int mem_console_ensure_directory(const char *path) {
    if (!path || !path[0]) {
        return 0;
    }
    if (mem_console_path_is_directory(path)) {
        return 1;
    }
    if (mkdir(path, 0755) == 0) {
        return 1;
    }
    return errno == EEXIST && mem_console_path_is_directory(path);
}

int mem_console_ensure_parent_directory(const char *path) {
    char buffer[1024];
    char *slash = 0;

    if (!path || !path[0]) {
        return 0;
    }

    if (strlen(path) >= sizeof(buffer)) {
        return 0;
    }

    (void)snprintf(buffer, sizeof(buffer), "%s", path);
    slash = strrchr(buffer, '/');
    if (!slash) {
        return 1;
    }

    while (slash > buffer && *slash == '/') {
        *slash = '\0';
        slash -= 1;
    }
    if (buffer[0] == '\0') {
        return 1;
    }

    {
        char partial[1024];
        size_t index = 0u;
        size_t length = strlen(buffer);

        memset(partial, 0, sizeof(partial));
        if (buffer[0] == '/') {
            partial[0] = '/';
            index = 1u;
        }

        while (index < length) {
            size_t next_index = index;
            size_t partial_len;

            while (buffer[next_index] != '\0' && buffer[next_index] != '/') {
                next_index += 1u;
            }

            partial_len = strlen(partial);
            if (partial_len > 0u && partial[partial_len - 1u] != '/') {
                if (partial_len + 1u >= sizeof(partial)) {
                    return 0;
                }
                partial[partial_len++] = '/';
                partial[partial_len] = '\0';
            }

            if (partial_len + (next_index - index) >= sizeof(partial)) {
                return 0;
            }
            memcpy(partial + partial_len, buffer + index, next_index - index);
            partial[partial_len + (next_index - index)] = '\0';

            if (!mem_console_ensure_directory(partial)) {
                return 0;
            }

            if (buffer[next_index] == '\0') {
                break;
            }
            index = next_index + 1u;
            while (buffer[index] == '/') {
                index += 1u;
            }
        }
    }

    return 1;
}

int mem_console_resolve_app_data_dir(char *out_path, size_t out_cap) {
    const char *home_path = 0;
    char *base_path = 0;
    int written = 0;

    if (!out_path || out_cap == 0u) {
        return 0;
    }

    out_path[0] = '\0';
    home_path = getenv("HOME");
    if (home_path && home_path[0]) {
        written = snprintf(out_path, out_cap, "%s/.local/share/mem_console", home_path);
        if (written > 0 && (size_t)written < out_cap) {
            return 1;
        }
        out_path[0] = '\0';
    }

    base_path = SDL_GetBasePath();
    if (!base_path) {
        written = snprintf(out_path, out_cap, "mem_console/data");
        return written > 0 && (size_t)written < out_cap;
    }

    written = snprintf(out_path, out_cap, "%s../data", base_path);
    SDL_free(base_path);
    return written > 0 && (size_t)written < out_cap;
}

static const CoreThemePresetId k_mem_console_theme_cycle_order[] = {
    CORE_THEME_PRESET_DAW_DEFAULT,
    CORE_THEME_PRESET_MAP_FORGE_DEFAULT,
    CORE_THEME_PRESET_DARK_DEFAULT,
    CORE_THEME_PRESET_LIGHT_DEFAULT,
    CORE_THEME_PRESET_IDE_GRAY,
    CORE_THEME_PRESET_GREYSCALE
};

static const CoreFontPresetId k_mem_console_font_cycle_order[] = {
    CORE_FONT_PRESET_DAW_DEFAULT,
    CORE_FONT_PRESET_IDE
};

typedef struct MemConsoleGraphKindBitEntry {
    const char *kind;
    uint32_t bit;
} MemConsoleGraphKindBitEntry;

static const MemConsoleGraphKindBitEntry k_graph_kind_bit_entries[] = {
    { "supports", 1u << 0 },
    { "depends_on", 1u << 1 },
    { "references", 1u << 2 },
    { "summarizes", 1u << 3 },
    { "related", 1u << 4 },
    { "implements", 1u << 5 },
    { "blocks", 1u << 6 },
    { "contradicts", 1u << 7 }
};

typedef struct MemConsoleNodeKindBitEntry {
    const char *kind;
    uint32_t bit;
} MemConsoleNodeKindBitEntry;

static const MemConsoleNodeKindBitEntry k_node_kind_bit_entries[] = {
    { "plan", 1u << 0 },
    { "decision", 1u << 1 },
    { "issue", 1u << 2 },
    { "scope", 1u << 3 },
    { "summary", 1u << 4 },
    { "policy", 1u << 5 },
    { "runtime", 1u << 6 }
};

int resolve_default_db_path(char *out_path, size_t out_cap) {
    const char *env_db_path = 0;
    char data_dir[1024];
    int written = 0;

    if (!out_path || out_cap == 0u) {
        return 0;
    }

    out_path[0] = '\0';
    env_db_path = getenv("CODEWORK_MEMDB_PATH");
    if (env_db_path && env_db_path[0]) {
        written = snprintf(out_path, out_cap, "%s", env_db_path);
        return written > 0 && (size_t)written < out_cap;
    }

    if (mem_console_resolve_app_data_dir(data_dir, sizeof(data_dir))) {
        written = snprintf(out_path, out_cap, "%s/default.sqlite", data_dir);
        if (written > 0 && (size_t)written < out_cap) {
            return 1;
        }
    }

    written = snprintf(out_path, out_cap, "%s", k_mem_console_default_db_path);
    return written > 0 && (size_t)written < out_cap;
}

void print_usage(const char *argv0) {
    fprintf(stderr, "usage: %s [--db <path>] [--kernel-bridge]\n", argv0);
}

const char *find_flag_value(int argc, char **argv, const char *flag) {
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            if ((i + 1) >= argc) {
                return 0;
            }
            return argv[i + 1];
        }
    }

    return 0;
}

int has_flag(int argc, char **argv, const char *flag) {
    int i;

    if (!flag) {
        return 0;
    }

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            return 1;
        }
    }

    return 0;
}

int has_unknown_flag(int argc, char **argv) {
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--db") == 0) {
            if ((i + 1) < argc) {
                i += 1;
                continue;
            }
            return 1;
        }
        if (strcmp(argv[i], "--kernel-bridge") == 0) {
            continue;
        }
        return 1;
    }

    return 0;
}

void set_default_detail(MemConsoleState *state) {
    if (!state) {
        return;
    }

    state->selected_item_id = 0;
    state->graph_center_item_id = 0;
    state->selected_created_ns = 0;
    state->selected_pinned = 0;
    state->selected_canonical = 0;
    (void)snprintf(state->selected_title,
                   sizeof(state->selected_title),
                   "No Matching Memory");
    (void)snprintf(state->selected_body,
                   sizeof(state->selected_body),
                   "Type to filter, or use mem_cli add to create records first.");
}

void copy_core_str(CoreStr value, char *out_text, size_t out_cap) {
    size_t copy_len;

    if (!out_text || out_cap == 0u) {
        return;
    }

    out_text[0] = '\0';
    if (!value.data || value.len == 0u) {
        return;
    }

    copy_len = value.len;
    if (copy_len >= out_cap) {
        copy_len = out_cap - 1u;
    }

    memcpy(out_text, value.data, copy_len);
    out_text[copy_len] = '\0';
}

static void sync_theme_name(MemConsoleState *state) {
    const char *name;

    if (!state) {
        return;
    }

    name = core_theme_preset_name(state->theme_preset_id);
    if (!name || !name[0]) {
        name = "unknown";
    }
    (void)snprintf(state->theme_name, sizeof(state->theme_name), "%s", name);
}

int cycle_theme_preset(MemConsoleState *state, int direction) {
    size_t i;
    size_t count;

    if (!state) {
        return 0;
    }

    count = sizeof(k_mem_console_theme_cycle_order) / sizeof(k_mem_console_theme_cycle_order[0]);
    if (count == 0u) {
        return 0;
    }

    for (i = 0; i < count; ++i) {
        if (k_mem_console_theme_cycle_order[i] == state->theme_preset_id) {
            if (direction >= 0) {
                state->theme_preset_id = k_mem_console_theme_cycle_order[(i + 1u) % count];
            } else {
                state->theme_preset_id = k_mem_console_theme_cycle_order[(i + count - 1u) % count];
            }
            sync_theme_name(state);
            return 1;
        }
    }

    state->theme_preset_id = k_mem_console_theme_cycle_order[0];
    sync_theme_name(state);
    return 1;
}

int state_set_theme_preset(MemConsoleState *state, CoreThemePresetId preset_id) {
    CoreThemePreset preset;
    CoreResult result;

    if (!state) {
        return 0;
    }

    result = core_theme_get_preset(preset_id, &preset);
    if (result.code != CORE_OK) {
        return 0;
    }

    state->theme_preset_id = preset_id;
    sync_theme_name(state);
    return 1;
}

static void sync_font_name(MemConsoleState *state) {
    const char *name;

    if (!state) {
        return;
    }

    name = core_font_preset_name(state->font_preset_id);
    if (!name || !name[0]) {
        name = "unknown";
    }
    (void)snprintf(state->font_name, sizeof(state->font_name), "%s", name);
}

int cycle_font_preset(MemConsoleState *state, int direction) {
    size_t i;
    size_t count;

    if (!state) {
        return 0;
    }

    count = sizeof(k_mem_console_font_cycle_order) / sizeof(k_mem_console_font_cycle_order[0]);
    if (count == 0u) {
        return 0;
    }

    for (i = 0; i < count; ++i) {
        if (k_mem_console_font_cycle_order[i] == state->font_preset_id) {
            if (direction >= 0) {
                state->font_preset_id = k_mem_console_font_cycle_order[(i + 1u) % count];
            } else {
                state->font_preset_id = k_mem_console_font_cycle_order[(i + count - 1u) % count];
            }
            sync_font_name(state);
            return 1;
        }
    }

    state->font_preset_id = k_mem_console_font_cycle_order[0];
    sync_font_name(state);
    return 1;
}

int state_set_font_preset(MemConsoleState *state, CoreFontPresetId preset_id) {
    CoreFontPreset preset;
    CoreResult result;

    if (!state) {
        return 0;
    }

    result = core_font_get_preset(preset_id, &preset);
    if (result.code != CORE_OK) {
        return 0;
    }

    state->font_preset_id = preset_id;
    sync_font_name(state);
    return 1;
}

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

void seed_state(MemConsoleState *state, const char *db_path) {
    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));
    if (db_path) {
        (void)snprintf(state->db_path_storage, sizeof(state->db_path_storage), "%s", db_path);
    } else {
        state->db_path_storage[0] = '\0';
    }
    state->db_path = state->db_path_storage;
    state->theme_preset_id = CORE_THEME_PRESET_DAW_DEFAULT;
    state->font_preset_id = CORE_FONT_PRESET_IDE;
    state->search_text[0] = '\0';
    state->graph_mode_enabled = 1;
    state->list_query_offset = 0;
    state->visible_start_index = 0;
    state->project_filter_scroll = 0.0f;
    state->detail_connection_scroll = 0.0f;
    state->detail_body_scroll = 0.0f;
    state->title_edit_mode = 0;
    state->body_edit_mode = 0;
    state->db_modal_open = 0;
    state->db_modal_create_mode = 0;
    state->input_target = MEM_CONSOLE_INPUT_SEARCH;
    state->search_cursor = 0;
    state->title_edit_cursor = 0;
    state->body_edit_cursor = 0;
    state->db_modal_cursor = 0;
    state->db_modal_selection_anchor = 0;
    state->db_modal_selection_start = 0;
    state->db_modal_selection_end = 0;
    state->db_modal_drag_select_active = 0;
    state->graph_edge_limit_cursor = 0;
    state->search_refresh_pending = 0;
    state->search_last_input_ms = 0u;
    state->project_filter_option_count = 0;
    state->selected_project_count = 0;
    state->graph_kind_filter[0] = '\0';
    state->graph_kind_filter_mask = mem_console_graph_kind_filter_all_mask();
    state->graph_kind_filter_all_override = 1;
    state->graph_node_kind_filter_mask = mem_console_graph_node_kind_filter_all_mask();
    state->graph_node_kind_filter_all_override = 1;
    mem_console_graph_edge_limit_set(state, MEM_CONSOLE_GRAPH_EDGE_LIMIT_DEFAULT);
    state->graph_query_hops = MEM_CONSOLE_GRAPH_HOPS_MIN;
    state->graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_DAG;
    state->graph_sort_mode = MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST;
    state->graph_scope_full_mode_enabled = 0;
    state->graph_center_item_id = 0;
    sync_theme_name(state);
    sync_font_name(state);
    (void)snprintf(state->status_line,
                   sizeof(state->status_line),
                   "Type to search. Click a row to inspect it.");
    (void)snprintf(state->runtime_summary_line,
                   sizeof(state->runtime_summary_line),
                   "Async s0 a0 d0 e0 c0 | if=0 p=0");
    (void)snprintf(state->kernel_summary_line,
                   sizeof(state->kernel_summary_line),
                   "Kernel off");
    (void)snprintf(state->redraw_summary_line,
                   sizeof(state->redraw_summary_line),
                   "Render idle");
    (void)snprintf(state->project_filter_summary_line,
                   sizeof(state->project_filter_summary_line),
                   "Projects: all");
    set_default_detail(state);
    state->title_edit_text[0] = '\0';
    state->body_edit_text[0] = '\0';
    state->db_modal_text[0] = '\0';
    state->db_modal_resolved_path[0] = '\0';
    state->db_modal_visible_text[0] = '\0';
    state->db_modal_resolved_line[0] = '\0';
    state->pending_db_path[0] = '\0';
    state->redraw_pending_reasons = MEM_CONSOLE_REDRAW_REASON_BACKGROUND;
    state->redraw_last_reasons = MEM_CONSOLE_REDRAW_REASON_NONE;
    state->redraw_frame_count = 0u;
    state->redraw_last_frame_ms = 0u;
    state->pane_left_ratio = 0.0f;
    state->pane_right_split_ratio = 0.0f;
    state->pane_detail_split_ratio = 0.0f;
    state->pane_detail_top_split_ratio = 0.0f;
    state->pane_drag_active = 0;
    state->pane_drag_splitter_id = 0;
    state->pane_drag_anchor_x = 0.0f;
    state->pane_drag_anchor_y = 0.0f;
    state->pane_drag_start_left_ratio = 0.0f;
    state->pane_drag_start_right_ratio = 0.0f;
    state->pane_drag_start_detail_ratio = 0.0f;
    state->pane_drag_start_detail_top_ratio = 0.0f;
    state->pane_left_collapsed = 0;
    state->pane_right_detail_collapsed = 0;
    state->pane_prefs_dirty = 0;
    kit_graph_struct_viewport_default(&state->graph_viewport);
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

void build_like_pattern(const char *search_text, char *out_pattern, size_t out_cap) {
    if (!out_pattern || out_cap == 0u) {
        return;
    }

    if (!search_text || search_text[0] == '\0') {
        out_pattern[0] = '\0';
        return;
    }

    (void)snprintf(out_pattern, out_cap, "%%%s%%", search_text);
}

int mem_console_graph_edge_limit_clamp(int value) {
    if (value < MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN) {
        return MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN;
    }
    if (value > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        return MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }
    return value;
}

int mem_console_graph_hops_clamp(int value) {
    if (value < MEM_CONSOLE_GRAPH_HOPS_MIN) {
        return MEM_CONSOLE_GRAPH_HOPS_MIN;
    }
    if (value > MEM_CONSOLE_GRAPH_HOPS_MAX) {
        return MEM_CONSOLE_GRAPH_HOPS_MAX;
    }
    return value;
}

int mem_console_graph_layout_mode_clamp(int value) {
    if (value != MEM_CONSOLE_GRAPH_LAYOUT_TREE) {
        return MEM_CONSOLE_GRAPH_LAYOUT_DAG;
    }
    return MEM_CONSOLE_GRAPH_LAYOUT_TREE;
}

int mem_console_graph_sort_mode_clamp(int value) {
    if (value != MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST) {
        return MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST;
    }
    return MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST;
}

uint32_t mem_console_graph_kind_filter_all_mask(void) {
    uint32_t mask = 0u;
    uint32_t i;

    for (i = 0u; i < (uint32_t)(sizeof(k_graph_kind_bit_entries) / sizeof(k_graph_kind_bit_entries[0])); ++i) {
        mask |= k_graph_kind_bit_entries[i].bit;
    }
    return mask;
}

uint32_t mem_console_graph_kind_filter_mask_for_kind(const char *kind) {
    uint32_t i;

    if (!kind || !kind[0]) {
        return mem_console_graph_kind_filter_all_mask();
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_graph_kind_bit_entries) / sizeof(k_graph_kind_bit_entries[0])); ++i) {
        if (strcmp(kind, k_graph_kind_bit_entries[i].kind) == 0) {
            return k_graph_kind_bit_entries[i].bit;
        }
    }
    return 0u;
}

void mem_console_graph_kind_sync_text_filter(MemConsoleState *state) {
    uint32_t all_mask;
    uint32_t mask;
    uint32_t i;
    uint32_t selected_count = 0u;
    const char *selected_kind = "";

    if (!state) {
        return;
    }

    all_mask = mem_console_graph_kind_filter_all_mask();
    mask = state->graph_kind_filter_mask & all_mask;
    state->graph_kind_filter_mask = mask;

    if (state->graph_kind_filter_all_override) {
        state->graph_kind_filter[0] = '\0';
        return;
    }

    for (i = 0u; i < (uint32_t)(sizeof(k_graph_kind_bit_entries) / sizeof(k_graph_kind_bit_entries[0])); ++i) {
        if ((mask & k_graph_kind_bit_entries[i].bit) != 0u) {
            selected_count += 1u;
            selected_kind = k_graph_kind_bit_entries[i].kind;
        }
    }

    if (selected_count == 1u) {
        (void)snprintf(state->graph_kind_filter, sizeof(state->graph_kind_filter), "%s", selected_kind);
    } else {
        state->graph_kind_filter[0] = '\0';
    }
}

int mem_console_graph_kind_is_enabled(const MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;

    if (!state) {
        return 1;
    }
    if (state->graph_kind_filter_all_override) {
        return 1;
    }

    all_mask = mem_console_graph_kind_filter_all_mask();
    kind_mask = mem_console_graph_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u) {
        return (state->graph_kind_filter_mask & all_mask) == all_mask ? 1 : 0;
    }
    if (kind_mask == all_mask) {
        return (state->graph_kind_filter_mask & all_mask) == all_mask ? 1 : 0;
    }
    return (state->graph_kind_filter_mask & kind_mask) != 0u ? 1 : 0;
}

int mem_console_graph_kind_toggle_enabled(MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;
    uint32_t before_mask;

    if (!state || !kind || !kind[0]) {
        return 0;
    }

    all_mask = mem_console_graph_kind_filter_all_mask();
    kind_mask = mem_console_graph_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u || kind_mask == all_mask) {
        return 0;
    }

    before_mask = state->graph_kind_filter_mask;
    state->graph_kind_filter_mask = (state->graph_kind_filter_mask ^ kind_mask) & all_mask;
    mem_console_graph_kind_sync_text_filter(state);
    return before_mask != state->graph_kind_filter_mask ? 1 : 0;
}

void mem_console_graph_kind_select_all(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->graph_kind_filter_all_override = 1;
    mem_console_graph_kind_sync_text_filter(state);
}

int mem_console_graph_kind_toggle_all_override(MemConsoleState *state) {
    int before;

    if (!state) {
        return 0;
    }
    before = state->graph_kind_filter_all_override;
    state->graph_kind_filter_all_override = state->graph_kind_filter_all_override ? 0 : 1;
    mem_console_graph_kind_sync_text_filter(state);
    return before != state->graph_kind_filter_all_override ? 1 : 0;
}

void mem_console_graph_kind_set_single(MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;

    if (!state) {
        return;
    }

    all_mask = mem_console_graph_kind_filter_all_mask();
    if (!kind || !kind[0]) {
        state->graph_kind_filter_all_override = 1;
        mem_console_graph_kind_sync_text_filter(state);
        return;
    }

    kind_mask = mem_console_graph_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u || kind_mask == all_mask) {
        state->graph_kind_filter_all_override = 1;
    } else {
        state->graph_kind_filter_mask = kind_mask;
        state->graph_kind_filter_all_override = 0;
    }
    mem_console_graph_kind_sync_text_filter(state);
}

uint32_t mem_console_graph_node_kind_filter_all_mask(void) {
    uint32_t mask = 0u;
    uint32_t i;

    for (i = 0u; i < (uint32_t)(sizeof(k_node_kind_bit_entries) / sizeof(k_node_kind_bit_entries[0])); ++i) {
        mask |= k_node_kind_bit_entries[i].bit;
    }
    return mask;
}

uint32_t mem_console_graph_node_kind_filter_mask_for_kind(const char *kind) {
    uint32_t i;

    if (!kind || !kind[0]) {
        return 0u;
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_node_kind_bit_entries) / sizeof(k_node_kind_bit_entries[0])); ++i) {
        if (strcmp(kind, k_node_kind_bit_entries[i].kind) == 0) {
            return k_node_kind_bit_entries[i].bit;
        }
    }
    return 0u;
}

int mem_console_graph_node_kind_is_enabled(const MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;

    if (!state) {
        return 1;
    }
    if (state->graph_node_kind_filter_all_override) {
        return 1;
    }

    all_mask = mem_console_graph_node_kind_filter_all_mask();
    kind_mask = mem_console_graph_node_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u) {
        return 0;
    }
    return (state->graph_node_kind_filter_mask & kind_mask & all_mask) != 0u ? 1 : 0;
}

int mem_console_graph_node_kind_toggle_enabled(MemConsoleState *state, const char *kind) {
    uint32_t all_mask;
    uint32_t kind_mask;
    uint32_t before_mask;

    if (!state || !kind || !kind[0]) {
        return 0;
    }

    all_mask = mem_console_graph_node_kind_filter_all_mask();
    kind_mask = mem_console_graph_node_kind_filter_mask_for_kind(kind);
    if (kind_mask == 0u) {
        return 0;
    }

    before_mask = state->graph_node_kind_filter_mask;
    state->graph_node_kind_filter_mask = (state->graph_node_kind_filter_mask ^ kind_mask) & all_mask;
    state->graph_node_kind_filter_all_override = 0;
    return before_mask != state->graph_node_kind_filter_mask ? 1 : 0;
}

void mem_console_graph_node_kind_select_all(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->graph_node_kind_filter_all_override = 1;
    state->graph_node_kind_filter_mask = mem_console_graph_node_kind_filter_all_mask();
}

int mem_console_graph_node_kind_toggle_all_override(MemConsoleState *state) {
    int before;

    if (!state) {
        return 0;
    }
    before = state->graph_node_kind_filter_all_override;
    state->graph_node_kind_filter_all_override = state->graph_node_kind_filter_all_override ? 0 : 1;
    if (state->graph_node_kind_filter_all_override) {
        state->graph_node_kind_filter_mask = mem_console_graph_node_kind_filter_all_mask();
    }
    return before != state->graph_node_kind_filter_all_override ? 1 : 0;
}

int mem_console_graph_edge_limit_parse(const char *text, int fallback) {
    int parsed = 0;
    int i = 0;

    if (!text || text[0] == '\0') {
        return mem_console_graph_edge_limit_clamp(fallback);
    }

    while (text[i] != '\0') {
        if (text[i] < '0' || text[i] > '9') {
            return mem_console_graph_edge_limit_clamp(fallback);
        }
        parsed = (parsed * 10) + (text[i] - '0');
        if (parsed > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
            parsed = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
            break;
        }
        i += 1;
    }

    return mem_console_graph_edge_limit_clamp(parsed);
}

void mem_console_graph_edge_limit_set(MemConsoleState *state, int value) {
    if (!state) {
        return;
    }

    state->graph_query_edge_limit = mem_console_graph_edge_limit_clamp(value);
    (void)snprintf(state->graph_edge_limit_text,
                   sizeof(state->graph_edge_limit_text),
                   "%d",
                   state->graph_query_edge_limit);
    state->graph_edge_limit_cursor = (int)strlen(state->graph_edge_limit_text);
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

int mem_console_project_filter_is_selected(const MemConsoleState *state, const char *project_key) {
    int i;

    if (!state || !project_key || project_key[0] == '\0') {
        return 0;
    }

    for (i = 0; i < state->selected_project_count; ++i) {
        if (strcmp(state->selected_project_keys[i], project_key) == 0) {
            return 1;
        }
    }
    return 0;
}

void mem_console_project_filter_clear(MemConsoleState *state) {
    int i;

    if (!state) {
        return;
    }
    for (i = 0; i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        state->selected_project_keys[i][0] = '\0';
    }
    state->selected_project_count = 0;
}

int mem_console_project_filter_toggle(MemConsoleState *state, const char *project_key) {
    int i;

    if (!state || !project_key || project_key[0] == '\0') {
        return 0;
    }

    for (i = 0; i < state->selected_project_count; ++i) {
        if (strcmp(state->selected_project_keys[i], project_key) == 0) {
            int j;
            for (j = i; j < (state->selected_project_count - 1); ++j) {
                (void)snprintf(state->selected_project_keys[j],
                               sizeof(state->selected_project_keys[j]),
                               "%s",
                               state->selected_project_keys[j + 1]);
            }
            state->selected_project_keys[state->selected_project_count - 1][0] = '\0';
            state->selected_project_count -= 1;
            return 1;
        }
    }

    if (state->selected_project_count >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
        return 0;
    }
    (void)snprintf(state->selected_project_keys[state->selected_project_count],
                   sizeof(state->selected_project_keys[state->selected_project_count]),
                   "%s",
                   project_key);
    state->selected_project_count += 1;
    return 1;
}

void mem_console_project_filter_prune_to_options(MemConsoleState *state) {
    int write_index = 0;
    int i;

    if (!state) {
        return;
    }

    for (i = 0; i < state->selected_project_count; ++i) {
        int option_index;
        int found = 0;
        const char *selected_key = state->selected_project_keys[i];

        if (!selected_key[0]) {
            continue;
        }
        for (option_index = 0; option_index < state->project_filter_option_count; ++option_index) {
            if (strcmp(selected_key, state->project_filter_keys[option_index]) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            continue;
        }
        if (write_index != i) {
            (void)snprintf(state->selected_project_keys[write_index],
                           sizeof(state->selected_project_keys[write_index]),
                           "%s",
                           selected_key);
        }
        write_index += 1;
    }

    for (i = write_index; i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        state->selected_project_keys[i][0] = '\0';
    }
    state->selected_project_count = write_index;
}

int selected_id_in_visible_items(const MemConsoleState *state) {
    int i;

    if (!state || state->selected_item_id == 0) {
        return 0;
    }

    for (i = 0; i < state->visible_count; ++i) {
        if (state->visible_items[i].id == state->selected_item_id) {
            return 1;
        }
    }

    return 0;
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
