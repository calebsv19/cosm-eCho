#include "mem_console_state_roles.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

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

static void left_panel_format_updated_ns(int64_t updated_ns, char *out_text, size_t out_cap) {
    time_t seconds;
    struct tm tm_value;

    if (!out_text || out_cap == 0u) {
        return;
    }
    out_text[0] = '\0';
    if (updated_ns <= 0) {
        (void)snprintf(out_text, out_cap, "time ?");
        return;
    }

    seconds = (time_t)(updated_ns / 1000000000LL);
    if (localtime_r(&seconds, &tm_value) == NULL) {
        (void)snprintf(out_text, out_cap, "time ?");
        return;
    }
    (void)snprintf(out_text,
                   out_cap,
                   "%02d/%02d %02d:%02d",
                   tm_value.tm_mon + 1,
                   tm_value.tm_mday,
                   tm_value.tm_hour,
                   tm_value.tm_min);
}

int mem_console_left_panel_render_state_from_state(const MemConsoleState *state,
                                                   MemConsoleLeftPanelRenderState *out_view) {
    if (!state || !out_view) {
        return 0;
    }

    memset(out_view, 0, sizeof(*out_view));
    out_view->db_path = state->db_path ? state->db_path : "";
    out_view->input_root = state->input_root;
    out_view->schema_version = state->schema_version;
    out_view->theme_name = state->theme_name;
    out_view->status_line = state->status_line;
    out_view->runtime_summary_line = state->runtime_summary_line;
    out_view->active_count = state->active_count;
    out_view->matching_count = state->matching_count;
    out_view->visible_count = state->visible_count;
    if (out_view->visible_count < 0) {
        out_view->visible_count = 0;
    } else if (out_view->visible_count > MEM_CONSOLE_LIST_FETCH_LIMIT) {
        out_view->visible_count = MEM_CONSOLE_LIST_FETCH_LIMIT;
    }
    out_view->visible_items = state->visible_items;
    return 1;
}

int mem_console_left_panel_render_storage_from_state(MemConsoleState *state,
                                                     MemConsoleLeftPanelRenderStorage *out_storage) {
    if (!state || !out_storage) {
        return 0;
    }

    memset(out_storage, 0, sizeof(*out_storage));
    out_storage->db_summary_line = state->db_summary_line;
    out_storage->db_summary_line_cap = sizeof(state->db_summary_line);
    out_storage->db_summary_draw_line = state->db_summary_draw_line;
    out_storage->db_summary_draw_line_cap = sizeof(state->db_summary_draw_line);
    out_storage->input_root_summary_line = state->input_root_summary_line;
    out_storage->input_root_summary_line_cap = sizeof(state->input_root_summary_line);
    out_storage->input_root_summary_draw_line = state->input_root_summary_draw_line;
    out_storage->input_root_summary_draw_line_cap = sizeof(state->input_root_summary_draw_line);
    out_storage->schema_summary_line = state->schema_summary_line;
    out_storage->schema_summary_line_cap = sizeof(state->schema_summary_line);
    out_storage->visible_summary_line = state->visible_summary_line;
    out_storage->visible_summary_line_cap = sizeof(state->visible_summary_line);
    out_storage->status_draw_line = state->status_draw_line;
    out_storage->status_draw_line_cap = sizeof(state->status_draw_line);
    out_storage->runtime_summary_draw_line = state->runtime_summary_draw_line;
    out_storage->runtime_summary_draw_line_cap = sizeof(state->runtime_summary_draw_line);
    out_storage->list_item_labels = state->list_item_labels;
    out_storage->list_item_label_count = MEM_CONSOLE_LIST_FETCH_LIMIT;
    return 1;
}

void mem_console_left_panel_derive_db_summary(const MemConsoleLeftPanelRenderState *view,
                                              MemConsoleLeftPanelRenderStorage *storage,
                                              float width_px) {
    if (!view || !storage || !storage->db_summary_line || !storage->db_summary_draw_line) {
        return;
    }

    (void)snprintf(storage->db_summary_line,
                   storage->db_summary_line_cap,
                   "DB: %s",
                   view->db_path ? view->db_path : "");
    format_text_for_width(storage->db_summary_draw_line,
                          storage->db_summary_draw_line_cap,
                          storage->db_summary_line,
                          width_px,
                          CORE_FONT_TEXT_SIZE_CAPTION);
}

void mem_console_left_panel_derive_input_root_summary(const MemConsoleLeftPanelRenderState *view,
                                                      MemConsoleLeftPanelRenderStorage *storage,
                                                      float width_px) {
    if (!view || !storage || !storage->input_root_summary_line || !storage->input_root_summary_draw_line) {
        return;
    }

    (void)snprintf(storage->input_root_summary_line,
                   storage->input_root_summary_line_cap,
                   "Input Root: %s",
                   view->input_root && view->input_root[0] ? view->input_root : "(unset)");
    format_text_for_width(storage->input_root_summary_draw_line,
                          storage->input_root_summary_draw_line_cap,
                          storage->input_root_summary_line,
                          width_px,
                          CORE_FONT_TEXT_SIZE_CAPTION);
}

void mem_console_left_panel_derive_schema_summary(const MemConsoleLeftPanelRenderState *view,
                                                  MemConsoleLeftPanelRenderStorage *storage) {
    if (!view || !storage || !storage->schema_summary_line) {
        return;
    }

    (void)snprintf(storage->schema_summary_line,
                   storage->schema_summary_line_cap,
                   "v%s | Active %lld | %s",
                   view->schema_version && view->schema_version[0] ? view->schema_version : "?",
                   (long long)view->active_count,
                   view->theme_name && view->theme_name[0] ? view->theme_name : "theme");
}

void mem_console_left_panel_derive_visible_summary(const MemConsoleLeftPanelRenderState *view,
                                                   MemConsoleLeftPanelRenderStorage *storage) {
    if (!view || !storage || !storage->visible_summary_line) {
        return;
    }

    (void)snprintf(storage->visible_summary_line,
                   storage->visible_summary_line_cap,
                   "%d loaded | %lld matching",
                   view->visible_count,
                   (long long)view->matching_count);
}

void mem_console_left_panel_derive_status_summary(const MemConsoleLeftPanelRenderState *view,
                                                  MemConsoleLeftPanelRenderStorage *storage,
                                                  float width_px) {
    if (!view || !storage || !storage->status_draw_line) {
        return;
    }

    format_text_for_width(storage->status_draw_line,
                          storage->status_draw_line_cap,
                          view->status_line,
                          width_px,
                          CORE_FONT_TEXT_SIZE_CAPTION);
}

void mem_console_left_panel_derive_runtime_summary(const MemConsoleLeftPanelRenderState *view,
                                                   MemConsoleLeftPanelRenderStorage *storage,
                                                   float width_px) {
    if (!view || !storage || !storage->runtime_summary_draw_line) {
        return;
    }

    format_text_for_width(storage->runtime_summary_draw_line,
                          storage->runtime_summary_draw_line_cap,
                          view->runtime_summary_line,
                          width_px,
                          CORE_FONT_TEXT_SIZE_CAPTION);
}

int mem_console_left_panel_derive_item_label(const MemConsoleLeftPanelRenderState *view,
                                             MemConsoleLeftPanelRenderStorage *storage,
                                             int visible_index) {
    const MemConsoleListItem *item;
    char updated_text[32];
    const char *project_text;
    const char *kind_text;
    const char *title;

    if (!view || !storage || !view->visible_items || !storage->list_item_labels) {
        return 0;
    }
    if (visible_index < 0 ||
        visible_index >= view->visible_count ||
        visible_index >= storage->list_item_label_count) {
        return 0;
    }

    item = &view->visible_items[visible_index];
    project_text = item->project_key[0] ? item->project_key : "no_project";
    kind_text = item->kind[0] ? item->kind : "note";
    title = item->title[0] ? item->title : "UNTITLED";
    left_panel_format_updated_ns(item->updated_ns, updated_text, sizeof(updated_text));
    (void)snprintf(storage->list_item_labels[visible_index],
                   MEM_CONSOLE_LIST_ITEM_LABEL_CAP,
                   "%lld %s%s[%s/%s] %s | %s",
                   (long long)item->id,
                   item->pinned ? "P " : "",
                   item->canonical ? "C " : "",
                   project_text,
                   kind_text,
                   updated_text,
                   title);
    return 1;
}

static int detail_format_created_timestamp_compact(int64_t created_ns, char *out_text, size_t out_cap) {
    time_t seconds;
    struct tm local_tm;
    size_t written = 0u;

    if (!out_text || out_cap == 0u || created_ns <= 0) {
        return 0;
    }
    out_text[0] = '\0';

    seconds = (time_t)(created_ns / 1000000000LL);
    if (seconds <= 0) {
        return 0;
    }

#if defined(_POSIX_VERSION) || defined(__APPLE__) || defined(__linux__)
    if (!localtime_r(&seconds, &local_tm)) {
        return 0;
    }
#else
    {
        struct tm *tmp = localtime(&seconds);
        if (!tmp) {
            return 0;
        }
        local_tm = *tmp;
    }
#endif
    written = strftime(out_text, out_cap, "%b %d %H:%M", &local_tm);
    return written > 0u ? 1 : 0;
}

int mem_console_detail_render_state_from_state(const MemConsoleState *state,
                                               MemConsoleDetailRenderState *out_view) {
    if (!state || !out_view) {
        return 0;
    }

    memset(out_view, 0, sizeof(*out_view));
    out_view->selected_item_id = state->selected_item_id;
    out_view->selected_created_ns = state->selected_created_ns;
    out_view->selected_title = state->selected_title;
    out_view->relationships = state->detail_relationships;
    out_view->relationship_count = state->detail_relationship_count;
    if (out_view->relationship_count < 0) {
        out_view->relationship_count = 0;
    } else if (out_view->relationship_count > MEM_CONSOLE_DETAIL_RELATIONSHIP_LIMIT) {
        out_view->relationship_count = MEM_CONSOLE_DETAIL_RELATIONSHIP_LIMIT;
    }
    out_view->relationship_out_count = state->detail_relationship_out_count;
    out_view->relationship_in_count = state->detail_relationship_in_count;
    out_view->relationship_summary_line = state->detail_relationship_summary_line;
    return 1;
}

int mem_console_detail_render_storage_from_state(MemConsoleState *state,
                                                 MemConsoleDetailRenderStorage *out_storage) {
    if (!state || !out_storage) {
        return 0;
    }

    memset(out_storage, 0, sizeof(*out_storage));
    out_storage->title_lines = state->detail_title_lines;
    out_storage->title_line_capacity = MEM_CONSOLE_DETAIL_TITLE_LINE_LIMIT;
    out_storage->title_line_count = &state->detail_title_line_count;
    out_storage->meta_line = state->detail_meta_line;
    out_storage->meta_line_cap = sizeof(state->detail_meta_line);
    out_storage->connection_summary_lines = state->detail_connection_summary_lines;
    out_storage->connection_summary_line_capacity = MEM_CONSOLE_DETAIL_CONNECTION_WRAP_LINE_LIMIT;
    out_storage->relationship_group_labels = state->detail_relationship_group_labels;
    out_storage->relationship_row_labels = state->detail_relationship_row_labels;
    out_storage->relationship_label_capacity = MEM_CONSOLE_DETAIL_RELATIONSHIP_LIMIT;
    return 1;
}

float mem_console_detail_wrapped_text_line_step(CoreFontTextSizeTier text_tier) {
    switch (text_tier) {
        case CORE_FONT_TEXT_SIZE_CAPTION:
            return 16.0f;
        case CORE_FONT_TEXT_SIZE_BASIC:
            return 20.0f;
        case CORE_FONT_TEXT_SIZE_PARAGRAPH:
        case CORE_FONT_TEXT_SIZE_TITLE:
        case CORE_FONT_TEXT_SIZE_HEADER:
        default:
            return 24.0f;
    }
}

int mem_console_detail_wrap_text_lines(const char *text,
                                       char line_storage[][MEM_CONSOLE_DETAIL_TEXT_LINE_CAP],
                                       int line_storage_count,
                                       int max_chars) {
    const char *cursor;
    int line_count = 0;

    if (!text || !line_storage || line_storage_count <= 0) {
        return 0;
    }

    if (max_chars < 18) {
        max_chars = 18;
    }
    if (max_chars > 120) {
        max_chars = 120;
    }

    cursor = text;
    while (*cursor != '\0' && line_count < line_storage_count) {
        const char *line_start;
        const char *break_at = 0;
        int len = 0;
        char *line = line_storage[line_count];

        while (*cursor == ' ') {
            cursor += 1;
        }
        if (*cursor == '\n') {
            line[0] = '\0';
            cursor += 1;
            line_count += 1;
            continue;
        }

        line_start = cursor;
        while (*cursor != '\0' && *cursor != '\n' && len < max_chars) {
            if (*cursor == ' ') {
                break_at = cursor;
            }
            cursor += 1;
            len += 1;
        }

        if (*cursor != '\0' && *cursor != '\n' && len >= max_chars && break_at && break_at > line_start) {
            cursor = break_at;
            len = (int)(break_at - line_start);
        }

        if (len <= 0) {
            if (*cursor == '\n') {
                cursor += 1;
            } else if (*cursor != '\0') {
                cursor += 1;
            }
            continue;
        }

        if ((size_t)len >= MEM_CONSOLE_DETAIL_TEXT_LINE_CAP) {
            len = MEM_CONSOLE_DETAIL_TEXT_LINE_CAP - 1;
        }
        memcpy(line, line_start, (size_t)len);
        line[len] = '\0';

        while (*cursor == ' ') {
            cursor += 1;
        }
        if (*cursor == '\n') {
            cursor += 1;
        }

        line_count += 1;
    }

    return line_count;
}

void mem_console_detail_clear_title_lines(MemConsoleDetailRenderStorage *storage) {
    if (!storage || !storage->title_line_count) {
        return;
    }
    *storage->title_line_count = 0;
}

int mem_console_detail_derive_title_lines(const MemConsoleDetailRenderState *view,
                                          MemConsoleDetailRenderStorage *storage,
                                          int line_limit,
                                          int max_chars) {
    int line_count;

    if (!view || !storage || !storage->title_lines || !storage->title_line_count) {
        return 0;
    }

    if (line_limit < 1) {
        line_limit = 1;
    }
    if (line_limit > storage->title_line_capacity) {
        line_limit = storage->title_line_capacity;
    }

    line_count = mem_console_detail_wrap_text_lines(view->selected_title,
                                                    storage->title_lines,
                                                    line_limit,
                                                    max_chars);
    if (line_count <= 0) {
        line_count = 1;
        (void)snprintf(storage->title_lines[0],
                       MEM_CONSOLE_DETAIL_TEXT_LINE_CAP,
                       "%s",
                       view->selected_title ? view->selected_title : "");
    }

    *storage->title_line_count = line_count;
    return line_count;
}

void mem_console_detail_derive_meta_line(const MemConsoleDetailRenderState *view,
                                         MemConsoleDetailRenderStorage *storage) {
    char created_label[32];

    if (!view || !storage || !storage->meta_line || storage->meta_line_cap == 0u) {
        return;
    }

    if (view->selected_item_id != 0) {
        if (detail_format_created_timestamp_compact(view->selected_created_ns,
                                                    created_label,
                                                    sizeof(created_label))) {
            (void)snprintf(storage->meta_line,
                           storage->meta_line_cap,
                           "MEMORY ID %lld | %s",
                           (long long)view->selected_item_id,
                           created_label);
        } else {
            (void)snprintf(storage->meta_line,
                           storage->meta_line_cap,
                           "MEMORY ID %lld",
                           (long long)view->selected_item_id);
        }
    } else {
        (void)snprintf(storage->meta_line,
                       storage->meta_line_cap,
                       "SELECT A MEMORY TO EDIT");
    }
}

const char *mem_console_detail_relationship_header_label(const MemConsoleDetailRenderState *view) {
    if (!view || !view->relationship_summary_line || !view->relationship_summary_line[0]) {
        return "RELATIONSHIPS";
    }
    return view->relationship_summary_line;
}

int mem_console_detail_relationship_group_changed(const MemConsoleRelationshipItem *prev,
                                                  const MemConsoleRelationshipItem *item) {
    if (!item) {
        return 0;
    }
    if (!prev) {
        return 1;
    }
    if (prev->outgoing != item->outgoing) {
        return 1;
    }
    if (strncmp(prev->kind, item->kind, sizeof(item->kind)) != 0) {
        return 1;
    }
    return 0;
}

int mem_console_detail_relationship_group_count(const MemConsoleDetailRenderState *view) {
    const MemConsoleRelationshipItem *prev = 0;
    int count = 0;
    int i;

    if (!view || !view->relationships) {
        return 0;
    }

    for (i = 0; i < view->relationship_count; ++i) {
        const MemConsoleRelationshipItem *item = &view->relationships[i];
        if (mem_console_detail_relationship_group_changed(prev, item)) {
            count += 1;
        }
        prev = item;
    }
    return count;
}

void mem_console_detail_derive_empty_relationship_line(const MemConsoleDetailRenderState *view,
                                                       MemConsoleDetailRenderStorage *storage) {
    if (!storage ||
        !storage->connection_summary_lines ||
        storage->connection_summary_line_capacity <= 0) {
        return;
    }

    (void)snprintf(storage->connection_summary_lines[0],
                   MEM_CONSOLE_DETAIL_CONNECTION_LINE_CAP,
                   "%s",
                   (!view || view->selected_item_id == 0)
                       ? "Select a memory to inspect relationships."
                       : "No relationships for selected memory.");
}

int mem_console_detail_derive_relationship_group_label(const MemConsoleDetailRenderState *view,
                                                       MemConsoleDetailRenderStorage *storage,
                                                       int group_index,
                                                       const MemConsoleRelationshipItem *item) {
    const char *direction;
    const char *kind;

    (void)view;
    if (!storage || !storage->relationship_group_labels || !item) {
        return 0;
    }
    if (group_index < 0 || group_index >= storage->relationship_label_capacity) {
        return 0;
    }

    direction = item->outgoing ? "OUT" : "IN";
    kind = item->kind[0] ? item->kind : "RELATED";
    (void)snprintf(storage->relationship_group_labels[group_index],
                   MEM_CONSOLE_DETAIL_RELATIONSHIP_GROUP_LABEL_CAP,
                   "%s %s",
                   direction,
                   kind);
    return 1;
}

int mem_console_detail_derive_relationship_row_label(const MemConsoleDetailRenderState *view,
                                                     MemConsoleDetailRenderStorage *storage,
                                                     int row_index,
                                                     const MemConsoleRelationshipItem *item) {
    const char *arrow;
    const char *project_key;
    const char *neighbor_kind;
    const char *title;

    (void)view;
    if (!storage || !storage->relationship_row_labels || !item) {
        return 0;
    }
    if (row_index < 0 || row_index >= storage->relationship_label_capacity) {
        return 0;
    }

    arrow = item->outgoing ? "->" : "<-";
    project_key = item->neighbor_project_key[0] ? item->neighbor_project_key : "misc";
    neighbor_kind = item->neighbor_kind[0] ? item->neighbor_kind : "memory";
    title = item->neighbor_title[0] ? item->neighbor_title : "UNTITLED";

    (void)snprintf(storage->relationship_row_labels[row_index],
                   MEM_CONSOLE_DETAIL_RELATIONSHIP_ROW_LABEL_CAP,
                   "%s %lld [%s] %s | %s",
                   arrow,
                   (long long)item->neighbor_item_id,
                   project_key,
                   neighbor_kind,
                   title);
    return 1;
}

int mem_console_graph_render_state_from_state(const MemConsoleState *state,
                                              MemConsoleGraphRenderState *out_view) {
    if (!state || !out_view) {
        return 0;
    }

    memset(out_view, 0, sizeof(*out_view));
    out_view->nodes = state->graph_nodes;
    out_view->node_count = state->graph_node_count;
    if (out_view->node_count < 0) {
        out_view->node_count = 0;
    } else if (out_view->node_count > MEM_CONSOLE_GRAPH_NODE_LIMIT) {
        out_view->node_count = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    }
    out_view->edges = state->graph_edges;
    out_view->edge_count = state->graph_edge_count;
    if (out_view->edge_count < 0) {
        out_view->edge_count = 0;
    } else if (out_view->edge_count > MEM_CONSOLE_GRAPH_EDGE_LIMIT) {
        out_view->edge_count = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    }
    return 1;
}

int mem_console_graph_render_storage_from_state(MemConsoleState *state,
                                                MemConsoleGraphRenderStorage *out_storage) {
    if (!state || !out_storage) {
        return 0;
    }

    memset(out_storage, 0, sizeof(*out_storage));
    out_storage->edge_labels = state->graph_draw_edge_labels;
    out_storage->edge_label_capacity = MEM_CONSOLE_GRAPH_EDGE_LIMIT;
    out_storage->node_labels = state->graph_draw_node_labels;
    out_storage->node_label_capacity = MEM_CONSOLE_GRAPH_NODE_LIMIT;
    return 1;
}

int mem_console_graph_derive_edge_label(const MemConsoleGraphRenderState *view,
                                        MemConsoleGraphRenderStorage *storage,
                                        int edge_index,
                                        const char *edge_kind_label) {
    (void)view;
    if (!storage || !storage->edge_labels) {
        return 0;
    }
    if (edge_index < 0 || edge_index >= storage->edge_label_capacity) {
        return 0;
    }

    (void)snprintf(storage->edge_labels[edge_index],
                   MEM_CONSOLE_GRAPH_EDGE_LABEL_CAP,
                   "%s",
                   edge_kind_label ? edge_kind_label : "");
    return 1;
}

int mem_console_graph_derive_node_label(const MemConsoleGraphRenderState *view,
                                        MemConsoleGraphRenderStorage *storage,
                                        int node_index) {
    const MemConsoleGraphNode *node = 0;

    if (!storage || !storage->node_labels) {
        return 0;
    }
    if (node_index < 0 || node_index >= storage->node_label_capacity) {
        return 0;
    }
    if (view && view->nodes && node_index < view->node_count) {
        node = &view->nodes[node_index];
    }

    if (node) {
        (void)snprintf(storage->node_labels[node_index],
                       MEM_CONSOLE_GRAPH_NODE_LABEL_CAP,
                       "%lld",
                       (long long)node->item_id);
    } else {
        (void)snprintf(storage->node_labels[node_index],
                       MEM_CONSOLE_GRAPH_NODE_LABEL_CAP,
                       "0");
    }
    return 1;
}
