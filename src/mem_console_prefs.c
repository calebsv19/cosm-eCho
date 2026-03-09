#include "mem_console_prefs.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core_base.h"
#include "core_pack.h"

typedef struct MemConsoleUiPrefsV1 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
} MemConsoleUiPrefsV1;

typedef struct MemConsoleUiPrefsV2 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
} MemConsoleUiPrefsV2;

typedef struct MemConsoleUiPrefsV3 {
    uint32_t version;
    int32_t theme_preset_id;
    int32_t font_preset_id;
    float pane_left_ratio;
    float pane_right_split_ratio;
    int32_t pane_left_collapsed;
    int32_t pane_right_detail_collapsed;
    int64_t selected_item_id;
    int32_t list_query_offset;
    int32_t selected_project_count;
    char selected_project_keys[MEM_CONSOLE_SCOPE_FILTER_LIMIT][64];
    char graph_kind_filter[32];
    int32_t graph_edge_limit;
    int32_t graph_hops;
    int32_t graph_mode_enabled;
    float graph_pan_x;
    float graph_pan_y;
    float graph_zoom;
    char search_text[256];
} MemConsoleUiPrefsV3;

enum {
    MEM_CONSOLE_UI_PREFS_VERSION = 3u
};

static float prefs_ratio_or_default(float ratio) {
    if (!isfinite(ratio)) {
        return 0.0f;
    }
    if (ratio <= 0.01f || ratio >= 0.99f) {
        return 0.0f;
    }
    return ratio;
}

static float prefs_viewport_component_or_default(float value, float fallback) {
    if (!isfinite(value)) {
        return fallback;
    }
    return value;
}

static float prefs_viewport_zoom_or_default(float zoom) {
    float next_zoom = zoom;

    if (!isfinite(next_zoom) || next_zoom <= 0.0f) {
        return 1.0f;
    }
    if (next_zoom < 0.1f) {
        next_zoom = 0.1f;
    }
    if (next_zoom > 8.0f) {
        next_zoom = 8.0f;
    }
    return next_zoom;
}

static void prefs_copy_project_filters_from_state(MemConsoleUiPrefsV3 *prefs,
                                                  const MemConsoleState *state) {
    int i;
    int write_index = 0;

    if (!prefs || !state) {
        return;
    }

    for (i = 0; i < state->selected_project_count && i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        if (state->selected_project_keys[i][0] == '\0') {
            continue;
        }
        (void)snprintf(prefs->selected_project_keys[write_index],
                       sizeof(prefs->selected_project_keys[write_index]),
                       "%s",
                       state->selected_project_keys[i]);
        write_index += 1;
        if (write_index >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
            break;
        }
    }

    prefs->selected_project_count = write_index;
}

static void prefs_copy_project_filters_to_state(const MemConsoleUiPrefsV3 *prefs,
                                                MemConsoleState *state) {
    int i;
    int write_index = 0;
    int load_count;

    if (!prefs || !state) {
        return;
    }

    load_count = prefs->selected_project_count;
    if (load_count < 0) {
        load_count = 0;
    }
    if (load_count > MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
        load_count = MEM_CONSOLE_SCOPE_FILTER_LIMIT;
    }

    for (i = 0; i < load_count; ++i) {
        if (prefs->selected_project_keys[i][0] == '\0') {
            continue;
        }
        (void)snprintf(state->selected_project_keys[write_index],
                       sizeof(state->selected_project_keys[write_index]),
                       "%s",
                       prefs->selected_project_keys[i]);
        write_index += 1;
        if (write_index >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
            break;
        }
    }

    for (i = write_index; i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        state->selected_project_keys[i][0] = '\0';
    }
    state->selected_project_count = write_index;
}

static void prefs_build_v3_from_state(const MemConsoleState *state, MemConsoleUiPrefsV3 *out_prefs) {
    if (!state || !out_prefs) {
        return;
    }

    memset(out_prefs, 0, sizeof(*out_prefs));
    out_prefs->version = MEM_CONSOLE_UI_PREFS_VERSION;
    out_prefs->theme_preset_id = (int32_t)state->theme_preset_id;
    out_prefs->font_preset_id = (int32_t)state->font_preset_id;
    out_prefs->pane_left_ratio = prefs_ratio_or_default(state->pane_left_ratio);
    out_prefs->pane_right_split_ratio = prefs_ratio_or_default(state->pane_right_split_ratio);
    out_prefs->pane_left_collapsed = state->pane_left_collapsed ? 1 : 0;
    out_prefs->pane_right_detail_collapsed = state->pane_right_detail_collapsed ? 1 : 0;
    out_prefs->selected_item_id = state->selected_item_id;
    out_prefs->list_query_offset = state->list_query_offset > 0 ? state->list_query_offset : 0;
    prefs_copy_project_filters_from_state(out_prefs, state);
    (void)snprintf(out_prefs->graph_kind_filter,
                   sizeof(out_prefs->graph_kind_filter),
                   "%s",
                   state->graph_kind_filter);
    out_prefs->graph_edge_limit = mem_console_graph_edge_limit_clamp(state->graph_query_edge_limit);
    out_prefs->graph_hops = mem_console_graph_hops_clamp(state->graph_query_hops);
    out_prefs->graph_mode_enabled = state->graph_mode_enabled ? 1 : 0;
    out_prefs->graph_pan_x = prefs_viewport_component_or_default(state->graph_viewport.pan_x, 0.0f);
    out_prefs->graph_pan_y = prefs_viewport_component_or_default(state->graph_viewport.pan_y, 0.0f);
    out_prefs->graph_zoom = prefs_viewport_zoom_or_default(state->graph_viewport.zoom);
    (void)snprintf(out_prefs->search_text,
                   sizeof(out_prefs->search_text),
                   "%s",
                   state->search_text);
}

static int prefs_apply_v3_to_state(const MemConsoleUiPrefsV3 *prefs, MemConsoleState *state) {
    if (!prefs || !state) {
        return 0;
    }

    (void)state_set_theme_preset(state, (CoreThemePresetId)prefs->theme_preset_id);
    (void)state_set_font_preset(state, (CoreFontPresetId)prefs->font_preset_id);
    state->pane_left_ratio = prefs_ratio_or_default(prefs->pane_left_ratio);
    state->pane_right_split_ratio = prefs_ratio_or_default(prefs->pane_right_split_ratio);
    state->pane_left_collapsed = prefs->pane_left_collapsed ? 1 : 0;
    state->pane_right_detail_collapsed = prefs->pane_right_detail_collapsed ? 1 : 0;

    state->selected_item_id = prefs->selected_item_id > 0 ? prefs->selected_item_id : 0;
    state->list_query_offset = prefs->list_query_offset > 0 ? prefs->list_query_offset : 0;
    prefs_copy_project_filters_to_state(prefs, state);

    (void)snprintf(state->graph_kind_filter,
                   sizeof(state->graph_kind_filter),
                   "%s",
                   prefs->graph_kind_filter);
    mem_console_graph_edge_limit_set(state,
                                     mem_console_graph_edge_limit_clamp((int)prefs->graph_edge_limit));
    state->graph_query_hops = mem_console_graph_hops_clamp((int)prefs->graph_hops);
    state->graph_mode_enabled = prefs->graph_mode_enabled ? 1 : 0;

    state->graph_viewport.pan_x = prefs_viewport_component_or_default(prefs->graph_pan_x, 0.0f);
    state->graph_viewport.pan_y = prefs_viewport_component_or_default(prefs->graph_pan_y, 0.0f);
    state->graph_viewport.zoom = prefs_viewport_zoom_or_default(prefs->graph_zoom);
    (void)snprintf(state->search_text, sizeof(state->search_text), "%s", prefs->search_text);

    return 1;
}

int mem_console_build_prefs_path(const char *db_path, char *out_path, size_t out_cap) {
    int written = 0;

    if (!db_path || !out_path || out_cap == 0u) {
        return 0;
    }

    written = snprintf(out_path, out_cap, "%s.ui.pack", db_path);
    if (written <= 0 || (size_t)written >= out_cap) {
        if (out_cap > 0u) {
            out_path[0] = '\0';
        }
        return 0;
    }
    return 1;
}

CoreResult mem_console_prefs_load(const char *prefs_path, MemConsoleState *state) {
    CorePackReader reader = {0};
    CorePackChunkInfo chunk = {0};
    CoreResult result;
    MemConsoleUiPrefsV3 prefs = {0};
    FILE *probe = 0;
    int loaded_any = 0;

    if (!prefs_path || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid prefs load request" };
    }

    probe = fopen(prefs_path, "rb");
    if (!probe) {
        return core_result_ok();
    }
    fclose(probe);

    result = core_pack_reader_open(prefs_path, &reader);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_pack_reader_find_chunk(&reader, "MCFG", 0, &chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return core_result_ok();
    }

    if (chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV1) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV2) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV3)) {
        (void)core_pack_reader_close(&reader);
        return (CoreResult){ CORE_ERR_FORMAT, "invalid mem_console prefs payload size" };
    }

    result = core_pack_reader_read_chunk_data(&reader, &chunk, &prefs, sizeof(prefs));
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return result;
    }

    if (prefs.version == 1u && chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV1)) {
        if (state_set_theme_preset(state, (CoreThemePresetId)prefs.theme_preset_id)) {
            loaded_any = 1;
        }
        if (state_set_font_preset(state, (CoreFontPresetId)prefs.font_preset_id)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 2u && chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV2)) {
        if (state_set_theme_preset(state, (CoreThemePresetId)prefs.theme_preset_id)) {
            loaded_any = 1;
        }
        if (state_set_font_preset(state, (CoreFontPresetId)prefs.font_preset_id)) {
            loaded_any = 1;
        }
        state->pane_left_ratio = prefs_ratio_or_default(prefs.pane_left_ratio);
        state->pane_right_split_ratio = prefs_ratio_or_default(prefs.pane_right_split_ratio);
        state->pane_left_collapsed = prefs.pane_left_collapsed ? 1 : 0;
        state->pane_right_detail_collapsed = prefs.pane_right_detail_collapsed ? 1 : 0;
        loaded_any = 1;
    } else if (prefs.version == MEM_CONSOLE_UI_PREFS_VERSION &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV3)) {
        if (prefs_apply_v3_to_state(&prefs, state)) {
            loaded_any = 1;
        }
    }

    result = core_pack_reader_close(&reader);
    if (result.code != CORE_OK) {
        return result;
    }

    if (loaded_any) {
        state->pane_prefs_dirty = 0;
        return (CoreResult){ CORE_OK, "prefs loaded" };
    }
    return core_result_ok();
}

CoreResult mem_console_prefs_save(const char *prefs_path, const MemConsoleState *state) {
    CorePackWriter writer = {0};
    CoreResult result;
    MemConsoleUiPrefsV3 prefs = {0};

    if (!prefs_path || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid prefs save request" };
    }

    prefs_build_v3_from_state(state, &prefs);

    result = core_pack_writer_open(prefs_path, &writer);
    if (result.code != CORE_OK) {
        return result;
    }

    result = core_pack_writer_add_chunk(&writer, "MCFG", &prefs, (uint64_t)sizeof(prefs));
    if (result.code != CORE_OK) {
        (void)core_pack_writer_close(&writer);
        return result;
    }

    result = core_pack_writer_close(&writer);
    if (result.code != CORE_OK) {
        return result;
    }

    return core_result_ok();
}

uint64_t mem_console_prefs_state_signature(const MemConsoleState *state) {
    MemConsoleUiPrefsV3 prefs = {0};

    if (!state) {
        return 0u;
    }

    prefs_build_v3_from_state(state, &prefs);
    return core_hash64_fnv1a(&prefs, sizeof(prefs));
}
