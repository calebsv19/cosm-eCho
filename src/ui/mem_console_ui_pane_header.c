#include "mem_console_ui_common.h"
#include "kit_pane_host.h"
CoreResult mem_console_ui_pane_header(KitRenderContext *render,KitUiContext *ui,
    KitRenderFrame *frame,MemConsoleState *s,KitRenderRect row,MemConsoleAction *action) {
    KitPaneHeaderLayout slots;
    KitPaneHeaderAction refresh={MEM_CONSOLE_ACTION_REFRESH_GRAPH,88,1};
    CoreResult r=kit_pane_header_layout(&slots,(CorePaneRect){row.x,row.y,row.width,row.height},0,8,48,&refresh,1);
    if(r.code!=CORE_OK)return r;
    CorePaneRect t=slots.title;
    r=mem_console_ui_draw_info_line_custom(ui,frame,(KitRenderRect){t.x,t.y,t.width,t.height},"GRAPH",
        CORE_THEME_COLOR_TEXT_PRIMARY,CORE_FONT_ROLE_UI_MEDIUM,CORE_FONT_TEXT_SIZE_CAPTION);
    if(r.code!=CORE_OK)return r;
    for(uint32_t i=0;i<slots.count;i++) {
        CorePaneRect b=slots.actions[i].bounds;KitRenderRect rect={b.x,b.y,b.width,b.height};
        KitUiButtonResult pressed=mem_console_ui_surface_button(ui,s,MC_BUTTON_PANE_HEADER,slots.actions[i].id,rect,slots.actions[i].enabled);
        if(pressed.clicked&&*action==MEM_CONSOLE_ACTION_NONE)*action=(MemConsoleAction)slots.actions[i].id;
        r=mem_console_ui_draw_button_custom(ui,frame,rect,"REFRESH",pressed.state,CORE_FONT_ROLE_UI_MEDIUM,CORE_FONT_TEXT_SIZE_CAPTION);
        if(r.code!=CORE_OK)return r;
    }
    (void)render;return core_result_ok();
}
