#include "mem_console_app_internal.h"

#include <stdio.h>

static void mark_search_input_changed(MemConsoleState *state);
static void append_graph_edge_limit_digits(MemConsoleState *state, const char *text);
static void commit_graph_edge_limit_input(MemConsoleState *state,
                                          MemConsoleAction *keyboard_action);

static void mark_search_input_changed(MemConsoleState *state) {
    if (!state) {
        return;
    }
    state->search_refresh_pending = 1;
    state->search_last_input_ms = SDL_GetTicks64();
    state->list_scroll = 0.0f;
    state->list_query_offset = 0;
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_INPUT | MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

static void append_graph_edge_limit_digits(MemConsoleState *state, const char *text) {
    int i;
    char digit_text[2];

    if (!state || !text) {
        return;
    }

    digit_text[1] = '\0';
    for (i = 0; text[i] != '\0'; ++i) {
        if (text[i] >= '0' && text[i] <= '9') {
            digit_text[0] = text[i];
            append_active_input_text(state, digit_text);
        }
    }
}

static void commit_graph_edge_limit_input(MemConsoleState *state,
                                          MemConsoleAction *keyboard_action) {
    int parsed_limit;
    int changed;

    if (!state || !keyboard_action) {
        return;
    }

    parsed_limit = mem_console_graph_edge_limit_parse(state->graph_edge_limit_text,
                                                      state->graph_query_edge_limit);
    changed = parsed_limit != state->graph_query_edge_limit;
    mem_console_graph_edge_limit_set(state, parsed_limit);

    *keyboard_action = MEM_CONSOLE_ACTION_REFRESH_GRAPH;
    if (changed) {
        (void)snprintf(state->status_line,
                       sizeof(state->status_line),
                       "Graph edge limit set to %d.",
                       parsed_limit);
    }
    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
}

void mem_console_app_process_sdl_event(const SDL_Event *event,
                                       bool *running,
                                       KitRenderContext *render_ctx,
                                       KitUiContext *ui_ctx,
                                       MemConsoleState *state,
                                       const char *prefs_path,
                                       KitUiInputState *input,
                                       int *wheel_y,
                                       MemConsoleAction *keyboard_action) {
    if (!event || !running || !render_ctx || !ui_ctx || !state || !input || !wheel_y || !keyboard_action) {
        return;
    }

    switch (event->type) {
        case SDL_QUIT:
            *running = false;
            break;
        case SDL_KEYDOWN:
            {
                Uint16 mod = event->key.keysym.mod;
                int ctrl_or_cmd = (mod & (KMOD_CTRL | KMOD_GUI)) != 0;
                int shift = (mod & KMOD_SHIFT) != 0;
                int handled_shortcut = 0;

                mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_INPUT);
                if (ctrl_or_cmd) {
                    handled_shortcut = mem_console_app_handle_text_zoom_shortcut(render_ctx,
                                                                                  ui_ctx,
                                                                                  state,
                                                                                  prefs_path,
                                                                                  event->key.keysym.sym);
                }
                if (!handled_shortcut && ctrl_or_cmd && shift) {
                    handled_shortcut = mem_console_app_handle_theme_shortcut(render_ctx,
                                                                             ui_ctx,
                                                                             state,
                                                                             prefs_path,
                                                                             event->key.keysym.sym);
                    if (!handled_shortcut) {
                        handled_shortcut = mem_console_app_handle_font_shortcut(render_ctx,
                                                                                ui_ctx,
                                                                                state,
                                                                                prefs_path,
                                                                                event->key.keysym.sym);
                    }
                }

                if (handled_shortcut) {
                    break;
                }
                if (ctrl_or_cmd && event->key.keysym.sym == SDLK_l) {
                    state->graph_edge_labels_enabled = state->graph_edge_labels_enabled ? 0 : 1;
                    (void)snprintf(state->status_line,
                                   sizeof(state->status_line),
                                   "Edge labels %s.",
                                   state->graph_edge_labels_enabled ? "enabled" : "disabled");
                    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                    break;
                }
                if (!state->db_modal_open && ctrl_or_cmd && event->key.keysym.sym == SDLK_i) {
                    *keyboard_action = MEM_CONSOLE_ACTION_BEGIN_INPUT_ROOT_PICKER;
                    break;
                }
                if (!state->db_modal_open && ctrl_or_cmd && event->key.keysym.sym == SDLK_b) {
                    *keyboard_action = MEM_CONSOLE_ACTION_PICK_INPUT_ROOT_FOLDER;
                    break;
                }

                if (event->key.keysym.sym == SDLK_ESCAPE) {
                    if (state->db_modal_open) {
                        *keyboard_action = MEM_CONSOLE_ACTION_CANCEL_DB_PICKER;
                    } else if (state->title_edit_mode) {
                        *keyboard_action = MEM_CONSOLE_ACTION_CANCEL_TITLE_EDIT;
                    } else if (state->body_edit_mode) {
                        *keyboard_action = MEM_CONSOLE_ACTION_CANCEL_BODY_EDIT;
                    } else {
                        *running = false;
                    }
                    break;
                }
                if (ctrl_or_cmd && event->key.keysym.sym == SDLK_v) {
                    char *clipboard_text = SDL_GetClipboardText();
                    if (clipboard_text && clipboard_text[0] != '\0') {
                        if (state->input_target == MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT) {
                            append_graph_edge_limit_digits(state, clipboard_text);
                        } else {
                            append_active_input_text(state, clipboard_text);
                        }
                        mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                        if (active_input_is_search(state)) {
                            mark_search_input_changed(state);
                        }
                    }
                    if (clipboard_text) {
                        SDL_free(clipboard_text);
                    }
                    break;
                }
                if (event->key.keysym.sym == SDLK_BACKSPACE) {
                    erase_active_input_char(state);
                    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                    if (active_input_is_search(state)) {
                        mark_search_input_changed(state);
                    }
                    break;
                }
                if (event->key.keysym.sym == SDLK_DELETE) {
                    delete_active_input_char(state);
                    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                    if (active_input_is_search(state)) {
                        mark_search_input_changed(state);
                    }
                    break;
                }
                if (event->key.keysym.sym == SDLK_LEFT) {
                    move_active_input_cursor(state, -1);
                    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                    break;
                }
                if (event->key.keysym.sym == SDLK_RIGHT) {
                    move_active_input_cursor(state, 1);
                    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                    break;
                }
                if (event->key.keysym.sym == SDLK_HOME) {
                    move_active_input_cursor_home(state);
                    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                    break;
                }
                if (event->key.keysym.sym == SDLK_END) {
                    move_active_input_cursor_end(state);
                    mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                    break;
                }
                if (event->key.keysym.sym == SDLK_RETURN ||
                    event->key.keysym.sym == SDLK_KP_ENTER) {
                    if (state->input_target == MEM_CONSOLE_INPUT_TITLE_EDIT) {
                        *keyboard_action = MEM_CONSOLE_ACTION_SAVE_TITLE_EDIT;
                    } else if (state->input_target == MEM_CONSOLE_INPUT_BODY_EDIT) {
                        if (ctrl_or_cmd && !shift) {
                            *keyboard_action = MEM_CONSOLE_ACTION_SAVE_BODY_EDIT;
                        } else {
                            append_active_input_text(state, "\n");
                            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_CONTENT);
                        }
                    } else if (state->input_target == MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT) {
                        commit_graph_edge_limit_input(state, keyboard_action);
                    } else if (state->input_target == MEM_CONSOLE_INPUT_DB_PATH) {
                        *keyboard_action = MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER;
                    } else {
                        state->search_refresh_pending = 0;
                        *keyboard_action = MEM_CONSOLE_ACTION_REFRESH;
                    }
                }
            }
            break;
        case SDL_TEXTINPUT:
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_INPUT | MEM_CONSOLE_REDRAW_REASON_CONTENT);
            if (state->input_target == MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT) {
                append_graph_edge_limit_digits(state, event->text.text);
            } else {
                append_active_input_text(state, event->text.text);
            }
            if (active_input_is_search(state)) {
                mark_search_input_changed(state);
            }
            break;
        case SDL_MOUSEMOTION:
            input->mouse_x = (float)event->motion.x;
            input->mouse_y = (float)event->motion.y;
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_INPUT);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                input->mouse_down = 1;
                input->mouse_pressed = 1;
                input->mouse_x = (float)event->button.x;
                input->mouse_y = (float)event->button.y;
                mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_INPUT);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                input->mouse_down = 0;
                input->mouse_released = 1;
                input->mouse_x = (float)event->button.x;
                input->mouse_y = (float)event->button.y;
                mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_INPUT);
            }
            break;
        case SDL_MOUSEWHEEL:
            *wheel_y = event->wheel.y;
            mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_INPUT);
            break;
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                event->window.event == SDL_WINDOWEVENT_RESIZED) {
                mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_LAYOUT);
            } else if (event->window.event == SDL_WINDOWEVENT_EXPOSED ||
                       event->window.event == SDL_WINDOWEVENT_SHOWN ||
                       event->window.event == SDL_WINDOWEVENT_RESTORED) {
                mem_console_redraw_mark(state, MEM_CONSOLE_REDRAW_REASON_BACKGROUND);
            }
            break;
        default:
            break;
    }
}
