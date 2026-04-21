#include "mem_console_ui_graph_internal.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const GraphEdgeLegendEntry k_graph_edge_legend_entries[] = {
    { "next_step", "NEXT", { 120, 214, 250, 255 } },
    { "child_of", "CHILD", { 184, 212, 120, 255 } },
    { "supports", "SUPPORTS", { 64, 208, 128, 255 } },
    { "depends_on", "DEPENDS", { 232, 162, 56, 255 } },
    { "references", "REFS", { 74, 184, 255, 255 } },
    { "summarizes", "SUMMARY", { 178, 120, 255, 255 } },
    { "related", "RELATED", { 168, 178, 196, 255 } },
    { "implements", "IMPLEMENTS", { 156, 214, 78, 255 } },
    { "blocks", "BLOCKS", { 230, 92, 92, 255 } },
    { "contradicts", "CONTRADICTS", { 255, 96, 152, 255 } }
};

const GraphEdgeLegendEntry *graph_edge_legend_entry_for_kind(const char *kind) {
    uint32_t i;

    if (!kind || !kind[0]) {
        kind = "related";
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_graph_edge_legend_entries) / sizeof(k_graph_edge_legend_entries[0])); ++i) {
        if (strcmp(k_graph_edge_legend_entries[i].kind, kind) == 0) {
            return &k_graph_edge_legend_entries[i];
        }
    }
    return 0;
}

uint32_t graph_edge_legend_entry_count(void) {
    return (uint32_t)(sizeof(k_graph_edge_legend_entries) / sizeof(k_graph_edge_legend_entries[0]));
}

const GraphEdgeLegendEntry *graph_edge_legend_entry_at(uint32_t index) {
    if (index >= graph_edge_legend_entry_count()) {
        return 0;
    }
    return &k_graph_edge_legend_entries[index];
}

KitRenderColor graph_edge_color_for_kind(const char *kind) {
    const GraphEdgeLegendEntry *entry = graph_edge_legend_entry_for_kind(kind);
    if (entry) {
        return entry->color;
    }
    return (KitRenderColor){ 188, 196, 210, 255 };
}

const char *graph_edge_display_label_for_kind(const char *kind) {
    const GraphEdgeLegendEntry *entry = graph_edge_legend_entry_for_kind(kind);
    if (entry) {
        return entry->label;
    }
    if (!kind || !kind[0]) {
        return "RELATED";
    }
    return kind;
}

static int graph_text_has_prefix(const char *text, const char *prefix) {
    size_t prefix_len;

    if (!text || !prefix) {
        return 0;
    }
    prefix_len = strlen(prefix);
    if (prefix_len == 0u) {
        return 0;
    }
    return strncmp(text, prefix, prefix_len) == 0 ? 1 : 0;
}

static int graph_project_char_is_separator(unsigned char c) {
    return c == '_' || c == '-' || c == '/' || c == '.';
}

void graph_format_project_display_name(const char *project_key,
                                       char *out_text,
                                       size_t out_cap) {
    size_t write_index = 0u;
    int start_of_word = 1;
    size_t i = 0u;

    if (!out_text || out_cap == 0u) {
        return;
    }

    out_text[0] = '\0';
    if (!project_key || project_key[0] == '\0') {
        (void)snprintf(out_text, out_cap, "Misc");
        return;
    }

    while (project_key[i] != '\0' && write_index + 1u < out_cap) {
        unsigned char c = (unsigned char)project_key[i];
        if (graph_project_char_is_separator(c)) {
            if (write_index > 0u && out_text[write_index - 1u] != ' ') {
                out_text[write_index++] = ' ';
            }
            start_of_word = 1;
            i += 1u;
            continue;
        }
        if (start_of_word) {
            out_text[write_index++] = (char)toupper(c);
            start_of_word = 0;
        } else {
            out_text[write_index++] = (char)tolower(c);
        }
        i += 1u;
    }

    while (write_index > 0u && out_text[write_index - 1u] == ' ') {
        write_index -= 1u;
    }
    out_text[write_index] = '\0';

    if (out_text[0] == '\0') {
        (void)snprintf(out_text, out_cap, "Misc");
    }
}

GraphBucketRole graph_bucket_role_for_node(const MemConsoleGraphNode *node) {
    if (!node) {
        return GRAPH_BUCKET_ROLE_NONE;
    }
    if (graph_text_has_prefix(node->stable_id, "scope-")) {
        return GRAPH_BUCKET_ROLE_SCOPE;
    }
    if (graph_text_has_prefix(node->stable_id, "plans-")) {
        return GRAPH_BUCKET_ROLE_PLANS;
    }
    if (graph_text_has_prefix(node->stable_id, "decisions-")) {
        return GRAPH_BUCKET_ROLE_DECISIONS;
    }
    if (graph_text_has_prefix(node->stable_id, "issues-")) {
        return GRAPH_BUCKET_ROLE_ISSUES;
    }
    if (graph_text_has_prefix(node->stable_id, "misc-")) {
        return GRAPH_BUCKET_ROLE_MISC;
    }
    return GRAPH_BUCKET_ROLE_NONE;
}

const char *graph_bucket_role_label(GraphBucketRole role) {
    switch (role) {
        case GRAPH_BUCKET_ROLE_SCOPE: return "SCOPE";
        case GRAPH_BUCKET_ROLE_PLANS: return "PLANS";
        case GRAPH_BUCKET_ROLE_DECISIONS: return "DECISIONS";
        case GRAPH_BUCKET_ROLE_ISSUES: return "ISSUES";
        case GRAPH_BUCKET_ROLE_MISC: return "MISC";
        default: break;
    }
    return "";
}

KitRenderColor graph_bucket_border_color(GraphBucketRole role) {
    switch (role) {
        case GRAPH_BUCKET_ROLE_SCOPE: return (KitRenderColor){ 110, 170, 255, 255 };
        case GRAPH_BUCKET_ROLE_PLANS: return (KitRenderColor){ 128, 224, 152, 255 };
        case GRAPH_BUCKET_ROLE_DECISIONS: return (KitRenderColor){ 244, 190, 92, 255 };
        case GRAPH_BUCKET_ROLE_ISSUES: return (KitRenderColor){ 244, 128, 128, 255 };
        case GRAPH_BUCKET_ROLE_MISC: return (KitRenderColor){ 178, 178, 196, 255 };
        default: break;
    }
    return (KitRenderColor){ 188, 196, 210, 255 };
}

void graph_build_node_label_text(const MemConsoleGraphNode *node,
                                 char *out_text,
                                 size_t out_cap) {
    if (!out_text || out_cap == 0u) {
        return;
    }
    out_text[0] = '\0';
    if (!node) {
        (void)snprintf(out_text, out_cap, "0");
        return;
    }

    (void)snprintf(out_text, out_cap, "%lld", (long long)node->item_id);
}

int graph_edge_is_hierarchy_kind(const char *kind) {
    if (!kind || !kind[0]) {
        return 0;
    }
    if (strcmp(kind, "next_step") == 0 ||
        strcmp(kind, "child_of") == 0 ||
        strcmp(kind, "summarizes") == 0 ||
        strcmp(kind, "implements") == 0 ||
        strcmp(kind, "depends_on") == 0 ||
        strcmp(kind, "supports") == 0) {
        return 1;
    }
    return strstr(kind, "step") != 0 || strstr(kind, "child") != 0;
}
