#ifndef MEM_CONSOLE_PREFS_H
#define MEM_CONSOLE_PREFS_H

#include <stddef.h>
#include <stdint.h>

#include "mem_console_state.h"

int mem_console_build_prefs_path(const char *db_path, char *out_path, size_t out_cap);
int mem_console_build_app_prefs_path(char *out_path, size_t out_cap);
int mem_console_build_app_prefs_path_for_output_root(const char *output_root,
                                                     char *out_path,
                                                     size_t out_cap);
CoreResult mem_console_prefs_load(const char *prefs_path, MemConsoleState *state);
CoreResult mem_console_prefs_save(const char *prefs_path, const MemConsoleState *state);
CoreResult mem_console_app_prefs_load(const char *prefs_path,
                                      char *db_path,
                                      size_t db_path_cap,
                                      char *input_root,
                                      size_t input_root_cap,
                                      char *output_root,
                                      size_t output_root_cap,
                                      char *active_db_path,
                                      size_t active_db_path_cap);
CoreResult mem_console_app_prefs_save(const char *prefs_path,
                                      const char *db_path,
                                      const char *input_root,
                                      const char *output_root,
                                      const char *active_db_path);
uint64_t mem_console_prefs_state_signature(const MemConsoleState *state);

#endif
