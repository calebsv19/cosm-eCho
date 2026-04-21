#ifndef MEM_CONSOLE_PREFS_APP_IO_INTERNAL_H
#define MEM_CONSOLE_PREFS_APP_IO_INTERNAL_H

#include <stddef.h>

int mem_console_build_prefs_path_impl(const char *db_path, char *out_path, size_t out_cap);
int mem_console_build_app_prefs_path_impl(char *out_path, size_t out_cap);
int mem_console_build_app_prefs_path_for_output_root_impl(const char *output_root,
                                                          char *out_path,
                                                          size_t out_cap);

#endif
