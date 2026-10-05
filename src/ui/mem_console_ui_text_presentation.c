#include "mem_console_ui_common.h"
#include <math.h>
#include <string.h>
/* One slot per field: multiple queued rows never alias another field's storage. */
typedef struct TextSlot {
    KitUiTextPresentation view;
    const char *source;
    size_t capacity;
    const KitRenderContext *render;
    CoreFontRoleId role;
    CoreFontTextSizeTier tier;
    size_t cursor,anchor;
    char composition[sizeof(((KitUiTextEdit*)0)->composition)];
} TextSlot;
static TextSlot slots[6];
static CoreResult measure(void *user,const char *text,float *width) {
    TextSlot *slot=user;if(!text[0]){*width=0;return core_result_ok();}
    KitRenderTextMetrics m;CoreResult r=kit_render_measure_text(slot->render,slot->role,slot->tier,text,&m);
    if(r.code==CORE_OK)*width=m.width_px;return r;
}
const KitUiTextPresentation *mem_console_ui_text_view(MemConsoleInputTarget target) {
    return target>=0 && target<6?&slots[target].view:NULL;
}
static TextSlot *field(MemConsoleState *s,const char *text,size_t *capacity,MemConsoleInputTarget *target) {
#define FIELD(name,id) if(text==s->name){*capacity=sizeof(s->name);*target=id;return &slots[id];}
    FIELD(search_text,MEM_CONSOLE_INPUT_SEARCH)
    FIELD(title_edit_text,MEM_CONSOLE_INPUT_TITLE_EDIT)
    FIELD(body_edit_text,MEM_CONSOLE_INPUT_BODY_EDIT)
    FIELD(graph_edge_limit_text,MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT)
    FIELD(db_modal_text,MEM_CONSOLE_INPUT_DB_PATH)
    FIELD(relationship_target_text,MEM_CONSOLE_INPUT_RELATIONSHIP_TARGET)
#undef FIELD
    return NULL;
}
static CoreResult present(KitUiContext *ui,const KitRenderContext *render,KitRenderFrame *frame,
    MemConsoleState *s,KitRenderRect viewport,const char *text,CoreThemeColorToken token,
    CoreFontRoleId role,CoreFontTextSizeTier tier,int active,int cursor,int wrap,int wheel) {
    size_t cap;MemConsoleInputTarget target;TextSlot *slot=field(s,text,&cap,&target);
    if(!slot)return (CoreResult){CORE_ERR_INVALID_ARG,"unregistered editable field"};
    KitUiTextEdit edit=s->text_edit;int changed=edit.text!=text;
    CoreResult r=kit_ui_text_bind(&edit,(char*)text,cap,wrap?0:KIT_UI_TEXT_SINGLE_LINE);if(r.code!=CORE_OK)return r;
    int anchor=target==MEM_CONSOLE_INPUT_DB_PATH?s->db_modal_selection_anchor:cursor;
    if(changed || (int)edit.cursor!=cursor || (target==MEM_CONSOLE_INPUT_DB_PATH && (int)edit.anchor!=anchor)) {
        r=kit_ui_text_position(&edit,(size_t)mem_console_ui_clamp_cursor_for_text(text,cursor),(size_t)mem_console_ui_clamp_cursor_for_text(text,anchor));
        if(r.code!=CORE_OK)return r;
    }
    /* Domain/input eligibility stays with the host; focused buttons hide text ownership. */
    active=active && (s->button_keyboard_text || !s->button_surface.interaction.focused_id);
    slot->render=render;slot->role=role;slot->tier=tier;
    KitRenderTextMetrics m;r=kit_render_measure_text(render,role,tier,"Ag",&m);if(r.code!=CORE_OK)return r;
    float line=fmaxf(1,m.height_px);float scroll=wrap?s->detail_body_scroll:0;
    int reveal=!wrap || (active && (slot->source!=text || slot->cursor!=edit.cursor || slot->anchor!=edit.anchor || strcmp(slot->composition,edit.composition)));
    if(wrap && wheel) {scroll=fmaxf(0,scroll-wheel*line*3);reveal=0;}
    KitUiTextPresentationOptions options={viewport,line,1,scroll,wrap,active,reveal};
    r=kit_ui_text_presentation_build(&slot->view,&edit,&options,measure,slot);if(r.code!=CORE_OK)return r;
    slot->source=text;slot->capacity=cap;slot->cursor=edit.cursor;slot->anchor=edit.anchor;strcpy(slot->composition,edit.composition);
    if(wrap)s->detail_body_scroll=slot->view.scroll_y;
    KitRenderColor accent;r=mem_console_ui_resolve_theme_color(render,CORE_THEME_COLOR_ACCENT_PRIMARY,&accent);if(r.code!=CORE_OK)return r;
    KitUiTextPresentationColors colors={accent,accent,accent};colors.selection.a=72;
    return kit_ui_text_presentation_render(ui,frame,&slot->view,role,tier,token,&colors);
}
CoreResult mem_console_ui_draw_editable_line(KitUiContext *ui,const KitRenderContext *render,KitRenderFrame *frame,
    MemConsoleState *s,KitRenderRect rect,const char *text,CoreThemeColorToken token,
    CoreFontRoleId role,CoreFontTextSizeTier tier,int active,int cursor) {
    size_t capacity;MemConsoleInputTarget target;
    if(!field(s,text,&capacity,&target) && !active) {
        CoreResult r=kit_ui_clip_push(ui,frame,rect);if(r.code!=CORE_OK)return r;
        r=mem_console_ui_draw_info_line_custom(ui,frame,rect,text,token,role,tier);
        CoreResult pop=kit_ui_clip_pop(ui,frame);return r.code!=CORE_OK?r:pop;
    }
    KitRenderTextMetrics m;CoreResult r=kit_render_measure_text(render,role,tier,"Ag",&m);if(r.code!=CORE_OK)return r;
    rect.x+=ui->style.padding;rect.width-=2*ui->style.padding;
    rect.y+=fmaxf(0,(rect.height-m.height_px)*.5f);rect.height=fminf(rect.height,m.height_px);
    return present(ui,render,frame,s,rect,text,token,role,tier,active,cursor,0,0);
}
int mem_console_ui_cursor_index_for_click(const char *text,MemConsoleState *state,const KitRenderContext *render,float x,float unused,
    CoreFontRoleId role,CoreFontTextSizeTier tier) {
    (void)render;(void)unused;(void)role;(void)tier;
    for(int i=0;i<6;++i)if(slots[i].source==text) {
        size_t position;TextSlot *slot=&slots[i];
        if(kit_ui_text_presentation_hit(&slot->view,x,slot->view.options.viewport.y,measure,slot,&position).code==CORE_OK) {
            unsigned flags=i==MEM_CONSOLE_INPUT_BODY_EDIT?0:KIT_UI_TEXT_SINGLE_LINE;
            if(i==MEM_CONSOLE_INPUT_GRAPH_EDGE_LIMIT || i==MEM_CONSOLE_INPUT_RELATIONSHIP_TARGET)flags|=KIT_UI_TEXT_DIGITS;
            if(kit_ui_text_bind(&state->text_edit,(char*)text,slot->capacity,flags).code==CORE_OK)
                (void)kit_ui_text_position(&state->text_edit,position,position);
            return (int)position;
        }
    }
    return 0;
}
CoreResult mem_console_ui_draw_editable_body(KitUiContext *ui,const KitRenderContext *render,KitRenderFrame *frame,
    MemConsoleState *s,KitRenderRect rect,const KitUiInputState *input,int wheel) {
    rect.x+=8;rect.y+=8;rect.width-=16;rect.height-=16;
    KitRenderRect scroll_bounds=rect;rect.width-=10;
    if(rect.width<=0 || rect.height<=0)return core_result_ok();
    CoreResult r=present(ui,render,frame,s,rect,s->body_edit_text,CORE_THEME_COLOR_TEXT_MUTED,
        CORE_FONT_ROLE_UI_REGULAR,CORE_FONT_TEXT_SIZE_BASIC,s->input_target==MEM_CONSOLE_INPUT_BODY_EDIT,s->body_edit_cursor,1,
        kit_ui_point_in_rect(rect,input->mouse_x,input->mouse_y)?wheel:0);
    if(r.code!=CORE_OK)return r;
    const KitUiTextPresentation *view=&slots[MEM_CONSOLE_INPUT_BODY_EDIT].view;
    if(view->content_height>rect.height) {
        r=kit_ui_draw_scrollbar(ui,frame,scroll_bounds,s->detail_body_scroll,view->content_height);if(r.code!=CORE_OK)return r;
    }
    if(input->mouse_released && kit_ui_point_in_rect(rect,input->mouse_x,input->mouse_y)) {
        TextSlot *slot=&slots[MEM_CONSOLE_INPUT_BODY_EDIT];size_t position;
        if(kit_ui_text_presentation_hit(&slot->view,input->mouse_x,input->mouse_y,measure,slot,&position).code==CORE_OK) {
            mem_console_input_target_set(s,MEM_CONSOLE_INPUT_BODY_EDIT);mem_console_ui_surface_text_focus(s);
            s->body_edit_cursor=(int)position;
            if(kit_ui_text_bind(&s->text_edit,s->body_edit_text,sizeof(s->body_edit_text),0).code==CORE_OK)
                (void)kit_ui_text_position(&s->text_edit,position,position);
        }
    }
    return core_result_ok();
}
