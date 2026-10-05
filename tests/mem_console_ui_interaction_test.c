#include "mem_console_workspace_authoring.h"
#include "mem_console_state.h"
#include "kit_workspace_authoring_interaction.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void route(MemConsoleState *state, KitRenderContext *render, KitUiContext *ui, SDL_Event *event) {
    mem_console_workspace_authoring_host_handle_sdl_event(&state->workspace_authoring,state,render,ui,event,0);
}

int main(void) {
    static MemConsoleState state;
    KitRenderContext render;
    KitUiContext ui;
    assert(kit_render_context_init(&render,KIT_RENDER_BACKEND_NULL,CORE_THEME_PRESET_DAW_DEFAULT,CORE_FONT_PRESET_DAW_DEFAULT).code==CORE_OK);
    assert(kit_ui_context_init(&ui,&render).code==CORE_OK);
    state.theme_preset_id=CORE_THEME_PRESET_DAW_DEFAULT;
    state.font_preset_id=CORE_FONT_PRESET_DAW_DEFAULT;
    state.text_zoom_step=kit_render_text_zoom_step(&render);
    mem_console_workspace_authoring_host_reset(&state.workspace_authoring);
    state.workspace_authoring.active=1;
    state.workspace_authoring.overlay_mode=MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_FONT_THEME;
    mem_console_workspace_authoring_host_set_viewport(&state.workspace_authoring,1440,1000);
    KitWorkspaceAuthoringFontThemeLayout layout;
    assert(kit_workspace_authoring_ui_font_theme_build_layout(&render,1440,1000,&layout));
    SDL_Event event={0};
    event.type=SDL_MOUSEBUTTONUP; event.button.button=SDL_BUTTON_LEFT;
    event.button.x=(int)(layout.text_size_inc_button.x+4); event.button.y=(int)(layout.text_size_inc_button.y+4);
    int step=state.text_zoom_step;
    route(&state,&render,&ui,&event); assert(state.text_zoom_step==step);
    event.type=SDL_MOUSEBUTTONDOWN; route(&state,&render,&ui,&event);
    assert(state.text_zoom_step==step && state.workspace_authoring.font_theme_interaction.captured_id==2u);
    event.type=SDL_MOUSEBUTTONUP; route(&state,&render,&ui,&event);
    assert(state.text_zoom_step>step && state.workspace_authoring.font_theme_button_click_count==1u);
    step=state.text_zoom_step;
    memset(&event,0,sizeof(event)); event.type=SDL_KEYDOWN; event.key.keysym.sym=SDLK_SPACE;
    route(&state,&render,&ui,&event); assert(state.text_zoom_step==step);
    event.key.repeat=1; route(&state,&render,&ui,&event);
    event.type=SDL_KEYUP; event.key.repeat=0; route(&state,&render,&ui,&event);
    assert(state.text_zoom_step>step && state.workspace_authoring.font_theme_button_click_count==2u);
    kit_ui_interaction_reset(&state.workspace_authoring.font_theme_interaction);
    event.type=SDL_KEYDOWN; event.key.keysym.sym=SDLK_TAB;
    route(&state,&render,&ui,&event); assert(state.workspace_authoring.font_theme_interaction.focused_id==4u);
    route(&state,&render,&ui,&event); assert(state.workspace_authoring.font_theme_interaction.focused_id==5u);
    route(&state,&render,&ui,&event); assert(state.workspace_authoring.font_theme_interaction.focused_id==1u);
    KitRenderCommand storage[1024]; KitRenderCommandBuffer buffer={storage,1024,0}; KitRenderFrame frame;
    assert(kit_render_begin_frame(&render,1440,1000,&buffer,&frame).code==CORE_OK);
    assert(mem_console_workspace_authoring_overlay_render(&render,&ui,&frame,&state,1440,1000).code==CORE_OK);
    assert(buffer.count && storage[buffer.count-1].kind==KIT_RENDER_CMD_RECT && storage[buffer.count-1].data.rect.color.b==255);
    assert(kit_render_end_frame(&render,&frame).code==CORE_OK);
    memset(&event,0,sizeof(event)); event.type=SDL_WINDOWEVENT; event.window.event=SDL_WINDOWEVENT_FOCUS_LOST;
    route(&state,&render,&ui,&event); assert(!state.workspace_authoring.font_theme_interaction.focused_id);
    event.type=SDL_QUIT;
    assert(!mem_console_workspace_authoring_host_handle_sdl_event(&state.workspace_authoring,&state,&render,&ui,&event,0));
    puts("eCho interaction: real preview actions, focus order, release/repeat, marker and quit passthrough pass");
    return 0;
}
