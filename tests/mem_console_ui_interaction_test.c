#include "mem_console_workspace_authoring.h"
#include "mem_console_ui_surface.h"
#include "mem_console_ui.h"
#include "mem_console_ui_common.h"
#include "app/mem_console_app_internal.h"
#include "mem_console_ui_pane_composition.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static KitUiInputState input;
static bool running=true;
static int wheel;
static MemConsoleAction keyboard;
static void route(MemConsoleState *s, KitRenderContext *r, KitUiContext *ui, SDL_Event *e) {
    mem_console_app_process_sdl_event(e,&running,r,ui,s,"",&input,&wheel,&keyboard);
}
static KitUiInteractionControl control(MemConsoleState *s, uint32_t domain, uint64_t key) {
    for(uint32_t i=0;i<s->button_surface.count;++i)
        if(s->button_surface.keys[i].domain==domain && s->button_surface.keys[i].value==key)
            return s->button_surface.controls[i];
    assert(!"expected visible control missing");
    return (KitUiInteractionControl){0};
}
static void pointer(MemConsoleState *s, KitRenderContext *r, KitUiContext *ui, KitRenderRect rect, Uint32 type) {
    SDL_Event e={0}; e.type=type; e.button.button=SDL_BUTTON_LEFT;
    e.button.x=(int)(rect.x+rect.width/2); e.button.y=(int)(rect.y+rect.height/2);
    route(s,r,ui,&e);
}
static MemConsoleAction frame(MemConsoleState *s, KitRenderContext *r, KitUiContext *ui) {
    MemConsoleAction action;
    assert(run_frame(r,ui,s,&input,1440,1000,0,&action)==MEM_CONSOLE_FRAME_OK);
    input.mouse_pressed=0; input.mouse_released=0;
    return action;
}
static KitUiSurfaceKey focused(MemConsoleState *s) {
    KitUiSurfaceKey key;
    assert(kit_ui_surface_key(&s->button_surface,s->button_surface.interaction.focused_id,&key));
    return key;
}
int main(void) {
    static MemConsoleState state;
    KitRenderContext render; KitUiContext ui;
    assert(kit_render_context_init(&render,KIT_RENDER_BACKEND_NULL,CORE_THEME_PRESET_DAW_DEFAULT,CORE_FONT_PRESET_DAW_DEFAULT).code==CORE_OK);
    assert(kit_ui_context_init(&ui,&render).code==CORE_OK);
    seed_state(&state,"fixture.sqlite");
    state.workspace_authoring.active=1;
    state.workspace_authoring.overlay_mode=MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_FONT_THEME;
    mem_console_workspace_authoring_host_set_viewport(&state.workspace_authoring,1440,1000);
    frame(&state,&render,&ui);
    KitRenderRect inc=control(&state,MC_BUTTON_FONT,KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_INC).bounds;
    int step=state.text_zoom_step;
    pointer(&state,&render,&ui,inc,SDL_MOUSEBUTTONUP); assert(state.text_zoom_step==step);
    pointer(&state,&render,&ui,inc,SDL_MOUSEBUTTONDOWN); assert(state.text_zoom_step==step);
    assert(focused(&state).domain==MC_BUTTON_FONT && focused(&state).value==2);
    pointer(&state,&render,&ui,inc,SDL_MOUSEBUTTONUP); assert(state.text_zoom_step>step);
    step=state.text_zoom_step;
    SDL_Event e={0}; e.type=SDL_KEYDOWN; e.key.keysym.sym=SDLK_SPACE;
    route(&state,&render,&ui,&e); assert(state.text_zoom_step==step);
    e.key.repeat=1; route(&state,&render,&ui,&e);
    e.type=SDL_KEYUP; e.key.repeat=0; route(&state,&render,&ui,&e);
    assert(state.text_zoom_step>step && state.workspace_authoring.font_theme_button_click_count==2);
    kit_ui_interaction_reset(&state.button_surface.interaction);
    e.type=SDL_KEYDOWN; e.key.keysym.sym=SDLK_TAB;
    route(&state,&render,&ui,&e); assert(focused(&state).domain==MC_BUTTON_OVERLAY);
    /* Top-level authoring mode changes on release and shares the same focus owner. */
    KitRenderRect mode=control(&state,MC_BUTTON_OVERLAY,KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_MODE).bounds;
    pointer(&state,&render,&ui,mode,SDL_MOUSEBUTTONDOWN);
    assert(state.workspace_authoring.overlay_mode==MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_FONT_THEME);
    pointer(&state,&render,&ui,mode,SDL_MOUSEBUTTONUP);
    assert(state.workspace_authoring.overlay_mode==MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES);
    state.workspace_authoring.active=0;
    frame(&state,&render,&ui);
    /* Seed real draw models so item/project/relationship surfaces are included
     * in the same production frame used by the button event replay. */
    state.selected_item_id=42; state.visible_count=1; state.visible_items[0].id=42;
    snprintf(state.visible_items[0].title,sizeof(state.visible_items[0].title),"Fixture item");
    state.project_filter_option_count=1;
    snprintf(state.project_filter_keys[0],sizeof(state.project_filter_keys[0]),"fixture-project");
    state.project_filter_counts[0]=1;
    snprintf(state.project_filter_labels[0],sizeof(state.project_filter_labels[0]),"Fixture project");
    state.detail_relationship_count=1; state.detail_relationships[0].link_id=1001;
    state.detail_relationships[0].neighbor_item_id=43; state.detail_relationships[0].outgoing=1;
    snprintf(state.detail_relationships[0].kind,sizeof(state.detail_relationships[0].kind),"related");
    snprintf(state.detail_relationships[0].neighbor_title,sizeof(state.detail_relationships[0].neighbor_title),"Fixture neighbor");
    snprintf(state.relationship_target_text,sizeof(state.relationship_target_text),"43");
    state.graph_node_count=2; state.graph_edge_count=1;
    for(int i=0;i<2;++i) {
        state.graph_nodes[i].item_id=42+i;
        snprintf(state.graph_nodes[i].title,sizeof(state.graph_nodes[i].title),"Fixture graph %d",i);
        snprintf(state.graph_nodes[i].kind,sizeof(state.graph_nodes[i].kind),"plan");
        snprintf(state.graph_nodes[i].project_key,sizeof(state.graph_nodes[i].project_key),"fixture-project");
    }
    state.graph_edges[0].from_index=0; state.graph_edges[0].to_index=1;
    snprintf(state.graph_edges[0].kind,sizeof(state.graph_edges[0].kind),"related");
    frame(&state,&render,&ui);
    control(&state,MC_BUTTON_ITEM,42);
    control(&state,MC_BUTTON_PROJECT,mem_console_ui_surface_string_key("fixture-project"));
    KitRenderRect legend=control(&state,MC_BUTTON_LEGEND,mem_console_ui_surface_string_key("ALL")).bounds;
    pointer(&state,&render,&ui,legend,SDL_MOUSEBUTTONDOWN);
    frame(&state,&render,&ui);
    pointer(&state,&render,&ui,legend,SDL_MOUSEBUTTONUP);
    frame(&state,&render,&ui);
    KitRenderRect add=control(&state,MC_BUTTON_REL_ADD,1).bounds;
    pointer(&state,&render,&ui,add,SDL_MOUSEBUTTONDOWN);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE);
    pointer(&state,&render,&ui,add,SDL_MOUSEBUTTONUP);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_ADD_RELATIONSHIP);
    const uint32_t rel_domains[]={MC_BUTTON_REL_KIND,MC_BUTTON_REL_DELETE};
    const MemConsoleAction rel_actions[]={MEM_CONSOLE_ACTION_CYCLE_RELATIONSHIP_KIND,MEM_CONSOLE_ACTION_REMOVE_RELATIONSHIP};
    for(int i=0;i<2;++i) {
        KitRenderRect row=control(&state,rel_domains[i],1001).bounds;
        pointer(&state,&render,&ui,row,SDL_MOUSEBUTTONDOWN);
        assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE);
        pointer(&state,&render,&ui,row,SDL_MOUSEBUTTONUP);
        assert(frame(&state,&render,&ui)==rel_actions[i] && state.relationship_action_link_id==1001);
    }
    KitRenderRect labels=control(&state,MC_BUTTON_GRAPH_SETTING,1).bounds;
    int enabled=state.graph_edge_labels_enabled;
    pointer(&state,&render,&ui,labels,SDL_MOUSEBUTTONUP); frame(&state,&render,&ui);
    assert(state.graph_edge_labels_enabled==enabled);
    pointer(&state,&render,&ui,labels,SDL_MOUSEBUTTONDOWN); frame(&state,&render,&ui);
    assert(state.graph_edge_labels_enabled==enabled);
    pointer(&state,&render,&ui,labels,SDL_MOUSEBUTTONUP); frame(&state,&render,&ui);
    assert(state.graph_edge_labels_enabled!=enabled);
    enabled=state.graph_edge_labels_enabled;
    /* Two activations survive one input drain and execute in consecutive frames. */
    for(int i=0;i<2;++i) { pointer(&state,&render,&ui,labels,SDL_MOUSEBUTTONDOWN); pointer(&state,&render,&ui,labels,SDL_MOUSEBUTTONUP); }
    frame(&state,&render,&ui); assert(state.graph_edge_labels_enabled!=enabled && kit_ui_surface_pending(&state.button_surface));
    frame(&state,&render,&ui); assert(state.graph_edge_labels_enabled==enabled && !kit_ui_surface_pending(&state.button_surface));
    KitRenderRect root=control(&state,MC_BUTTON_LEFT,1103).bounds;
    pointer(&state,&render,&ui,root,SDL_MOUSEBUTTONDOWN);
    state.db_modal_open=1; state.db_modal_create_mode=0;
    state.db_picker_entry_count=1;
    snprintf(state.db_picker_entry_names[0],sizeof(state.db_picker_entry_names[0]),"fixture.sqlite");
    snprintf(state.db_picker_entry_paths[0],sizeof(state.db_picker_entry_paths[0]),"fixture.sqlite");
    pointer(&state,&render,&ui,root,SDL_MOUSEBUTTONUP);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE);
    for(uint32_t i=0;i<state.button_surface.count;++i)
        assert(state.button_surface.keys[i].domain==MC_BUTTON_DB_ROW || state.button_surface.keys[i].domain==MC_BUTTON_DB_ACTION);
    KitRenderRect dbrow=control(&state,MC_BUTTON_DB_ROW,mem_console_ui_surface_string_key("fixture.sqlite")).bounds;
    pointer(&state,&render,&ui,dbrow,SDL_MOUSEBUTTONDOWN);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE && !state.db_modal_text[0]);
    pointer(&state,&render,&ui,dbrow,SDL_MOUSEBUTTONUP);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE && strcmp(state.db_modal_text,"fixture.sqlite")==0);
    KitRenderRect cancel=control(&state,MC_BUTTON_DB_ACTION,4101).bounds;
    pointer(&state,&render,&ui,cancel,SDL_MOUSEBUTTONDOWN);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE);
    pointer(&state,&render,&ui,cancel,SDL_MOUSEBUTTONUP);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_CANCEL_DB_PICKER);
    state.db_modal_open=0; frame(&state,&render,&ui);
    assert(focused(&state).domain==MC_BUTTON_LEFT && focused(&state).value==1103);
    puts("Echo actual DB modal restores enabled semantic button identity");
    root=control(&state,MC_BUTTON_LEFT,1103).bounds;
    pointer(&state,&render,&ui,root,SDL_MOUSEBUTTONDOWN);
    pointer(&state,&render,&ui,(KitRenderRect){0,0,1,1},SDL_MOUSEBUTTONUP);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE);
    strcpy(state.search_text,"fixture");state.search_cursor=7;
    state.button_keyboard_text=0;e=(SDL_Event){0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_BACKSPACE;
    route(&state,&render,&ui,&e);assert(strcmp(state.search_text,"fixture")==0);
    /* Text focus passes plain activation keys to the existing editor owner. */
    mem_console_ui_surface_text_focus(&state);
    e.type=SDL_KEYDOWN; e.key.keysym.sym=SDLK_SPACE;
    assert(!mem_console_ui_surface_event(&state,&render,&ui,&e,&input));
    state.input_target=MEM_CONSOLE_INPUT_SEARCH;state.button_keyboard_text=1;
    strcpy(state.search_text,"aé😀z");state.search_cursor=8;memset(&state.text_edit,0,sizeof(state.text_edit));
    e=(SDL_Event){0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_LEFT;route(&state,&render,&ui,&e);
    e.key.keysym.sym=SDLK_BACKSPACE;route(&state,&render,&ui,&e);assert(strcmp(state.search_text,"aéz")==0);
    e=(SDL_Event){0};e.type=SDL_TEXTEDITING;strcpy(e.edit.text,"候補");e.edit.length=2;route(&state,&render,&ui,&e);
    assert(strcmp(state.search_text,"aéz")==0 && state.text_edit.composition[0] && !state.text_edit_changed);
    e=(SDL_Event){0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_RETURN;keyboard=MEM_CONSOLE_ACTION_NONE;route(&state,&render,&ui,&e);
    assert(keyboard==MEM_CONSOLE_ACTION_NONE);
    e=(SDL_Event){0};e.type=SDL_TEXTINPUT;strcpy(e.text.text,"中");route(&state,&render,&ui,&e);assert(strcmp(state.search_text,"aé中z")==0);
    state.input_target=MEM_CONSOLE_INPUT_BODY_EDIT;state.body_edit_mode=1;state.body_edit_text[0]=0;state.body_edit_cursor=0;
    e=(SDL_Event){0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_RETURN;route(&state,&render,&ui,&e);assert(strcmp(state.body_edit_text,"\n")==0);
    e.key.keysym.mod=KMOD_CTRL;keyboard=MEM_CONSOLE_ACTION_NONE;route(&state,&render,&ui,&e);assert(keyboard==MEM_CONSOLE_ACTION_SAVE_BODY_EDIT);
    state.body_edit_mode=0;state.input_target=MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT;strcpy(state.graph_edge_limit_text,"128");state.graph_edge_limit_cursor=3;
    e=(SDL_Event){0};e.type=SDL_TEXTINPUT;strcpy(e.text.text,"9x");route(&state,&render,&ui,&e);assert(strcmp(state.graph_edge_limit_text,"128")==0);
    puts("Echo text adapter: UTF8, preedit, multiline host save intent and transactional numeric refusal pass");
    memset(&e,0,sizeof(e)); e.type=SDL_WINDOWEVENT; e.window.event=SDL_WINDOWEVENT_FOCUS_LOST;
    state.pane_pointer_owner=(KitPanePointerOwner){1,1};
    route(&state,&render,&ui,&e); assert(!state.button_surface.interaction.focused_id&&!state.pane_pointer_owner.down&&!state.pane_pointer_owner.captured_id);
    e.type=SDL_QUIT; route(&state,&render,&ui,&e); assert(!running);
    puts("eCho surfaces: production event/frame routes, preview/top controls, HUD release actions, FIFO, modal exclusion, editor ownership and quit pass");
    char *fields[]={state.search_text,state.title_edit_text,state.body_edit_text,state.graph_edge_limit_text,state.db_modal_text,state.relationship_target_text};
    size_t caps[]={sizeof(state.search_text),sizeof(state.title_edit_text),sizeof(state.body_edit_text),sizeof(state.graph_edge_limit_text),sizeof(state.db_modal_text),sizeof(state.relationship_target_text)};
    KitRenderCommand commands[256];KitRenderCommandBuffer queue={commands,256,0};KitRenderFrame presented;
    assert(kit_render_begin_frame(&render,1440,1000,&queue,&presented).code==CORE_OK);
    kit_ui_interaction_reset(&state.button_surface.interaction);state.button_keyboard_text=1;
    for(int target=0;target<6;++target) {
        strcpy(fields[target],"aéWz");state.text_edit=(KitUiTextEdit){0};state.input_target=(MemConsoleInputTarget)target;
        assert(kit_ui_text_bind(&state.text_edit,fields[target],caps[target],0).code==CORE_OK);
        assert(kit_ui_text_position(&state.text_edit,3,1).code==CORE_OK);
        assert(kit_ui_text_compose(&state.text_edit,"中W",1,1).code==CORE_OK);state.db_modal_selection_anchor=1;
        assert(mem_console_ui_draw_editable_line(&ui,&render,&presented,&state,(KitRenderRect){20,20+target*50,300,40},fields[target],CORE_THEME_COLOR_TEXT_PRIMARY,CORE_FONT_ROLE_UI_REGULAR,CORE_FONT_TEXT_SIZE_BASIC,1,3).code==CORE_OK);
        const KitUiTextPresentation *v=mem_console_ui_text_view((MemConsoleInputTarget)target);
        assert(!strcmp(v->display,"a中WWz") && v->rows[0].preedit.width>0 && v->rows[0].selection.width>0);
        assert(mem_console_ui_cursor_index_for_click(fields[target],&state,&render,v->rows[0].origin.x,0,CORE_FONT_ROLE_UI_REGULAR,CORE_FONT_TEXT_SIZE_BASIC)==0);
        assert(state.text_edit.cursor==0 && state.text_edit.anchor==0 && !state.text_edit.composition[0]);
    }
    for(int target=0;target<6;++target)strcpy(fields[target],"mutated");
    for(int target=0;target<6;++target)assert(!strcmp(mem_console_ui_text_view((MemConsoleInputTarget)target)->row_text,"a中WWz"));
    assert(kit_render_end_frame(&render,&presented).code==CORE_OK);
    strcpy(state.body_edit_text,"first W row\nsecond é row\n");state.text_edit=(KitUiTextEdit){0};state.body_edit_cursor=0;state.input_target=MEM_CONSOLE_INPUT_BODY_EDIT;
    assert(kit_render_begin_frame(&render,1440,1000,&queue,&presented).code==CORE_OK);
    assert(mem_console_ui_draw_editable_body(&ui,&render,&presented,&state,(KitRenderRect){20,20,180,120},&input,0).code==CORE_OK);
    assert(mem_console_ui_text_view(MEM_CONSOLE_INPUT_BODY_EDIT)->count>=3);
    assert(kit_render_end_frame(&render,&presented).code==CORE_OK);
    memset(state.body_edit_text,'W',1000);state.body_edit_text[1000]=0;state.body_edit_cursor=1000;
    state.text_edit=(KitUiTextEdit){0};KitUiInputState body_pointer={0};body_pointer.mouse_x=50;body_pointer.mouse_y=50;
    assert(kit_render_begin_frame(&render,1440,1000,&queue,&presented).code==CORE_OK);
    assert(mem_console_ui_draw_editable_body(&ui,&render,&presented,&state,(KitRenderRect){20,20,180,120},&body_pointer,0).code==CORE_OK);
    float revealed=state.detail_body_scroll;assert(revealed>0);
    assert(kit_render_end_frame(&render,&presented).code==CORE_OK);
    assert(kit_render_begin_frame(&render,1440,1000,&queue,&presented).code==CORE_OK);
    assert(mem_console_ui_draw_editable_body(&ui,&render,&presented,&state,(KitRenderRect){20,20,180,120},&body_pointer,1).code==CORE_OK);
    assert(state.detail_body_scroll<revealed && state.detail_body_scroll>=0);
    assert(kit_render_end_frame(&render,&presented).code==CORE_OK);
    puts("Echo presentation: field click cancels preedit/collapses selection; body caret reveal, wheel scroll and scrollbar pass");
    puts("Echo six field presentation slots: Unicode preedit, measured hit, queued lifetime and explicit multiline rows pass");
    KitPaneComposition panes;assert(mem_console_ui_panes_build(&state,1440,1000,&panes).code==CORE_OK);
    assert(panes.count==3&&panes.entries[0].id==1&&panes.entries[2].id==3);
    KitUiInputState press={state.left_pane.x+10,state.left_pane.y+10,1,1,0};
    KitPanePointerOwner owner={0};CorePaneId pane=kit_pane_pointer_route(&owner,&panes,press.mouse_x,press.mouse_y,1,0,0);
    assert(pane==1&&mem_console_ui_pane_input(&press,pane,1).mouse_pressed&&!mem_console_ui_pane_input(&press,pane,3).mouse_pressed);
    assert(kit_pane_pointer_route(&owner,&panes,state.pane_right_graph.x+10,state.pane_right_graph.y+10,0,1,0)==1);
    puts("Echo production pane adapter: navigation/detail/graph identity and cross-pane release ownership pass");
    return 0;
}
