#include "mem_console_prefs.h"

#include <stdio.h>

#include "core_base.h"
#include "core_pack.h"
#include "runtime/mem_console_prefs_app_io_internal.h"
#include "runtime/mem_console_prefs_internal.h"

int mem_console_build_prefs_path(const char *db_path, char *out_path, size_t out_cap) {
    return mem_console_build_prefs_path_impl(db_path, out_path, out_cap);
}

int mem_console_build_app_prefs_path(char *out_path, size_t out_cap) {
    return mem_console_build_app_prefs_path_impl(out_path, out_cap);
}

int mem_console_build_app_prefs_path_for_output_root(const char *output_root,
                                                     char *out_path,
                                                     size_t out_cap) {
    return mem_console_build_app_prefs_path_for_output_root_impl(output_root, out_path, out_cap);
}

CoreResult mem_console_prefs_load(const char *prefs_path, MemConsoleState *state) {
    CorePackReader reader = {0};
    CorePackChunkInfo chunk = {0};
    CoreResult result;
    MemConsoleUiPrefsV11 prefs = {0};
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
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV3) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV4) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV5) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV6) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV7) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV8) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV9) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV10) &&
        chunk.size != (uint64_t)sizeof(MemConsoleUiPrefsV11)) {
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
    } else if (prefs.version == 3u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV3)) {
        if (prefs_apply_v3_to_state((const MemConsoleUiPrefsV3 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 4u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV4)) {
        if (prefs_apply_v5_to_state((const MemConsoleUiPrefsV5 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 5u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV5)) {
        if (prefs_apply_v5_to_state((const MemConsoleUiPrefsV5 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 6u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV6)) {
        if (prefs_apply_v6_to_state((const MemConsoleUiPrefsV6 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 7u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV7)) {
        if (prefs_apply_v7_to_state((const MemConsoleUiPrefsV7 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 8u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV8)) {
        if (prefs_apply_v8_to_state((const MemConsoleUiPrefsV8 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 9u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV9)) {
        if (prefs_apply_v9_to_state((const MemConsoleUiPrefsV9 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == 10u &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV10)) {
        if (prefs_apply_v10_to_state((const MemConsoleUiPrefsV10 *)&prefs, state)) {
            loaded_any = 1;
        }
    } else if (prefs.version == MEM_CONSOLE_UI_PREFS_VERSION &&
               chunk.size == (uint64_t)sizeof(MemConsoleUiPrefsV11)) {
        if (prefs_apply_v11_to_state(&prefs, state)) {
            loaded_any = 1;
        }
    }

    result = core_pack_reader_close(&reader);
    if (result.code != CORE_OK) {
        return result;
    }

    if (loaded_any) {
        state->graph_mode_enabled = 1;
        mem_console_pane_prefs_mark_clean(state);
        return (CoreResult){ CORE_OK, "prefs loaded" };
    }
    return core_result_ok();
}

CoreResult mem_console_prefs_save(const char *prefs_path, const MemConsoleState *state) {
    CorePackWriter writer = {0};
    CoreResult result;
    MemConsoleUiPrefsV11 prefs = {0};

    if (!prefs_path || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid prefs save request" };
    }
    if (!mem_console_ensure_parent_directory(prefs_path)) {
        return (CoreResult){ CORE_ERR_IO, "failed to create prefs directory" };
    }

    prefs_build_v11_from_state(state, &prefs);

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
    MemConsoleUiPrefsV11 prefs = {0};

    if (!state) {
        return 0u;
    }

    prefs_build_v11_from_state(state, &prefs);
    return core_hash64_fnv1a(&prefs, sizeof(prefs));
}
