#include "mem_console_ui_surface.h"
#include "kit_ui_text_edit_sdl.h"
#include "mem_console_state.h"
#include <string.h>
#include <stdio.h>
static void buffer(MemConsoleState *s,char **text,size_t *cap,int **cursor) {
#define FIELD(name, pos) do { *text=s->name;*cap=sizeof(s->name);*cursor=&s->pos; } while(0)
    switch(s->input_target) {
        case MEM_CONSOLE_INPUT_TITLE_EDIT:FIELD(title_edit_text,title_edit_cursor);break;
        case MEM_CONSOLE_INPUT_BODY_EDIT:FIELD(body_edit_text,body_edit_cursor);break;
        case MEM_CONSOLE_INPUT_DB_PATH:FIELD(db_modal_text,db_modal_cursor);break;
        case MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT:FIELD(graph_edge_limit_text,graph_edge_limit_cursor);break;
        case MEM_CONSOLE_INPUT_RELATIONSHIP_TARGET:FIELD(relationship_target_text,relationship_target_cursor);break;
        default:FIELD(search_text,search_cursor);break;
    }
#undef FIELD
}
int mem_console_ui_text_event(MemConsoleState *s,const SDL_Event *event,MemConsoleAction *action) {
    if(!s||!event||!action)return 0;
    s->text_edit_changed=0;
    if(event->type==SDL_QUIT || (event->type==SDL_WINDOWEVENT &&
       (event->window.event==SDL_WINDOWEVENT_FOCUS_LOST || event->window.event==SDL_WINDOWEVENT_HIDDEN))) {
        kit_ui_text_cancel_composition(&s->text_edit);kit_ui_focus_scope_cancel(&s->focus_scope,&s->button_surface);s->button_keyboard_text=0;return 0;
    }
    if(mem_console_workspace_authoring_host_active(&s->workspace_authoring)) {
        kit_ui_text_cancel_composition(&s->text_edit);return event->type==SDL_TEXTINPUT || event->type==SDL_TEXTEDITING;
    }
    if(s->db_modal_open && s->input_target!=MEM_CONSOLE_INPUT_DB_PATH) {
        kit_ui_text_cancel_composition(&s->text_edit);return event->type==SDL_TEXTINPUT || event->type==SDL_TEXTEDITING;
    }
    if(event->type==SDL_KEYDOWN && event->key.keysym.sym==SDLK_TAB) {
        s->button_keyboard_text=0;kit_ui_text_cancel_composition(&s->text_edit);return 0;
    }
    if(!s->button_keyboard_text && s->button_surface.interaction.focused_id)
        return event->type==SDL_TEXTINPUT || event->type==SDL_TEXTEDITING;
    if(event->type!=SDL_TEXTINPUT && event->type!=SDL_TEXTEDITING && event->type!=SDL_KEYDOWN)return 0;
    char *text=NULL;size_t cap=0;int *cursor=NULL;buffer(s,&text,&cap,&cursor);
    int changed_owner=s->text_edit.text!=text;
    unsigned flags=s->input_target==MEM_CONSOLE_INPUT_BODY_EDIT?0:KIT_UI_TEXT_SINGLE_LINE;
    if(s->input_target==MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT || s->input_target==MEM_CONSOLE_INPUT_RELATIONSHIP_TARGET)flags|=KIT_UI_TEXT_DIGITS;
    if(kit_ui_text_bind(&s->text_edit,text,cap,flags).code!=CORE_OK)return 1;
    if(changed_owner || (int)s->text_edit.cursor!=*cursor || (s->input_target==MEM_CONSOLE_INPUT_DB_PATH && (int)s->text_edit.anchor!=s->db_modal_selection_anchor)) {
        int anchor=s->input_target==MEM_CONSOLE_INPUT_DB_PATH?s->db_modal_selection_anchor:*cursor;
        (void)kit_ui_text_position(&s->text_edit,*cursor<0?0:(size_t)*cursor,anchor<0?0:(size_t)anchor);
    }
    KitUiTextEventResult result=kit_ui_text_event_sdl(&s->text_edit,event);
    *cursor=(int)s->text_edit.cursor;s->text_edit_target=s->input_target;
    if(s->input_target==MEM_CONSOLE_INPUT_DB_PATH) {
        s->db_modal_selection_anchor=(int)s->text_edit.anchor;
        s->db_modal_selection_start=(int)(s->text_edit.cursor<s->text_edit.anchor?s->text_edit.cursor:s->text_edit.anchor);
        s->db_modal_selection_end=(int)(s->text_edit.cursor>s->text_edit.anchor?s->text_edit.cursor:s->text_edit.anchor);
    }
    if(result.status.code!=CORE_OK)snprintf(s->status_line,sizeof(s->status_line),"%s",result.status.message);
    if(result.cancel || result.submit) {
        if(s->input_target==MEM_CONSOLE_INPUT_TITLE_EDIT)*action=result.cancel?MEM_CONSOLE_ACTION_CANCEL_TITLE_EDIT:MEM_CONSOLE_ACTION_SAVE_TITLE_EDIT;
        else if(s->input_target==MEM_CONSOLE_INPUT_BODY_EDIT)*action=result.cancel?MEM_CONSOLE_ACTION_CANCEL_BODY_EDIT:MEM_CONSOLE_ACTION_SAVE_BODY_EDIT;
        else if(s->input_target==MEM_CONSOLE_INPUT_DB_PATH)*action=result.cancel?MEM_CONSOLE_ACTION_CANCEL_DB_PICKER:MEM_CONSOLE_ACTION_CONFIRM_DB_PICKER;
        else if(result.submit && s->input_target==MEM_CONSOLE_INPUT_RELATIONSHIP_TARGET)*action=MEM_CONSOLE_ACTION_ADD_RELATIONSHIP;
        else if(result.submit && s->input_target==MEM_CONSOLE_INPUT_SEARCH)*action=MEM_CONSOLE_ACTION_REFRESH;
        else if(result.submit && s->input_target==MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT) {
            mem_console_graph_edge_limit_set(s,mem_console_graph_edge_limit_parse(s->graph_edge_limit_text,s->graph_query_edge_limit));*action=MEM_CONSOLE_ACTION_REFRESH_GRAPH;
        } else if(result.cancel)return 0; /* preserve ordinary runtime Escape policy */
    }
    if(event->type==SDL_KEYDOWN && !(event->key.keysym.mod&(KMOD_CTRL|KMOD_GUI|KMOD_ALT)) && event->key.keysym.sym>=32 && event->key.keysym.sym<127)result.consumed=1;
    if(event->type==SDL_KEYDOWN && (event->key.keysym.mod&(KMOD_CTRL|KMOD_GUI)))result.consumed=1;
    s->text_edit_changed=result.changed;
    if(result.consumed)mem_console_ui_surface_text_focus(s);
    return result.consumed;
}
