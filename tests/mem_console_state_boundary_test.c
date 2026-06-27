#include "mem_console_action_roles.h"
#include "mem_console_state_roles.h"

#include <stdio.h>
#include <string.h>

static int expect_true(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "state-boundary contract failed: %s\n", message);
        return 0;
    }
    return 1;
}

static int contains_text(const char *text, const char *needle) {
    return text && needle && strstr(text, needle) != 0;
}

static int check_left_panel_render_derivation(void) {
    MemConsoleState state;
    MemConsoleLeftPanelRenderState view;
    MemConsoleLeftPanelRenderStorage storage;

    memset(&state, 0, sizeof(state));
    state.db_path = state.db_path_storage;
    (void)snprintf(state.db_path_storage, sizeof(state.db_path_storage), "/tmp/example.sqlite");
    (void)snprintf(state.input_root, sizeof(state.input_root), "/tmp/mem-input-root");
    (void)snprintf(state.schema_version, sizeof(state.schema_version), "6");
    (void)snprintf(state.theme_name, sizeof(state.theme_name), "echo-dark");
    (void)snprintf(state.status_line, sizeof(state.status_line), "Ready for boundary test.");
    state.active_count = 42;
    state.matching_count = 7;
    state.visible_count = 1;
    state.visible_items[0].id = 101;
    state.visible_items[0].updated_ns = 1710000000000000000LL;
    state.visible_items[0].pinned = 1;
    state.visible_items[0].canonical = 1;
    (void)snprintf(state.visible_items[0].project_key,
                   sizeof(state.visible_items[0].project_key),
                   "memory_console");
    (void)snprintf(state.visible_items[0].kind,
                   sizeof(state.visible_items[0].kind),
                   "plan");
    (void)snprintf(state.visible_items[0].title,
                   sizeof(state.visible_items[0].title),
                   "Boundary hardening");

    if (!expect_true(mem_console_left_panel_render_state_from_state(&state, &view),
                     "left render state view builds")) {
        return 0;
    }
    if (!expect_true(mem_console_left_panel_render_storage_from_state(&state, &storage),
                     "left render storage view builds")) {
        return 0;
    }

    mem_console_left_panel_derive_db_summary(&view, &storage, 320.0f);
    mem_console_left_panel_derive_input_root_summary(&view, &storage, 320.0f);
    mem_console_left_panel_derive_schema_summary(&view, &storage);
    mem_console_left_panel_derive_visible_summary(&view, &storage);
    mem_console_left_panel_derive_status_summary(&view, &storage, 320.0f);
    if (!expect_true(mem_console_left_panel_derive_item_label(&view, &storage, 0),
                     "list row label derives")) {
        return 0;
    }

    if (!expect_true(contains_text(state.db_summary_line, "example.sqlite"),
                     "DB summary uses DB path")) {
        return 0;
    }
    if (!expect_true(contains_text(state.input_root_summary_line, "/tmp/mem-input-root"),
                     "input root summary has dedicated storage")) {
        return 0;
    }
    if (!expect_true(strcmp(state.status_draw_line, state.input_root_summary_draw_line) != 0,
                     "status draw storage is distinct from input-root draw storage")) {
        return 0;
    }
    if (!expect_true(contains_text(state.schema_summary_line, "Active 42"),
                     "schema summary derives counts")) {
        return 0;
    }
    if (!expect_true(contains_text(state.visible_summary_line, "1 loaded"),
                     "visible summary derives visible count")) {
        return 0;
    }
    if (!expect_true(contains_text(state.list_item_labels[0], "101 P C [memory_console/plan]"),
                     "list item label derives item metadata")) {
        return 0;
    }

    return 1;
}

static int check_detail_render_derivation(void) {
    MemConsoleState state;
    MemConsoleDetailRenderState view;
    MemConsoleDetailRenderStorage storage;
    int title_lines;

    memset(&state, 0, sizeof(state));
    state.selected_item_id = 55;
    state.selected_created_ns = 1710000000000000000LL;
    (void)snprintf(state.selected_title,
                   sizeof(state.selected_title),
                   "Detail boundary render derivation");
    state.detail_relationship_count = 2;
    state.detail_relationship_out_count = 1;
    state.detail_relationship_in_count = 1;
    (void)snprintf(state.detail_relationship_summary_line,
                   sizeof(state.detail_relationship_summary_line),
                   "RELATIONSHIPS | OUT 1 | IN 1");

    state.detail_relationships[0].neighbor_item_id = 88;
    state.detail_relationships[0].outgoing = 1;
    (void)snprintf(state.detail_relationships[0].kind,
                   sizeof(state.detail_relationships[0].kind),
                   "supports");
    (void)snprintf(state.detail_relationships[0].neighbor_project_key,
                   sizeof(state.detail_relationships[0].neighbor_project_key),
                   "memory_console");
    (void)snprintf(state.detail_relationships[0].neighbor_kind,
                   sizeof(state.detail_relationships[0].neighbor_kind),
                   "plan");
    (void)snprintf(state.detail_relationships[0].neighbor_title,
                   sizeof(state.detail_relationships[0].neighbor_title),
                   "Neighbor Alpha");

    state.detail_relationships[1].neighbor_item_id = 89;
    state.detail_relationships[1].outgoing = 0;
    (void)snprintf(state.detail_relationships[1].kind,
                   sizeof(state.detail_relationships[1].kind),
                   "references");
    (void)snprintf(state.detail_relationships[1].neighbor_project_key,
                   sizeof(state.detail_relationships[1].neighbor_project_key),
                   "shared");
    (void)snprintf(state.detail_relationships[1].neighbor_kind,
                   sizeof(state.detail_relationships[1].neighbor_kind),
                   "decision");
    (void)snprintf(state.detail_relationships[1].neighbor_title,
                   sizeof(state.detail_relationships[1].neighbor_title),
                   "Neighbor Beta");

    if (!expect_true(mem_console_detail_render_state_from_state(&state, &view),
                     "detail render state view builds")) {
        return 0;
    }
    if (!expect_true(mem_console_detail_render_storage_from_state(&state, &storage),
                     "detail render storage view builds")) {
        return 0;
    }

    title_lines = mem_console_detail_derive_title_lines(&view, &storage, 3, 20);
    mem_console_detail_derive_meta_line(&view, &storage);
    if (!expect_true(title_lines > 0 && state.detail_title_line_count == title_lines,
                     "detail title lines derive into storage")) {
        return 0;
    }
    if (!expect_true(contains_text(state.detail_title_lines[0], "Detail"),
                     "detail title line preserves title text")) {
        return 0;
    }
    if (!expect_true(contains_text(state.detail_meta_line, "MEMORY ID 55"),
                     "detail meta line derives selected item id")) {
        return 0;
    }
    if (!expect_true(strcmp(mem_console_detail_relationship_header_label(&view),
                           "RELATIONSHIPS | OUT 1 | IN 1") == 0,
                     "detail relationship header uses summary line")) {
        return 0;
    }
    if (!expect_true(mem_console_detail_relationship_group_count(&view) == 2,
                     "detail relationship groups derive by direction and kind")) {
        return 0;
    }
    if (!expect_true(mem_console_detail_derive_relationship_group_label(&view,
                                                                        &storage,
                                                                        0,
                                                                        &state.detail_relationships[0]),
                     "detail relationship group label derives")) {
        return 0;
    }
    if (!expect_true(mem_console_detail_derive_relationship_row_label(&view,
                                                                      &storage,
                                                                      0,
                                                                      &state.detail_relationships[0]),
                     "detail relationship row label derives")) {
        return 0;
    }
    if (!expect_true(contains_text(state.detail_relationship_group_labels[0], "OUT supports"),
                     "detail relationship group label uses direction and kind")) {
        return 0;
    }
    if (!expect_true(contains_text(state.detail_relationship_row_labels[0],
                                   "-> 88 [memory_console] plan | Neighbor Alpha"),
                     "detail relationship row label uses neighbor metadata")) {
        return 0;
    }

    state.selected_item_id = 0;
    state.detail_relationship_count = 0;
    if (!expect_true(mem_console_detail_render_state_from_state(&state, &view),
                     "empty detail render view rebuilds")) {
        return 0;
    }
    mem_console_detail_derive_empty_relationship_line(&view, &storage);
    if (!expect_true(contains_text(state.detail_connection_summary_lines[0],
                                   "Select a memory"),
                     "empty relationship line derives from selected state")) {
        return 0;
    }

    return 1;
}

static int check_graph_render_derivation(void) {
    MemConsoleState state;
    MemConsoleGraphRenderState view;
    MemConsoleGraphRenderStorage storage;

    memset(&state, 0, sizeof(state));
    state.graph_node_count = 1;
    state.graph_edge_count = 1;
    state.graph_nodes[0].item_id = 20260616;
    state.graph_edges[0].from_index = 0;
    state.graph_edges[0].to_index = 0;
    (void)snprintf(state.graph_edges[0].kind,
                   sizeof(state.graph_edges[0].kind),
                   "implements");

    if (!expect_true(mem_console_graph_render_state_from_state(&state, &view),
                     "graph render state view builds")) {
        return 0;
    }
    if (!expect_true(mem_console_graph_render_storage_from_state(&state, &storage),
                     "graph render storage view builds")) {
        return 0;
    }
    if (!expect_true(mem_console_graph_derive_edge_label(&view,
                                                         &storage,
                                                         0,
                                                         "IMPLEMENTS"),
                     "graph edge label derives")) {
        return 0;
    }
    if (!expect_true(mem_console_graph_derive_node_label(&view, &storage, 0),
                     "graph node label derives")) {
        return 0;
    }
    if (!expect_true(strcmp(state.graph_draw_edge_labels[0], "IMPLEMENTS") == 0,
                     "graph edge label uses state-backed storage")) {
        return 0;
    }
    if (!expect_true(strcmp(state.graph_draw_node_labels[0], "20260616") == 0,
                     "graph node label uses graph item id")) {
        return 0;
    }
    if (!expect_true(mem_console_graph_derive_node_label(&view, &storage, 1),
                     "missing graph node label derives fallback")) {
        return 0;
    }
    if (!expect_true(strcmp(state.graph_draw_node_labels[1], "0") == 0,
                     "missing graph node label fallback is stable")) {
        return 0;
    }

    return 1;
}

static int check_action_roles(void) {
    int action_value;

    for (action_value = (int)MEM_CONSOLE_ACTION_NONE;
         action_value <= (int)MEM_CONSOLE_ACTION_REFRESH_DETAIL;
         ++action_value) {
        MemConsoleAction action = (MemConsoleAction)action_value;
        if (!expect_true(mem_console_action_role(action) != MEM_CONSOLE_ACTION_ROLE_UNKNOWN,
                         "all current actions have explicit roles")) {
            return 0;
        }
    }

    if (!expect_true(mem_console_action_role(MEM_CONSOLE_ACTION_TOGGLE_PINNED) ==
                         MEM_CONSOLE_ACTION_ROLE_ITEM_MUTATION,
                     "pinned toggle is item mutation")) {
        return 0;
    }
    if (!expect_true(mem_console_action_may_write_db(MEM_CONSOLE_ACTION_ADD_RELATIONSHIP),
                     "relationship add is DB-writing action")) {
        return 0;
    }
    if (!expect_true(!mem_console_action_may_write_db(MEM_CONSOLE_ACTION_REFRESH),
                     "refresh is not DB-writing action")) {
        return 0;
    }
    if (!expect_true(mem_console_action_routes_db_switch(MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER),
                     "confirm DB picker routes DB switch")) {
        return 0;
    }
    if (!expect_true(strcmp(mem_console_action_role_name(MEM_CONSOLE_ACTION_ROLE_GRAPH_VIEW),
                           "graph_view") == 0,
                     "action role names are stable")) {
        return 0;
    }

    return 1;
}

int main(void) {
    if (!check_left_panel_render_derivation()) {
        return 1;
    }
    if (!check_detail_render_derivation()) {
        return 1;
    }
    if (!check_graph_render_derivation()) {
        return 1;
    }
    if (!check_action_roles()) {
        return 1;
    }

    puts("state-boundary contract test ok");
    return 0;
}
