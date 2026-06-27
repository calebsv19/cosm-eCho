#include "mem_console_visual_artifact.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/mem_console_app_internal.h"
#include "mem_console_db.h"
#include "mem_console_state.h"
#include "mem_console_ui.h"

static int s_mem_console_visual_artifact_captured = 0;

static void write_xml_text(FILE *out, const char *text) {
    const unsigned char *p = (const unsigned char *)text;

    if (!out || !text) {
        return;
    }

    while (*p) {
        switch (*p) {
            case '&':
                fputs("&amp;", out);
                break;
            case '<':
                fputs("&lt;", out);
                break;
            case '>':
                fputs("&gt;", out);
                break;
            case '"':
                fputs("&quot;", out);
                break;
            default:
                if (*p >= 32u || *p == '\n' || *p == '\t') {
                    fputc((int)*p, out);
                }
                break;
        }
        ++p;
    }
}

static void write_color(FILE *out, KitRenderColor color) {
    fprintf(out, "rgba(%u,%u,%u,%.3f)",
            (unsigned int)color.r,
            (unsigned int)color.g,
            (unsigned int)color.b,
            (double)color.a / 255.0);
}

static int command_is_visible(const KitRenderCommand *cmd) {
    if (!cmd) {
        return 0;
    }
    switch (cmd->kind) {
        case KIT_RENDER_CMD_RECT:
            return cmd->data.rect.color.a != 0u &&
                   cmd->data.rect.rect.width > 0.0f &&
                   cmd->data.rect.rect.height > 0.0f;
        case KIT_RENDER_CMD_LINE:
            return cmd->data.line.color.a != 0u && cmd->data.line.thickness > 0.0f;
        case KIT_RENDER_CMD_TEXTURED_QUAD:
            return cmd->data.textured_quad.tint.a != 0u &&
                   cmd->data.textured_quad.rect.width > 0.0f &&
                   cmd->data.textured_quad.rect.height > 0.0f;
        case KIT_RENDER_CMD_TEXT:
            return cmd->data.text.text && cmd->data.text.text[0] != '\0';
        default:
            return 0;
    }
}

static void write_svg_command(FILE *out, const KitRenderCommand *cmd) {
    if (!out || !cmd) {
        return;
    }

    switch (cmd->kind) {
        case KIT_RENDER_CMD_CLEAR:
            fputs("  <rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" fill=\"", out);
            write_color(out, cmd->data.clear.color);
            fputs("\"/>\n", out);
            break;
        case KIT_RENDER_CMD_SET_CLIP:
            fprintf(out,
                    "  <!-- clip %.1f %.1f %.1f %.1f -->\n",
                    cmd->data.clip.rect.x,
                    cmd->data.clip.rect.y,
                    cmd->data.clip.rect.width,
                    cmd->data.clip.rect.height);
            break;
        case KIT_RENDER_CMD_CLEAR_CLIP:
            fputs("  <!-- clear clip -->\n", out);
            break;
        case KIT_RENDER_CMD_RECT:
            fprintf(out,
                    "  <rect x=\"%.1f\" y=\"%.1f\" width=\"%.1f\" height=\"%.1f\" rx=\"%.1f\" fill=\"",
                    cmd->data.rect.rect.x,
                    cmd->data.rect.rect.y,
                    cmd->data.rect.rect.width,
                    cmd->data.rect.rect.height,
                    cmd->data.rect.corner_radius);
            write_color(out, cmd->data.rect.color);
            fputs("\"/>\n", out);
            break;
        case KIT_RENDER_CMD_LINE:
            fprintf(out,
                    "  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" stroke=\"",
                    cmd->data.line.p0.x,
                    cmd->data.line.p0.y,
                    cmd->data.line.p1.x,
                    cmd->data.line.p1.y);
            write_color(out, cmd->data.line.color);
            fprintf(out,
                    "\" stroke-width=\"%.1f\" stroke-linecap=\"round\"/>\n",
                    cmd->data.line.thickness);
            break;
        case KIT_RENDER_CMD_TEXTURED_QUAD:
            fprintf(out,
                    "  <rect x=\"%.1f\" y=\"%.1f\" width=\"%.1f\" height=\"%.1f\" fill=\"",
                    cmd->data.textured_quad.rect.x,
                    cmd->data.textured_quad.rect.y,
                    cmd->data.textured_quad.rect.width,
                    cmd->data.textured_quad.rect.height);
            write_color(out, cmd->data.textured_quad.tint);
            fprintf(out,
                    "\" opacity=\"0.55\"><title>texture %llu</title></rect>\n",
                    (unsigned long long)cmd->data.textured_quad.texture_id);
            break;
        case KIT_RENDER_CMD_TEXT:
            fprintf(out,
                    "  <text x=\"%.1f\" y=\"%.1f\" font-family=\"monospace\" font-size=\"13\" fill=\"currentColor\">",
                    cmd->data.text.origin.x,
                    cmd->data.text.origin.y);
            write_xml_text(out, cmd->data.text.text);
            fputs("</text>\n", out);
            break;
        case KIT_RENDER_CMD_POLYLINE:
        default:
            break;
    }
}

static int write_svg_artifact(const char *path,
                              const KitRenderCommandBuffer *commands,
                              uint32_t width_px,
                              uint32_t height_px) {
    FILE *out;
    size_t i;
    size_t visible_count = 0u;

    if (!path || !path[0] || !commands || !commands->commands || commands->count == 0u) {
        return 0;
    }
    if (!mem_console_ensure_parent_directory(path)) {
        fprintf(stderr, "mem_console: visual-artifact failed to create parent directory: %s\n", path);
        return 0;
    }

    for (i = 0u; i < commands->count; ++i) {
        if (command_is_visible(&commands->commands[i])) {
            visible_count += 1u;
        }
    }
    if (visible_count == 0u) {
        fprintf(stderr, "mem_console: visual-artifact frame was empty\n");
        return 0;
    }

    out = fopen(path, "w");
    if (!out) {
        fprintf(stderr, "mem_console: visual-artifact failed to open output: %s\n", path);
        return 0;
    }

    fprintf(out,
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%u\" height=\"%u\" viewBox=\"0 0 %u %u\" "
            "color=\"rgb(226,232,240)\">\n",
            (unsigned int)width_px,
            (unsigned int)height_px,
            (unsigned int)width_px,
            (unsigned int)height_px);
    fprintf(out,
            "  <title>mem_console visual artifact first frame</title>\n"
            "  <desc>Recorded from the app-owned first-frame render command stream. commands=%zu visible=%zu</desc>\n",
            commands->count,
            visible_count);
    for (i = 0u; i < commands->count; ++i) {
        write_svg_command(out, &commands->commands[i]);
    }
    fputs("</svg>\n", out);

    if (fclose(out) != 0) {
        fprintf(stderr, "mem_console: visual-artifact failed to close output: %s\n", path);
        return 0;
    }

    printf("visual-artifact: %s\n", path);
    return 1;
}

int mem_console_visual_artifact_capture_if_requested(const KitRenderCommandBuffer *commands,
                                                     uint32_t width_px,
                                                     uint32_t height_px) {
    const char *path;

    if (s_mem_console_visual_artifact_captured) {
        return 0;
    }

    path = getenv("MEM_CONSOLE_VISUAL_ARTIFACT_PATH");
    if (!path || !path[0]) {
        return 0;
    }

    if (!write_svg_artifact(path, commands, width_px, height_px)) {
        return -1;
    }
    s_mem_console_visual_artifact_captured = 1;
    return 1;
}

static const char *visual_artifact_flag_value(int argc, char **argv, const char *flag) {
    int i;

    if (!flag) {
        return NULL;
    }
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            if ((i + 1) >= argc) {
                return NULL;
            }
            return argv[i + 1];
        }
    }
    return NULL;
}

static int visual_artifact_has_flag(int argc, char **argv, const char *flag) {
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

static int visual_artifact_parse_mode(const char *text, int *out_mode) {
    if (!out_mode) {
        return 0;
    }
    if (!text || !text[0] || strcmp(text, "focus") == 0 || strcmp(text, "FOCUS") == 0) {
        *out_mode = MEM_CONSOLE_GRAPH_VIEW_FOCUS;
        return 1;
    }
    if (strcmp(text, "pods") == 0 || strcmp(text, "PODS") == 0) {
        *out_mode = MEM_CONSOLE_GRAPH_VIEW_PODS;
        return 1;
    }
    if (strcmp(text, "web") == 0 || strcmp(text, "WEB") == 0) {
        *out_mode = MEM_CONSOLE_GRAPH_VIEW_WEB;
        return 1;
    }
    return 0;
}

static int visual_artifact_parse_item_id(const char *text, int64_t *out_item_id) {
    char *end = NULL;
    long long value = 0;

    if (!out_item_id) {
        return 0;
    }
    *out_item_id = 0;
    if (!text || !text[0]) {
        return 1;
    }
    value = strtoll(text, &end, 10);
    if (!end || *end != '\0' || value <= 0) {
        return 0;
    }
    *out_item_id = (int64_t)value;
    return 1;
}

int mem_console_visual_artifact_run_cli(int argc, char **argv) {
    const char *artifact_path = visual_artifact_flag_value(argc, argv, "--visual-artifact");
    const char *db_path = visual_artifact_flag_value(argc, argv, "--db");
    const char *mode_text = visual_artifact_flag_value(argc, argv, "--visual-review-mode");
    const char *selected_id_text = visual_artifact_flag_value(argc, argv, "--visual-review-selected-id");
    MemConsoleState state;
    CoreMemDb db;
    KitRenderContext render_ctx;
    KitUiContext ui_ctx;
    KitUiInputState input;
    MemConsoleAction action;
    CoreResult result;
    int mode = MEM_CONSOLE_GRAPH_VIEW_FOCUS;
    int64_t selected_id = 0;
    int frame_result;
    int db_open = 0;
    int render_ctx_initialized = 0;
    int ui_ctx_initialized = 0;

    if (!artifact_path || !artifact_path[0]) {
        fprintf(stderr, "mem_console: --visual-artifact requires an output path\n");
        return 1;
    }
    if (visual_artifact_has_flag(argc, argv, "--visual-review-mode") &&
        !visual_artifact_parse_mode(mode_text, &mode)) {
        fprintf(stderr, "mem_console: invalid --visual-review-mode value.\n");
        return 1;
    }
    if (visual_artifact_has_flag(argc, argv, "--visual-review-selected-id") &&
        !visual_artifact_parse_item_id(selected_id_text, &selected_id)) {
        fprintf(stderr, "mem_console: invalid --visual-review-selected-id value.\n");
        return 1;
    }
    if (!db_path || !db_path[0]) {
        fprintf(stderr, "mem_console: --visual-artifact requires --db <path> for deterministic proof\n");
        return 1;
    }

    if (setenv("MEM_CONSOLE_VISUAL_ARTIFACT_PATH", artifact_path, 1) != 0) {
        fprintf(stderr, "mem_console: failed to set visual artifact output path\n");
        return 1;
    }

    seed_state(&state, db_path);
    mem_console_state_set_path_contract(&state, "", "", db_path);
    result = core_memdb_open(db_path, &db);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: visual-artifact DB open failed: %s\n", result.message);
        return 1;
    }
    db_open = 1;

    result = refresh_state_from_db(&db, &state);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: visual-artifact DB refresh failed: %s\n", result.message);
        goto fail;
    }
    sync_edit_buffers_from_selection(&state);
    if (selected_id > 0) {
        mem_console_selection_center_on(&state, selected_id);
    }
    (void)mem_console_graph_view_mode_set(&state, mode);
    mem_console_graph_view_mode_reset_viewport(&state);
    state.graph_edge_labels_enabled = 1;
    state.graph_edge_limit_cursor = 0;
    mem_console_graph_edge_limit_set(&state, 1024);
    mem_console_app_set_statusf(&state, "Visual artifact first-frame mode active.");
    mem_console_redraw_mark(&state, MEM_CONSOLE_REDRAW_REASON_CONTENT);

    result = kit_render_context_init(&render_ctx,
                                     KIT_RENDER_BACKEND_NULL,
                                     state.theme_preset_id,
                                     state.font_preset_id);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: visual-artifact render init failed: %d\n", (int)result.code);
        goto fail;
    }
    render_ctx_initialized = 1;
    (void)kit_render_set_text_zoom_step(&render_ctx, state.text_zoom_step);
    result = kit_ui_context_init(&ui_ctx, &render_ctx);
    if (result.code != CORE_OK) {
        fprintf(stderr, "mem_console: visual-artifact UI init failed: %d\n", (int)result.code);
        goto fail;
    }
    ui_ctx_initialized = 1;
    (void)kit_ui_style_apply_theme_scale(&ui_ctx);
    mem_console_app_apply_compact_ui_density(&ui_ctx, &render_ctx);

    memset(&input, 0, sizeof(input));
    action = MEM_CONSOLE_ACTION_NONE;
    frame_result = run_frame(&render_ctx, &ui_ctx, &state, &input, 1440, 900, 0, &action);
    if (frame_result != MEM_CONSOLE_FRAME_OK) {
        fprintf(stderr, "mem_console: visual-artifact frame failed: %d\n", frame_result);
        goto fail;
    }

    if (ui_ctx_initialized) {
        ui_ctx_initialized = 0;
    }
    if (render_ctx_initialized) {
        kit_render_context_shutdown(&render_ctx);
        render_ctx_initialized = 0;
    }
    if (db_open) {
        (void)core_memdb_close(&db);
        db_open = 0;
    }
    return 0;

fail:
    if (ui_ctx_initialized) {
        ui_ctx_initialized = 0;
    }
    if (render_ctx_initialized) {
        kit_render_context_shutdown(&render_ctx);
    }
    if (db_open) {
        (void)core_memdb_close(&db);
    }
    return 1;
}
