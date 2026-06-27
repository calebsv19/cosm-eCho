#ifndef MEM_CONSOLE_ACTION_ROLES_H
#define MEM_CONSOLE_ACTION_ROLES_H

#include "mem_console_types.h"

typedef enum MemConsoleActionRole {
    MEM_CONSOLE_ACTION_ROLE_UNKNOWN = 0,
    MEM_CONSOLE_ACTION_ROLE_NONE = 1,
    MEM_CONSOLE_ACTION_ROLE_REFRESH = 2,
    MEM_CONSOLE_ACTION_ROLE_EDIT_SESSION = 3,
    MEM_CONSOLE_ACTION_ROLE_ITEM_MUTATION = 4,
    MEM_CONSOLE_ACTION_ROLE_GRAPH_VIEW = 5,
    MEM_CONSOLE_ACTION_ROLE_DB_PATH = 6,
    MEM_CONSOLE_ACTION_ROLE_INPUT_ROOT = 7,
    MEM_CONSOLE_ACTION_ROLE_RELATIONSHIP_MUTATION = 8,
    MEM_CONSOLE_ACTION_ROLE_BROWSE_FILTER = 9,
    MEM_CONSOLE_ACTION_ROLE_EXTERNAL_REFERENCE = 10
} MemConsoleActionRole;

MemConsoleActionRole mem_console_action_role(MemConsoleAction action);
const char *mem_console_action_role_name(MemConsoleActionRole role);
int mem_console_action_may_write_db(MemConsoleAction action);
int mem_console_action_routes_db_switch(MemConsoleAction action);

#endif
