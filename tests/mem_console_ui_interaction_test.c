#include "mem_console_workspace_authoring.h"
#include "mem_console_ui_surface.h"
#include "mem_console_ui.h"
#include "app/mem_console_app_internal.h"
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
    pointer(&state,&render,&ui,root,SDL_MOUSEBUTTONUP);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE);
    for(uint32_t i=0;i<state.button_surface.count;++i)
        assert(state.button_surface.keys[i].domain==MC_BUTTON_DB_ROW || state.button_surface.keys[i].domain==MC_BUTTON_DB_ACTION);
    KitRenderRect cancel=control(&state,MC_BUTTON_DB_ACTION,4101).bounds;
    pointer(&state,&render,&ui,cancel,SDL_MOUSEBUTTONDOWN);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE);
    pointer(&state,&render,&ui,cancel,SDL_MOUSEBUTTONUP);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_CANCEL_DB_PICKER);
    state.db_modal_open=0; frame(&state,&render,&ui);
    root=control(&state,MC_BUTTON_LEFT,1103).bounds;
    pointer(&state,&render,&ui,root,SDL_MOUSEBUTTONDOWN);
    pointer(&state,&render,&ui,(KitRenderRect){0,0,1,1},SDL_MOUSEBUTTONUP);
    assert(frame(&state,&render,&ui)==MEM_CONSOLE_ACTION_NONE);
    /* Text focus passes plain activation keys to the existing editor owner. */
    mem_console_ui_surface_text_focus(&state);
    e.type=SDL_KEYDOWN; e.key.keysym.sym=SDLK_SPACE;
    assert(!mem_console_ui_surface_event(&state,&render,&ui,&e,&input));
    memset(&e,0,sizeof(e)); e.type=SDL_WINDOWEVENT; e.window.event=SDL_WINDOWEVENT_FOCUS_LOST;
    route(&state,&render,&ui,&e); assert(!state.button_surface.interaction.focused_id);
    e.type=SDL_QUIT; route(&state,&render,&ui,&e); assert(!running);
    puts("eCho surfaces: production event/frame routes, preview/top controls, HUD release actions, FIFO, modal exclusion, editor ownership and quit pass");
    return 0;
}
