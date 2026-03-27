#ifndef MEM_CONSOLE_STATE_H
#define MEM_CONSOLE_STATE_H

#include <stddef.h>

#include "mem_console_types.h"

extern const char *k_mem_console_default_db_path;

int mem_console_path_is_directory(const char *path);
int mem_console_path_exists(const char *path);
int mem_console_ensure_directory(const char *path);
int mem_console_ensure_parent_directory(const char *path);
int mem_console_resolve_app_data_dir(char *out_path, size_t out_cap);
int resolve_default_db_path(char *out_path, size_t out_cap);
void print_usage(const char *argv0);
const char *find_flag_value(int argc, char **argv, const char *flag);
int has_flag(int argc, char **argv, const char *flag);
int has_unknown_flag(int argc, char **argv);

void set_default_detail(MemConsoleState *state);
void copy_core_str(CoreStr value, char *out_text, size_t out_cap);

int cycle_theme_preset(MemConsoleState *state, int direction);
int cycle_font_preset(MemConsoleState *state, int direction);
int state_set_theme_preset(MemConsoleState *state, CoreThemePresetId preset_id);
int state_set_font_preset(MemConsoleState *state, CoreFontPresetId preset_id);

void seed_state(MemConsoleState *state, const char *db_path);
void compute_layout(MemConsoleState *state, int frame_width, int frame_height);
void build_like_pattern(const char *search_text, char *out_pattern, size_t out_cap);
int mem_console_graph_edge_limit_clamp(int value);
int mem_console_graph_edge_limit_parse(const char *text, int fallback);
void mem_console_graph_edge_limit_set(MemConsoleState *state, int value);
int mem_console_graph_hops_clamp(int value);
int mem_console_graph_layout_mode_clamp(int value);
int mem_console_graph_sort_mode_clamp(int value);
uint32_t mem_console_graph_kind_filter_all_mask(void);
uint32_t mem_console_graph_kind_filter_mask_for_kind(const char *kind);
int mem_console_graph_kind_is_enabled(const MemConsoleState *state, const char *kind);
int mem_console_graph_kind_toggle_enabled(MemConsoleState *state, const char *kind);
void mem_console_graph_kind_select_all(MemConsoleState *state);
int mem_console_graph_kind_toggle_all_override(MemConsoleState *state);
void mem_console_graph_kind_set_single(MemConsoleState *state, const char *kind);
void mem_console_graph_kind_sync_text_filter(MemConsoleState *state);
uint32_t mem_console_graph_node_kind_filter_all_mask(void);
uint32_t mem_console_graph_node_kind_filter_mask_for_kind(const char *kind);
int mem_console_graph_node_kind_is_enabled(const MemConsoleState *state, const char *kind);
int mem_console_graph_node_kind_toggle_enabled(MemConsoleState *state, const char *kind);
void mem_console_graph_node_kind_select_all(MemConsoleState *state);
int mem_console_graph_node_kind_toggle_all_override(MemConsoleState *state);
int mem_console_graph_anchor_hidden_is_set(const MemConsoleState *state, int64_t item_id);
int mem_console_graph_anchor_hidden_set(MemConsoleState *state, int64_t item_id, int hidden);
int mem_console_graph_anchor_hidden_toggle(MemConsoleState *state, int64_t item_id, int *out_hidden);

void format_text_for_width(char *out_text,
                           size_t out_cap,
                           const char *source_text,
                           float width_px,
                           CoreFontTextSizeTier text_tier);
int mem_console_project_filter_is_selected(const MemConsoleState *state, const char *project_key);
void mem_console_project_filter_clear(MemConsoleState *state);
int mem_console_project_filter_toggle(MemConsoleState *state, const char *project_key);
void mem_console_project_filter_prune_to_options(MemConsoleState *state);

int selected_id_in_visible_items(const MemConsoleState *state);
void sync_edit_buffers_from_selection(MemConsoleState *state);
void begin_title_edit_mode(MemConsoleState *state);
void cancel_title_edit_mode(MemConsoleState *state);
void begin_body_edit_mode(MemConsoleState *state);
void cancel_body_edit_mode(MemConsoleState *state);
void begin_db_picker_mode(MemConsoleState *state, int create_mode);
void cancel_db_picker_mode(MemConsoleState *state);
int mem_console_db_picker_build_path(const MemConsoleState *state,
                                     char *out_path,
                                     size_t out_cap);
int mem_console_db_picker_has_selection(const MemConsoleState *state);
void mem_console_db_picker_select_all(MemConsoleState *state);
void mem_console_db_picker_begin_selection(MemConsoleState *state, int cursor_index);
void mem_console_db_picker_update_selection(MemConsoleState *state, int cursor_index);
void mem_console_db_picker_end_selection(MemConsoleState *state);
void append_active_input_text(MemConsoleState *state, const char *text);
void erase_active_input_char(MemConsoleState *state);
void delete_active_input_char(MemConsoleState *state);
void move_active_input_cursor(MemConsoleState *state, int delta);
void move_active_input_cursor_home(MemConsoleState *state);
void move_active_input_cursor_end(MemConsoleState *state);
int active_input_is_search(const MemConsoleState *state);
void mem_console_redraw_mark(MemConsoleState *state, uint32_t reasons);
uint32_t mem_console_redraw_pending(const MemConsoleState *state);
uint32_t mem_console_redraw_take_pending(MemConsoleState *state);
void mem_console_redraw_note_frame(MemConsoleState *state, uint32_t reasons, uint64_t now_ms);

#endif
