#ifndef MEM_CONSOLE_LAYOUT_CONFIG_H
#define MEM_CONSOLE_LAYOUT_CONFIG_H

typedef struct MemConsoleLayoutConfig {
    int min_frame_width;
    int min_frame_height;
    float outer_margin;
    float pane_gap;
    float left_pane_width;
    float panel_inner_padding;

    float left_header_h;
    float left_info_row_h;
    float left_reload_h;
    float left_section_h;
    float left_search_h;
    float left_project_filters_h;
    float left_results_header_h;
    float left_status_h;

    float right_header_h;
    float right_title_h;
    float right_meta_h;
    float right_section_h;
    float right_body_h;

    float graph_filter_h;
    float graph_settings_h;
    float graph_collapsed_hint_h;
    float action_button_h;
    float action_button_gap;
    float action_block_pad;
    float graph_panel_min_h;

    float list_row_pitch;
    float list_item_h;
} MemConsoleLayoutConfig;

const MemConsoleLayoutConfig *mem_console_layout_config_get(void);

#endif
