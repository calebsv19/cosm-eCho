#include "mem_console_state.h"

#include <stdio.h>
#include <string.h>

int mem_console_project_filter_is_selected(const MemConsoleState *state, const char *project_key) {
    int i;

    if (!state || !project_key || project_key[0] == '\0') {
        return 0;
    }

    for (i = 0; i < state->selected_project_count; ++i) {
        if (strcmp(state->selected_project_keys[i], project_key) == 0) {
            return 1;
        }
    }
    return 0;
}

void mem_console_project_filter_clear(MemConsoleState *state) {
    int i;

    if (!state) {
        return;
    }
    for (i = 0; i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        state->selected_project_keys[i][0] = '\0';
    }
    state->selected_project_count = 0;
}

int mem_console_project_filter_toggle(MemConsoleState *state, const char *project_key) {
    int i;

    if (!state || !project_key || project_key[0] == '\0') {
        return 0;
    }

    for (i = 0; i < state->selected_project_count; ++i) {
        if (strcmp(state->selected_project_keys[i], project_key) == 0) {
            int j;
            for (j = i; j < (state->selected_project_count - 1); ++j) {
                (void)snprintf(state->selected_project_keys[j],
                               sizeof(state->selected_project_keys[j]),
                               "%s",
                               state->selected_project_keys[j + 1]);
            }
            state->selected_project_keys[state->selected_project_count - 1][0] = '\0';
            state->selected_project_count -= 1;
            return 1;
        }
    }

    if (state->selected_project_count >= MEM_CONSOLE_SCOPE_FILTER_LIMIT) {
        return 0;
    }
    (void)snprintf(state->selected_project_keys[state->selected_project_count],
                   sizeof(state->selected_project_keys[state->selected_project_count]),
                   "%s",
                   project_key);
    state->selected_project_count += 1;
    return 1;
}

void mem_console_project_filter_prune_to_options(MemConsoleState *state) {
    int write_index = 0;
    int i;

    if (!state) {
        return;
    }

    for (i = 0; i < state->selected_project_count; ++i) {
        int option_index;
        int found = 0;
        const char *selected_key = state->selected_project_keys[i];

        if (!selected_key[0]) {
            continue;
        }
        for (option_index = 0; option_index < state->project_filter_option_count; ++option_index) {
            if (strcmp(selected_key, state->project_filter_keys[option_index]) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            continue;
        }
        if (write_index != i) {
            (void)snprintf(state->selected_project_keys[write_index],
                           sizeof(state->selected_project_keys[write_index]),
                           "%s",
                           selected_key);
        }
        write_index += 1;
    }

    for (i = write_index; i < MEM_CONSOLE_SCOPE_FILTER_LIMIT; ++i) {
        state->selected_project_keys[i][0] = '\0';
    }
    state->selected_project_count = write_index;
}

int selected_id_in_visible_items(const MemConsoleState *state) {
    int i;

    if (!state || state->selected_item_id == 0) {
        return 0;
    }

    for (i = 0; i < state->visible_count; ++i) {
        if (state->visible_items[i].id == state->selected_item_id) {
            return 1;
        }
    }

    return 0;
}
