#include "mem_console_ui_hud.h"
#include "mem_console_ui_common.h"

#include <stdio.h>
#include <string.h>

static uint64_t hud_hash_bytes(uint64_t seed, const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint64_t hash = seed;
    size_t i;

    for (i = 0u; i < len; ++i) {
        hash ^= (uint64_t)bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint32_t hud_float_bits(float value) {
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float hud_line_step(CoreFontTextSizeTier text_tier) {
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

static int hud_max_chars_for_outer_width(float outer_width) {
    float content_px = outer_width - 28.0f;
    int max_chars = (int)((content_px > 0.0f ? content_px : 120.0f) / 8.0f);

    if (max_chars < 12) {
        max_chars = 12;
    }
    if (max_chars > 120) {
        max_chars = 120;
    }
    return max_chars;
}

static void hud_sanitize_text(const char *input, char *output, size_t output_cap) {
    size_t w = 0u;
    size_t i = 0u;
    int last_was_space = 0;
    int in_ansi_escape = 0;

    if (!output || output_cap == 0u) {
        return;
    }
    output[0] = '\0';
    if (!input) {
        return;
    }

    while (input[i] != '\0' && w + 1u < output_cap) {
        unsigned char c = (unsigned char)input[i];
        char out_ch = '?';

        if (in_ansi_escape) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                in_ansi_escape = 0;
            }
            i += 1u;
            continue;
        }
        if (c == 27u) {
            in_ansi_escape = 1;
            i += 1u;
            continue;
        }
        if (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
            out_ch = ' ';
        } else if (c >= 32u && c <= 126u) {
            out_ch = (char)c;
        } else {
            i += 1u;
            continue;
        }

        if (out_ch == ' ') {
            if (!last_was_space) {
                output[w++] = out_ch;
                last_was_space = 1;
            }
        } else {
            output[w++] = out_ch;
            last_was_space = 0;
        }
        i += 1u;
    }

    while (w > 0u && output[w - 1u] == ' ') {
        w -= 1u;
    }
    output[w] = '\0';
    if (output[0] == '\0' && output_cap >= 2u) {
        output[0] = '-';
        output[1] = '\0';
    }
}

static int hud_wrap_text_lines(const char *text,
                               char lines[][256],
                               int max_lines,
                               int max_chars_per_line) {
    const char *cursor;
    int line_count = 0;

    if (!text || !lines || max_lines <= 0) {
        return 0;
    }
    if (max_chars_per_line < 12) {
        max_chars_per_line = 12;
    }
    if (max_chars_per_line > 120) {
        max_chars_per_line = 120;
    }

    cursor = text;
    while (*cursor != '\0' && line_count < max_lines) {
        const char *line_start;
        const char *break_at = 0;
        int len = 0;

        while (*cursor == ' ') {
            cursor += 1;
        }
        line_start = cursor;

        while (*cursor != '\0' && *cursor != '\n' && len < max_chars_per_line) {
            if (*cursor == ' ') {
                break_at = cursor;
            }
            cursor += 1;
            len += 1;
        }

        if (*cursor != '\0' &&
            *cursor != '\n' &&
            len >= max_chars_per_line &&
            break_at &&
            break_at > line_start) {
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

        if ((size_t)len >= 256u) {
            len = 255;
        }
        memcpy(lines[line_count], line_start, (size_t)len);
        lines[line_count][len] = '\0';
        line_count += 1;

        while (*cursor == ' ') {
            cursor += 1;
        }
        if (*cursor == '\n') {
            cursor += 1;
        }
    }

    return line_count;
}

static uint64_t hud_signature_for_spec(const MemConsoleUiHudCardSpec *spec,
                                       float width,
                                       float height,
                                       float wrap_outer_width) {
    uint64_t hash = 1469598103934665603ull;
    uint32_t width_bits = hud_float_bits(width);
    uint32_t height_bits = hud_float_bits(height);
    uint32_t wrap_bits = hud_float_bits(wrap_outer_width);
    int i;

    if (!spec) {
        return hash;
    }

    hash = hud_hash_bytes(hash, &spec->cache_key, sizeof(spec->cache_key));
    hash = hud_hash_bytes(hash, &spec->min_width, sizeof(spec->min_width));
    hash = hud_hash_bytes(hash, &spec->max_width, sizeof(spec->max_width));
    hash = hud_hash_bytes(hash, &spec->edge_margin, sizeof(spec->edge_margin));
    hash = hud_hash_bytes(hash, &width_bits, sizeof(width_bits));
    hash = hud_hash_bytes(hash, &height_bits, sizeof(height_bits));
    hash = hud_hash_bytes(hash, &wrap_bits, sizeof(wrap_bits));
    hash = hud_hash_bytes(hash, &spec->row_count, sizeof(spec->row_count));
    for (i = 0; i < spec->row_count && i < MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT; ++i) {
        const MemConsoleUiHudRowSpec *row = &spec->rows[i];
        hash = hud_hash_bytes(hash, &row->token, sizeof(row->token));
        hash = hud_hash_bytes(hash, &row->font_role, sizeof(row->font_role));
        hash = hud_hash_bytes(hash, &row->text_tier, sizeof(row->text_tier));
        hash = hud_hash_bytes(hash, &row->max_lines, sizeof(row->max_lines));
        if (row->text) {
            hash = hud_hash_bytes(hash, row->text, strlen(row->text));
        }
    }
    return hash;
}

static void hud_layout_compute(MemConsoleState *state,
                               const MemConsoleUiHudCardSpec *spec,
                               KitRenderRect bounds,
                               float width,
                               float wrap_outer_width) {
    float edge_margin = spec->edge_margin > 0.0f ? spec->edge_margin : 10.0f;
    float inner_width = width - 12.0f;
    float line_rect_width = inner_width - 16.0f;
    float y = 6.0f;
    float total_inner_h;
    int i;
    int total_lines = 0;

    int max_chars_per_line = hud_max_chars_for_outer_width(wrap_outer_width);
    if (line_rect_width <= 0.0f) {
        max_chars_per_line = 12;
    }

    state->graph_hud_cache_row_count = 0;
    state->graph_hud_cache_line_count = 0;

    for (i = 0; i < spec->row_count && i < MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT; ++i) {
        MemConsoleUiHudRowSpec row = spec->rows[i];
        char clean[256];
        int line_cap;
        int line_count;
        float step = hud_line_step(row.text_tier);

        if (row.max_lines <= 0) {
            row.max_lines = 1;
        }
        line_cap = row.max_lines;
        if (line_cap > (MEM_CONSOLE_GRAPH_HUD_LINE_LIMIT - state->graph_hud_cache_line_count)) {
            line_cap = MEM_CONSOLE_GRAPH_HUD_LINE_LIMIT - state->graph_hud_cache_line_count;
        }
        if (line_cap <= 0) {
            break;
        }

        hud_sanitize_text(row.text, clean, sizeof(clean));
        line_count = hud_wrap_text_lines(clean,
                                         &state->graph_hud_cache_lines[state->graph_hud_cache_line_count],
                                         line_cap,
                                         max_chars_per_line);
        if (line_count <= 0) {
            (void)snprintf(state->graph_hud_cache_lines[state->graph_hud_cache_line_count],
                           sizeof(state->graph_hud_cache_lines[state->graph_hud_cache_line_count]),
                           "%s",
                           "-");
            line_count = 1;
        }

        state->graph_hud_cache_row_first_line[state->graph_hud_cache_row_count] = state->graph_hud_cache_line_count;
        state->graph_hud_cache_row_line_count[state->graph_hud_cache_row_count] = line_count;
        state->graph_hud_cache_row_tokens[state->graph_hud_cache_row_count] = row.token;
        state->graph_hud_cache_row_roles[state->graph_hud_cache_row_count] = row.font_role;
        state->graph_hud_cache_row_tiers[state->graph_hud_cache_row_count] = row.text_tier;
        state->graph_hud_cache_row_line_steps[state->graph_hud_cache_row_count] = step;

        state->graph_hud_cache_row_count += 1;
        state->graph_hud_cache_line_count += line_count;
        total_lines += line_count;
    }

    (void)total_lines;
    for (i = 0; i < state->graph_hud_cache_row_count; ++i) {
        y += ((float)state->graph_hud_cache_row_line_count[i]) *
             state->graph_hud_cache_row_line_steps[i];
        y += 2.0f;
    }
    y += 4.0f;
    total_inner_h = y;
    if (total_inner_h < 108.0f) {
        total_inner_h = 108.0f;
    }
    if (total_inner_h > (bounds.height - (edge_margin * 2.0f) - 12.0f)) {
        total_inner_h = bounds.height - (edge_margin * 2.0f) - 12.0f;
    }

    state->graph_hud_cache_outer = (KitRenderRect){
        bounds.x + edge_margin,
        bounds.y + edge_margin,
        width,
        total_inner_h + 12.0f
    };
    state->graph_hud_cache_inner = (KitRenderRect){
        state->graph_hud_cache_outer.x + 6.0f,
        state->graph_hud_cache_outer.y + 6.0f,
        state->graph_hud_cache_outer.width - 12.0f,
        state->graph_hud_cache_outer.height - 12.0f
    };
}

static float hud_compute_snug_width(const KitRenderContext *render_ctx,
                                    const MemConsoleUiHudCardSpec *spec,
                                    float min_width,
                                    float max_width,
                                    float fallback_width) {
    float width = fallback_width;
    float measured_width = 0.0f;
    int max_chars_per_line;
    int i;

    if (!spec) {
        return fallback_width;
    }
    if (width < min_width) width = min_width;
    if (width > max_width) width = max_width;

    /* Wrap once at max content width; size panel from the longest wrapped line. */
    max_chars_per_line = hud_max_chars_for_outer_width(max_width);
    for (i = 0; i < spec->row_count && i < MEM_CONSOLE_GRAPH_HUD_ROW_LIMIT; ++i) {
        const MemConsoleUiHudRowSpec *row = &spec->rows[i];
        char clean[256];
        int line_cap;
        int line_count;
        char wrapped[16][256];
        int l;

        hud_sanitize_text(row->text, clean, sizeof(clean));
        line_cap = row->max_lines > 0 ? row->max_lines : 1;
        if (line_cap > (int)(sizeof(wrapped) / sizeof(wrapped[0]))) {
            line_cap = (int)(sizeof(wrapped) / sizeof(wrapped[0]));
        }

        line_count = hud_wrap_text_lines(clean,
                                         wrapped,
                                         line_cap,
                                         max_chars_per_line);
        if (line_count <= 0) {
            line_count = 1;
            (void)snprintf(wrapped[0], sizeof(wrapped[0]), "%s", "-");
        }

        for (l = 0; l < line_count; ++l) {
            float line_width = mem_console_ui_measure_text_width_px(render_ctx,
                                                                    row->font_role,
                                                                    row->text_tier,
                                                                    wrapped[l]);
            if (line_width > measured_width) {
                measured_width = line_width;
            }
        }
    }

    width = measured_width > 0.0f ? (measured_width + 34.0f) : width;
    if (width < min_width) width = min_width;
    if (width > max_width) width = max_width;
    return width;
}

CoreResult mem_console_ui_hud_draw_cached(const KitRenderContext *render_ctx,
                                          KitUiContext *ui_ctx,
                                          KitRenderFrame *frame,
                                          KitRenderRect bounds,
                                          MemConsoleState *state,
                                          const MemConsoleUiHudCardSpec *spec) {
    float width;
    float min_width;
    float max_width;
    float wrap_outer_width;
    float edge_margin;
    float max_available_width;
    uint64_t signature;
    CoreResult result;
    KitRenderColor hud_outer_color;
    KitRenderColor hud_inner_color;
    float y;
    int i;

    if (!render_ctx || !ui_ctx || !frame || !state || !spec) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid hud draw request" };
    }

    if (spec->row_count <= 0 || bounds.width <= 20.0f || bounds.height <= 20.0f) {
        return core_result_ok();
    }

    width = bounds.width * (spec->width_ratio > 0.0f ? spec->width_ratio : 0.5f);
    min_width = spec->min_width > 0.0f ? spec->min_width : 260.0f;
    edge_margin = spec->edge_margin > 0.0f ? spec->edge_margin : 10.0f;
    max_available_width = bounds.width - (edge_margin * 2.0f);
    max_width = spec->max_width > 0.0f ? spec->max_width : max_available_width;
    if (max_width > max_available_width) {
        max_width = max_available_width;
    }
    if (max_width < 120.0f) {
        max_width = 120.0f;
    }
    if (min_width > max_width) {
        min_width = max_width;
    }
    wrap_outer_width = max_width;
    width = hud_compute_snug_width(render_ctx, spec, min_width, max_width, width);

    signature = hud_signature_for_spec(spec, width, bounds.height, wrap_outer_width);
    if (!state->graph_hud_cache_valid || state->graph_hud_cache_signature != signature) {
        hud_layout_compute(state, spec, bounds, width, wrap_outer_width);
        state->graph_hud_cache_signature = signature;
        state->graph_hud_cache_valid = 1;
    }

    result = mem_console_ui_resolve_theme_color(render_ctx,
                                                CORE_THEME_COLOR_SURFACE_1,
                                                &hud_outer_color);
    if (result.code != CORE_OK) {
        return result;
    }
    result = mem_console_ui_resolve_theme_color(render_ctx,
                                                CORE_THEME_COLOR_SURFACE_0,
                                                &hud_inner_color);
    if (result.code != CORE_OK) {
        return result;
    }
    hud_outer_color.a = 58u;
    hud_inner_color.a = 34u;

    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      state->graph_hud_cache_outer,
                                      8.0f,
                                      hud_outer_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }
    result = kit_render_push_rect(frame,
                                  &(KitRenderRectCommand){
                                      state->graph_hud_cache_inner,
                                      6.0f,
                                      hud_inner_color,
                                      kit_render_identity_transform()
                                  });
    if (result.code != CORE_OK) {
        return result;
    }

    y = state->graph_hud_cache_inner.y + 6.0f;
    for (i = 0; i < state->graph_hud_cache_row_count; ++i) {
        int first = state->graph_hud_cache_row_first_line[i];
        int count = state->graph_hud_cache_row_line_count[i];
        float step = state->graph_hud_cache_row_line_steps[i];
        int l;

        for (l = 0; l < count; ++l) {
            int line_index = first + l;
            if (line_index < 0 || line_index >= state->graph_hud_cache_line_count) {
                continue;
            }
            result = mem_console_ui_draw_info_line_custom(ui_ctx,
                                                          frame,
                                                          (KitRenderRect){
                                                              state->graph_hud_cache_inner.x + 8.0f,
                                                              y,
                                                              state->graph_hud_cache_inner.width - 16.0f,
                                                              step
                                                          },
                                                          state->graph_hud_cache_lines[line_index],
                                                          state->graph_hud_cache_row_tokens[i],
                                                          state->graph_hud_cache_row_roles[i],
                                                          state->graph_hud_cache_row_tiers[i]);
            if (result.code != CORE_OK) {
                return result;
            }
            y += step;
        }
        y += 2.0f;
    }

    return core_result_ok();
}
