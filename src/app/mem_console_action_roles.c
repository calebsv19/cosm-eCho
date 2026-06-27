#include "mem_console_action_roles.h"

MemConsoleActionRole mem_console_action_role(MemConsoleAction action) {
    switch (action) {
        case MEM_CONSOLE_ACTION_NONE:
            return MEM_CONSOLE_ACTION_ROLE_NONE;
        case MEM_CONSOLE_ACTION_REFRESH:
        case MEM_CONSOLE_ACTION_REFRESH_DETAIL:
            return MEM_CONSOLE_ACTION_ROLE_REFRESH;
        case MEM_CONSOLE_ACTION_BEGIN_TITLE_EDIT:
        case MEM_CONSOLE_ACTION_CANCEL_TITLE_EDIT:
        case MEM_CONSOLE_ACTION_BEGIN_BODY_EDIT:
        case MEM_CONSOLE_ACTION_CANCEL_BODY_EDIT:
            return MEM_CONSOLE_ACTION_ROLE_EDIT_SESSION;
        case MEM_CONSOLE_ACTION_CREATE_FROM_SEARCH:
        case MEM_CONSOLE_ACTION_SAVE_TITLE_EDIT:
        case MEM_CONSOLE_ACTION_SAVE_BODY_EDIT:
        case MEM_CONSOLE_ACTION_TOGGLE_PINNED:
        case MEM_CONSOLE_ACTION_TOGGLE_CANONICAL:
            return MEM_CONSOLE_ACTION_ROLE_ITEM_MUTATION;
        case MEM_CONSOLE_ACTION_TOGGLE_GRAPH_MODE:
        case MEM_CONSOLE_ACTION_REFRESH_GRAPH:
        case MEM_CONSOLE_ACTION_CENTER_GRAPH:
        case MEM_CONSOLE_ACTION_CENTER_SELECTED:
            return MEM_CONSOLE_ACTION_ROLE_GRAPH_VIEW;
        case MEM_CONSOLE_ACTION_BEGIN_DB_PICKER:
        case MEM_CONSOLE_ACTION_BEGIN_DB_CREATE:
        case MEM_CONSOLE_ACTION_CANCEL_DB_PICKER:
        case MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER:
            return MEM_CONSOLE_ACTION_ROLE_DB_PATH;
        case MEM_CONSOLE_ACTION_BEGIN_INPUT_ROOT_PICKER:
        case MEM_CONSOLE_ACTION_PICK_INPUT_ROOT_FOLDER:
            return MEM_CONSOLE_ACTION_ROLE_INPUT_ROOT;
        case MEM_CONSOLE_ACTION_ADD_RELATIONSHIP:
        case MEM_CONSOLE_ACTION_CYCLE_RELATIONSHIP_KIND:
        case MEM_CONSOLE_ACTION_REMOVE_RELATIONSHIP:
            return MEM_CONSOLE_ACTION_ROLE_RELATIONSHIP_MUTATION;
        case MEM_CONSOLE_ACTION_TOGGLE_BROWSE_PINNED:
        case MEM_CONSOLE_ACTION_TOGGLE_BROWSE_CANONICAL:
        case MEM_CONSOLE_ACTION_CYCLE_BROWSE_KIND:
            return MEM_CONSOLE_ACTION_ROLE_BROWSE_FILTER;
        case MEM_CONSOLE_ACTION_OPEN_REFERENCE_PATH:
            return MEM_CONSOLE_ACTION_ROLE_EXTERNAL_REFERENCE;
    }
    return MEM_CONSOLE_ACTION_ROLE_UNKNOWN;
}

const char *mem_console_action_role_name(MemConsoleActionRole role) {
    switch (role) {
        case MEM_CONSOLE_ACTION_ROLE_NONE:
            return "none";
        case MEM_CONSOLE_ACTION_ROLE_REFRESH:
            return "refresh";
        case MEM_CONSOLE_ACTION_ROLE_EDIT_SESSION:
            return "edit_session";
        case MEM_CONSOLE_ACTION_ROLE_ITEM_MUTATION:
            return "item_mutation";
        case MEM_CONSOLE_ACTION_ROLE_GRAPH_VIEW:
            return "graph_view";
        case MEM_CONSOLE_ACTION_ROLE_DB_PATH:
            return "db_path";
        case MEM_CONSOLE_ACTION_ROLE_INPUT_ROOT:
            return "input_root";
        case MEM_CONSOLE_ACTION_ROLE_RELATIONSHIP_MUTATION:
            return "relationship_mutation";
        case MEM_CONSOLE_ACTION_ROLE_BROWSE_FILTER:
            return "browse_filter";
        case MEM_CONSOLE_ACTION_ROLE_EXTERNAL_REFERENCE:
            return "external_reference";
        case MEM_CONSOLE_ACTION_ROLE_UNKNOWN:
        default:
            return "unknown";
    }
}

int mem_console_action_may_write_db(MemConsoleAction action) {
    MemConsoleActionRole role = mem_console_action_role(action);
    return role == MEM_CONSOLE_ACTION_ROLE_ITEM_MUTATION ||
           role == MEM_CONSOLE_ACTION_ROLE_RELATIONSHIP_MUTATION;
}

int mem_console_action_routes_db_switch(MemConsoleAction action) {
    return action == MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER ? 1 : 0;
}
