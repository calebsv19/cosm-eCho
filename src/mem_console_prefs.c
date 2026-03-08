#include "mem_console_prefs.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

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

enum {
    MEM_CONSOLE_UI_PREFS_VERSION = 2u
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
    MemConsoleUiPrefsV2 prefs_v2 = {0};
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
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV2)) {
        (void)core_pack_reader_close(&reader);
        return (CoreResult){ CORE_ERR_FORMAT, "invalid mem_console prefs payload size" };
    }

    result = core_pack_reader_read_chunk_data(&reader, &chunk, &prefs_v2, sizeof(prefs_v2));
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return result;
    }

    if (prefs_v2.version == 1u && chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV1)) {
        if (state_set_theme_preset(state, (CoreThemePresetId)prefs_v2.theme_preset_id)) {
            loaded_any = 1;
        }
        if (state_set_font_preset(state, (CoreFontPresetId)prefs_v2.font_preset_id)) {
            loaded_any = 1;
        }
    } else if (prefs_v2.version == MEM_CONSOLE_UI_PREFS_VERSION &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV2)) {
        if (state_set_theme_preset(state, (CoreThemePresetId)prefs_v2.theme_preset_id)) {
            loaded_any = 1;
        }
        if (state_set_font_preset(state, (CoreFontPresetId)prefs_v2.font_preset_id)) {
            loaded_any = 1;
        }
        state->pane_left_ratio = prefs_ratio_or_default(prefs_v2.pane_left_ratio);
        state->pane_right_split_ratio = prefs_ratio_or_default(prefs_v2.pane_right_split_ratio);
        state->pane_left_collapsed = prefs_v2.pane_left_collapsed ? 1 : 0;
        state->pane_right_detail_collapsed = prefs_v2.pane_right_detail_collapsed ? 1 : 0;
        loaded_any = 1;
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
    MemConsoleUiPrefsV2 prefs = {0};

    if (!prefs_path || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid prefs save request" };
    }

    prefs.version = MEM_CONSOLE_UI_PREFS_VERSION;
    prefs.theme_preset_id = (int32_t)state->theme_preset_id;
    prefs.font_preset_id = (int32_t)state->font_preset_id;
    prefs.pane_left_ratio = prefs_ratio_or_default(state->pane_left_ratio);
    prefs.pane_right_split_ratio = prefs_ratio_or_default(state->pane_right_split_ratio);
    prefs.pane_left_collapsed = state->pane_left_collapsed ? 1 : 0;
    prefs.pane_right_detail_collapsed = state->pane_right_detail_collapsed ? 1 : 0;

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
