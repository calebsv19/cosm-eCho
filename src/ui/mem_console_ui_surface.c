#include "mem_console_ui_surface.h"
#include "mem_console_ui_common.h"
#include "kit_ui_interaction_sdl.h"
#include "kit_workspace_authoring_interaction.h"
#include <math.h>

uint32_t mem_console_ui_surface_scope(const MemConsoleState *s) {
    if (s->db_modal_open) return 4;
    if (mem_console_workspace_authoring_host_active(&s->workspace_authoring))
        return mem_console_workspace_authoring_host_font_theme_overlay_active(&s->workspace_authoring)?5:6;
    if (s->title_edit_mode) return 2;
    if (s->body_edit_mode) return 3;
    return 1;
}
uint64_t mem_console_ui_surface_string_key(const char *text) {
    uint64_t key=UINT64_C(14695981039346656037);
    if (text) for (const unsigned char *p=(const unsigned char *)text;*p;++p) { key^=*p; key*=UINT64_C(1099511628211); }
    return key;
}
static int domain_active(const MemConsoleState *state, uint32_t domain) {
    uint32_t scope=mem_console_ui_surface_scope(state);
    if (scope==4) return domain==MC_BUTTON_DB_ROW || domain==MC_BUTTON_DB_ACTION;
    if (scope==5 || scope==6) return domain==MC_BUTTON_OVERLAY || (scope==5 && domain==MC_BUTTON_FONT);
    return domain!=MC_BUTTON_FONT && domain!=MC_BUTTON_OVERLAY && domain!=MC_BUTTON_DB_ROW && domain!=MC_BUTTON_DB_ACTION;
}
KitUiButtonResult mem_console_ui_surface_button(KitUiContext *ui, MemConsoleState *state,
    uint32_t domain, uint64_t key, KitRenderRect bounds, int enabled) {
    KitUiButtonResult result={KIT_UI_STATE_NORMAL,0};
    if (!ui || !state) return result;
    KitUiSurface *s=&state->button_surface;
    KitUiInteractionControl control={0};
    if (bounds.width<=0 || bounds.height<=0 || !domain_active(state,domain)) {
        result.state=enabled?KIT_UI_STATE_NORMAL:KIT_UI_STATE_DISABLED; return result;
    }
    if (s->collecting) {
        KitRenderRect viewport={0,0,(float)state->workspace_authoring.viewport_width,(float)state->workspace_authoring.viewport_height};
        KitRenderRect clip=ui->clip_depth?ui->clip_stack[ui->clip_depth-1]:viewport;
        float right=fminf(clip.x+clip.width,viewport.width),bottom=fminf(clip.y+clip.height,viewport.height);
        clip.x=fmaxf(clip.x,0); clip.y=fmaxf(clip.y,0);
        clip.width=right-clip.x;clip.height=bottom-clip.y;
        if (clip.width<=0 || clip.height<=0) return result;
        if (kit_ui_surface_register(s,(KitUiSurfaceKey){domain,key},bounds,&clip,enabled,&control).code!=CORE_OK) return result;
    } else {
        for (uint32_t i=0;i<s->count;++i) if (s->keys[i].domain==domain && s->keys[i].value==key) { control=s->controls[i]; break; }
    }
    KitUiButtonState appearance=kit_ui_interaction_button_state(&s->interaction,&control,0);
    result.state=!enabled?KIT_UI_STATE_DISABLED:(appearance.pressed?KIT_UI_STATE_ACTIVE:
        (appearance.hovered?KIT_UI_STATE_HOVERED:KIT_UI_STATE_NORMAL));
    if (control.id && !state->button_defer_actions) result.clicked=kit_ui_surface_take_activation(s,control.id);
    return result;
}
void mem_console_ui_surface_text_focus(MemConsoleState *state) {
    if (!state) return;
    state->button_keyboard_text=1;
    state->button_surface.interaction.focused_id=0;
    state->button_surface.interaction.key_owner_id=0;
}
static void collect_authoring(MemConsoleState *state, KitRenderContext *render) {
    KitUiSurface *s=&state->button_surface;
    MemConsoleWorkspaceAuthoringHost *host=&state->workspace_authoring;
    KitRenderRect viewport={0,0,(float)host->viewport_width,(float)host->viewport_height};
    kit_ui_surface_begin(s,mem_console_ui_surface_scope(state));
    KitUiInteractionControl c;
    KitWorkspaceAuthoringOverlayButton buttons[4];
    uint32_t n=kit_workspace_authoring_ui_build_overlay_buttons((int)host->viewport_width,1,
        mem_console_workspace_authoring_host_pane_overlay_active(host),buttons,4);
    for (uint32_t i=0;i<n;++i) if (buttons[i].visible)
        (void)kit_ui_surface_register(s,(KitUiSurfaceKey){MC_BUTTON_OVERLAY,buttons[i].id},(KitRenderRect){buttons[i].rect.x,buttons[i].rect.y,buttons[i].rect.width,buttons[i].rect.height},&viewport,buttons[i].enabled,&c);
    if (mem_console_workspace_authoring_host_font_theme_overlay_active(host)) {
        KitWorkspaceAuthoringFontThemeLayout layout;
        if (kit_workspace_authoring_ui_font_theme_build_layout(render,(int)host->viewport_width,(int)host->viewport_height,&layout)) {
            KitUiInteractionControl controls[13]; n=kit_workspace_authoring_font_theme_controls(&layout,controls,13);
            for (uint32_t i=0;i<n;++i)
                (void)kit_ui_surface_register(s,(KitUiSurfaceKey){MC_BUTTON_FONT,controls[i].id},controls[i].bounds,&viewport,controls[i].enabled,&c);
        }
    }
    s->collection_result=kit_ui_surface_end(s);
}
int mem_console_ui_surface_event(MemConsoleState *state, KitRenderContext *render,
    KitUiContext *ui, const SDL_Event *event, KitUiInputState *input) {
    if (!state || !render || !ui || !event) return 0;
    KitUiInteractionEvent e;
    if (!kit_ui_interaction_event_from_sdl(event,&e)) return 0;
    KitUiSurface *s=&state->button_surface;
    uint32_t scope=mem_console_ui_surface_scope(state);
    if(kit_ui_focus_scope_sync(&state->focus_scope,s,scope,scope==4).code!=CORE_OK)return 1;
    if ((scope==5 || scope==6) && !state->db_modal_open) collect_authoring(state,render);
    if (e.type==KIT_UI_INTERACTION_KEY_DOWN && (state->button_keyboard_text ||
        ((scope==2 || scope==3 || scope==4) && !s->interaction.focused_id))) return 0;
    if (e.type==KIT_UI_INTERACTION_POINTER_DOWN) state->button_keyboard_text=0;
    KitUiInteractionResult result;
    CoreResult routed=kit_ui_surface_route(s,&e,&result);
    if (routed.code!=CORE_OK) {
        state->button_surface_error=routed;
        mem_console_redraw_mark(state,MEM_CONSOLE_REDRAW_REASON_INPUT);
        return 1;
    }
    KitUiSurfaceKey key;
    if (result.activated_id && kit_ui_surface_key(s,result.activated_id,&key) &&
        (key.domain==MC_BUTTON_FONT || key.domain==MC_BUTTON_OVERLAY) &&
        kit_ui_surface_take_activation(s,result.activated_id)) {
        if (key.domain==MC_BUTTON_FONT) mem_console_workspace_authoring_apply_font_theme_button(
            &state->workspace_authoring,state,render,ui,(KitWorkspaceAuthoringFontThemeButtonId)key.value);
        else mem_console_workspace_authoring_apply_overlay_button(&state->workspace_authoring,state,render,ui,
            (KitWorkspaceAuthoringOverlayButtonId)key.value);
    }
    if (result.consumed && input && e.type>=KIT_UI_INTERACTION_POINTER_MOVE && e.type<=KIT_UI_INTERACTION_POINTER_UP) {
        input->mouse_x=e.x; input->mouse_y=e.y;
        input->mouse_down=0; input->mouse_pressed=0; input->mouse_released=0;
    }
    if (e.type==KIT_UI_INTERACTION_CANCEL) {
        if (input) { input->mouse_down=0; input->mouse_pressed=0; input->mouse_released=0; }
        return 0;
    }
    return result.consumed;
}
