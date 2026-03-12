#include "mem_console_layout_config.h"

static const MemConsoleLayoutConfig k_default_layout = {
    .min_frame_width = 1080,
    .min_frame_height = 720,
    .outer_margin = 0.0f,
    .pane_gap = 12.0f,
    .left_pane_width = 330.0f,
    .panel_inner_padding = 8.0f,

    .left_header_h = 22.0f,
    .left_info_row_h = 16.0f,
    .left_reload_h = 22.0f,
    .left_section_h = 14.0f,
    .left_search_h = 24.0f,
    .left_project_filters_h = 62.0f,
    .left_results_header_h = 14.0f,
    .left_status_h = 14.0f,

    .right_header_h = 20.0f,
    .right_title_h = 18.0f,
    .right_meta_h = 74.0f,
    .right_section_h = 12.0f,
    .right_body_h = 86.0f,

    .graph_filter_h = 22.0f,
    .graph_settings_h = 22.0f,
    .graph_collapsed_hint_h = 16.0f,
    .action_button_h = 18.0f,
    .action_button_gap = 3.0f,
    .action_block_pad = 3.0f,
    .graph_panel_min_h = 220.0f,

    .list_row_pitch = 32.0f,
    .list_item_h = 26.0f
};

const MemConsoleLayoutConfig *mem_console_layout_config_get(void) {
    return &k_default_layout;
}
