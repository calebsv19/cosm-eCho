#ifndef MEM_CONSOLE_DB_INTERNAL_H
#define MEM_CONSOLE_DB_INTERNAL_H

#include "mem_console_db.h"

CoreResult bind_project_filters(CoreMemStmt *stmt,
                                int start_index,
                                const MemConsoleState *state);
CoreResult read_project_filter_options(CoreMemDb *db, MemConsoleState *state);
CoreResult read_schema_version(CoreMemDb *db, MemConsoleState *state);
CoreResult read_active_count(CoreMemDb *db, MemConsoleState *state);
CoreResult read_matching_count(CoreMemDb *db, MemConsoleState *state);
CoreResult read_visible_items(CoreMemDb *db, MemConsoleState *state);
void clamp_list_query_offset(MemConsoleState *state);
CoreResult read_selected_detail(CoreMemDb *db, MemConsoleState *state);
CoreResult read_selected_relationships(CoreMemDb *db, MemConsoleState *state);

#endif
