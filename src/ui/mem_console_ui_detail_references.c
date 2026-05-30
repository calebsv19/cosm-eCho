#include "mem_console_ui_detail_section_internal.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *k_mem_console_reference_root = "/Users/calebsv/Desktop/CodeWork";

static int detail_is_md_path_char(char c) {
    return c == '/' || c == '.' || c == '_' || c == '-' || c == '~' || c == ':' || c == '#'
           || c == '?' || c == '=';
}

static int detail_is_token_trim_char(char c) {
    return c == '"' || c == '\'' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{'
           || c == '}' || c == '<' || c == '>' || c == ',' || c == ';' || c == '!' || c == '`';
}

static void detail_trim_token(const char *source, char *out_text, size_t out_cap) {
    size_t start = 0u;
    size_t end;
    size_t len;

    if (!out_text || out_cap == 0u) {
        return;
    }
    out_text[0] = '\0';
    if (!source || !source[0]) {
        return;
    }

    len = strlen(source);
    end = len;
    while (start < len && detail_is_token_trim_char(source[start])) {
        start += 1u;
    }
    while (end > start && detail_is_token_trim_char(source[end - 1u])) {
        end -= 1u;
    }
    if (end <= start) {
        return;
    }

    len = end - start;
    if (len >= out_cap) {
        len = out_cap - 1u;
    }
    memcpy(out_text, source + start, len);
    out_text[len] = '\0';
}

static int detail_extract_md_prefix(const char *token, char *out_text, size_t out_cap) {
    size_t i = 0u;
    size_t j = 0u;

    if (!token || !token[0] || !out_text || out_cap == 0u) {
        return 0;
    }
    out_text[0] = '\0';

    while (token[i] != '\0' && j + 1u < out_cap) {
        char c = token[i];
        out_text[j++] = c;
        if (j >= 3u &&
            out_text[j - 3u] == '.' &&
            (out_text[j - 2u] == 'm' || out_text[j - 2u] == 'M') &&
            (out_text[j - 1u] == 'd' || out_text[j - 1u] == 'D')) {
            char next = token[i + 1u];
            if (next == '\0' || next == '#' || next == '?' || detail_is_token_trim_char(next)) {
                out_text[j] = '\0';
                return strchr(out_text, '/') != 0;
            }
            if (next == '.') {
                out_text[j] = '\0';
                return strchr(out_text, '/') != 0;
            }
        }

        i += 1u;
        if (!detail_is_md_path_char(token[i]) && !isalnum((unsigned char)token[i])) {
            break;
        }
    }

    out_text[0] = '\0';
    return 0;
}

static int detail_path_is_regular_file(const char *path) {
    struct stat st = {0};

    if (!path || !path[0]) {
        return 0;
    }
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode) ? 1 : 0;
}

static int detail_resolve_reference_path(const char *path_text, char *out_path, size_t out_cap) {
    const char *relative = path_text;
    const char *home;

    if (!path_text || !path_text[0] || !out_path || out_cap == 0u) {
        return 0;
    }
    out_path[0] = '\0';

    if (path_text[0] == '/') {
        (void)snprintf(out_path, out_cap, "%s", path_text);
        return 1;
    }
    if (path_text[0] == '~' && path_text[1] == '/') {
        home = getenv("HOME");
        if (!home || !home[0]) {
            return 0;
        }
        (void)snprintf(out_path, out_cap, "%s/%s", home, path_text + 2);
        return 1;
    }

    while (relative[0] == '.' && relative[1] == '/') {
        relative += 2;
    }
    while (relative[0] == '/') {
        relative += 1;
    }
    if (!relative[0]) {
        return 0;
    }

    (void)snprintf(out_path, out_cap, "%s/%s", k_mem_console_reference_root, relative);
    return 1;
}

static int detail_find_markdown_reference_path(const char *text, char *out_path, size_t out_cap) {
    const char *cursor = text;
    char raw_token[320];
    char trimmed_token[320];
    char md_prefix[320];
    char resolved_path[1024];

    if (!text || !text[0] || !out_path || out_cap == 0u) {
        return 0;
    }
    out_path[0] = '\0';

    while (*cursor != '\0') {
        size_t len = 0u;

        while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
            cursor += 1;
        }
        if (*cursor == '\0') {
            break;
        }

        while (*cursor != '\0' && !isspace((unsigned char)*cursor)) {
            if (len + 1u < sizeof(raw_token)) {
                raw_token[len++] = *cursor;
            }
            cursor += 1;
        }
        raw_token[len] = '\0';
        if (len == 0u) {
            continue;
        }

        detail_trim_token(raw_token, trimmed_token, sizeof(trimmed_token));
        if (!detail_extract_md_prefix(trimmed_token, md_prefix, sizeof(md_prefix))) {
            continue;
        }
        if (!detail_resolve_reference_path(md_prefix, resolved_path, sizeof(resolved_path))) {
            continue;
        }
        if (!detail_path_is_regular_file(resolved_path)) {
            continue;
        }

        (void)snprintf(out_path, out_cap, "%s", resolved_path);
        return 1;
    }

    return 0;
}

void mem_console_ui_detail_refresh_reference_path_cache(MemConsoleState *state) {
    if (!state) {
        return;
    }
    if (state->detail_reference_scan_item_id == state->selected_item_id) {
        return;
    }

    state->detail_reference_scan_item_id = state->selected_item_id;
    state->detail_reference_path_available = 0;
    state->detail_reference_path[0] = '\0';

    if (state->selected_item_id == 0) {
        return;
    }
    if (detail_find_markdown_reference_path(state->selected_body,
                                            state->detail_reference_path,
                                            sizeof(state->detail_reference_path)) ||
        detail_find_markdown_reference_path(state->selected_title,
                                            state->detail_reference_path,
                                            sizeof(state->detail_reference_path))) {
        state->detail_reference_path_available = 1;
    }
}
