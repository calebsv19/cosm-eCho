#include "mem_console_ui_pane_composition.h"
#include <math.h>
static CorePaneRect rect(KitRenderRect r){return (CorePaneRect){r.x,r.y,r.width,r.height};}
CoreResult mem_console_ui_panes_build(const MemConsoleState *s,int width,int height,KitPaneComposition *view) {
    if(!s)return (CoreResult){CORE_ERR_INVALID_ARG,"invalid Echo pane state"};
    /* Existing product headers are content; no extra title row is injected. */
    KitPaneCompositionSpec specs[]={
        {1,rect(s->left_pane),0,0,0,1},
        {2,rect(s->pane_right_detail),0,0,0,1},
        {3,rect(s->pane_right_graph),0,0,0,1}};
    return kit_pane_composition_build(view,specs,3,(CorePaneRect){0,0,width,height});
}
KitUiInputState mem_console_ui_pane_input(const KitUiInputState *input,CorePaneId owner,CorePaneId pane) {
    KitUiInputState out=*input;
    if(owner!=pane){out.mouse_down=out.mouse_pressed=out.mouse_released=0;out.mouse_x=out.mouse_y=-1000000.0f;}
    return out;
}
