#include "mem_console_workspace_authoring.h"
#include "mem_console_state.h"
#include "kit_workspace_authoring_interaction.h"
#include "kit_ui_interaction_sdl.h"

int mem_console_workspace_authoring_interaction_event(MemConsoleWorkspaceAuthoringHost *host,
    MemConsoleState *state, KitRenderContext *render_ctx, KitUiContext *ui_ctx,
    const SDL_Event *event, int blocked) {
    if (!host || !state || !render_ctx || !ui_ctx || !event) return 0;
    if (blocked || !mem_console_workspace_authoring_host_font_theme_overlay_active(host)) {
        kit_ui_interaction_reset(&host->font_theme_interaction);
        return 0;
    }
    KitUiInteractionEvent normalized;
    if (!kit_ui_interaction_event_from_sdl(event,&normalized)) return 0;
    KitWorkspaceAuthoringFontThemeLayout layout;
    if (!kit_workspace_authoring_ui_font_theme_build_layout(render_ctx,
        (int)host->viewport_width,(int)host->viewport_height,&layout)) {
        kit_ui_interaction_reset(&host->font_theme_interaction);
        return 0;
    }
    KitUiInteractionControl controls[13];
    uint32_t count=kit_workspace_authoring_font_theme_controls(&layout,controls,13u);
    KitUiInteractionResult result;
    CoreResult routed=kit_ui_interaction_route(&host->font_theme_interaction,controls,count,&normalized,&result);
    if (routed.code != CORE_OK) return 1;
    if (result.activated_id)
        mem_console_workspace_authoring_apply_font_theme_button(host,state,render_ctx,ui_ctx,
            (KitWorkspaceAuthoringFontThemeButtonId)result.activated_id);
    /* Lifecycle events must still reach their window/quit owners. */
    if (normalized.type==KIT_UI_INTERACTION_CANCEL) return 0;
    return result.consumed;
}
