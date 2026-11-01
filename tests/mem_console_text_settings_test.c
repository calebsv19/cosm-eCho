/* Real renderer + host settings lifecycle, with loader fault injection. */
#include "app/mem_console_app_internal.h"
#include "mem_console_ui.h"
#include "mem_console_ui_common.h"
#include "mem_console_prefs.h"
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int fail_after = -1, open_calls;
TTF_Font *TTF_OpenFont(const char *file, int size) {
    static TTF_Font *(*real_open)(const char *, int);
    if (!real_open) { *(void **)(&real_open) = dlsym(RTLD_NEXT, "TTF_OpenFont"); assert(real_open); }
    ++open_calls;
    if (fail_after == 0) return NULL;
    if (fail_after > 0) --fail_after;
    return real_open(file, size);
}
static MemConsoleState state, reopened;
static KitUiInputState input;
static void enter(KitRenderContext *render, KitUiContext *ui) {
    SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_c;e.key.keysym.mod=KMOD_ALT;
    assert(mem_console_workspace_authoring_host_handle_sdl_event(&state.workspace_authoring,&state,render,ui,&e,0));
    e.key.keysym.sym=SDLK_v;
    assert(mem_console_workspace_authoring_host_handle_sdl_event(&state.workspace_authoring,&state,render,ui,&e,0));
    assert(state.workspace_authoring.active);
    state.workspace_authoring.overlay_mode=MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_FONT_THEME;
}
static void capture(KitRenderContext *r,KitUiContext *ui,VkRenderer *vk,const char *root,const char *name,int w,int h) {
    char path[4096];snprintf(path,sizeof(path),"%s/%s.ppm",root,name);
    assert(vk_renderer_request_capture(vk,path)==VK_SUCCESS);
    MemConsoleAction action;
    assert(run_frame(r,ui,&state,&input,w,h,0,&action)==MEM_CONSOLE_FRAME_OK);
}
int main(int argc,char **argv) {
    assert(argc==2);
    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0 && TTF_Init()==0);
    SDL_Window *window=SDL_CreateWindow("eCho text settings qualification",0,0,1200,800,SDL_WINDOW_VULKAN|SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_RESIZABLE);assert(window);
    VkRenderer vk;VkRendererConfig cfg;vk_renderer_config_set_defaults(&cfg);cfg.enable_validation=VK_TRUE;
    assert(vk_renderer_init(&vk,window,&cfg)==VK_SUCCESS);
    int dw,dh;SDL_Vulkan_GetDrawableSize(window,&dw,&dh);vk_renderer_set_logical_size(&vk,1200,800);
    KitRenderContext r;KitUiContext ui;
    seed_state(&state,"fixture.sqlite");
    assert(kit_render_context_init(&r,KIT_RENDER_BACKEND_VULKAN,state.theme_preset_id,state.font_preset_id).code==CORE_OK);
    assert(kit_render_attach_external_backend(&r,&vk).code==CORE_OK);
    assert(mem_console_app_prepare_initial_text(&r));
    assert(kit_ui_context_init(&ui,&r).code==CORE_OK);
    mem_console_app_apply_compact_ui_density(&ui,&r);
    state.selected_item_id=42;state.visible_count=1;state.visible_items[0].id=42;
    snprintf(state.visible_items[0].title,sizeof(state.visible_items[0].title),"Font and zoom fixture | Il1 MWmw 012345");
    snprintf(state.title_edit_text,sizeof(state.title_edit_text),"eCho title: UTF-8 café — Ω 漢字");
    snprintf(state.body_edit_text,sizeof(state.body_edit_text),"Editable text, caret and selection.\nWrapped paragraphs retain the shared measurement contract across fonts, zoom, and drawable scales.\n0123456789 | Il1 MWmw.");
    state.input_target=MEM_CONSOLE_INPUT_SEARCH;strcpy(state.search_text,"search café Ω");state.search_cursor=(int)strlen(state.search_text);
    char prefs[4096];snprintf(prefs,sizeof(prefs),"%s/preferences.pack",argv[1]);
    assert(mem_console_prefs_save(prefs,&state).code==CORE_OK);
    capture(&r,&ui,&vk,argv[1],"initial",1200,800);
    uint64_t initial=mem_console_prefs_state_signature(&state);
    state.pane_prefs_dirty=0;
    assert(mem_console_prefs_save(prefs,&state).code==CORE_OK);
    void *entry=r.backend_state;int entry_zoom=state.text_zoom_step;CoreFontPresetId entry_font=state.font_preset_id;
    /* A late loader failure must roll back all partially prepared sources. */
    for(int failure=0;failure<3;failure++) {
        fail_after=failure;int calls=open_calls;
        assert(mem_console_app_handle_text_zoom_shortcut(&r,&ui,&state,prefs,SDLK_PLUS));
        assert(open_calls>calls && r.backend_state==entry && state.text_zoom_step==entry_zoom);
        assert(mem_console_prefs_state_signature(&state)==initial);
        assert(!state.pane_prefs_dirty);
    }
    /* A font shortcut and active preview refuse through the same transaction. */
    fail_after=0;
    assert(mem_console_app_handle_font_shortcut(&r,&ui,&state,prefs,SDLK_u));
    assert(r.backend_state==entry && state.font_preset_id==entry_font && !state.pane_prefs_dirty);
    int warm_calls=open_calls;
    capture(&r,&ui,&vk,argv[1],"rejected",1200,800);
    assert(open_calls==warm_calls); /* Last-good native rasters survive refusal. */
    fail_after=-1;
    KitRenderContext untouched;memset(&untouched,0x5a,sizeof(untouched));KitRenderContext before=untouched;
    assert(kit_render_prepare_text_context(&r,(CoreFontPresetId)999,0,1,&untouched).code!=CORE_OK);
    assert(!memcmp(&before,&untouched,sizeof(before)));
    state.pane_prefs_dirty=1;enter(&r,&ui);
    fail_after=2;
    assert(mem_console_workspace_authoring_apply_font_theme_button(&state.workspace_authoring,&state,&r,&ui,KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_INC));
    assert(state.text_zoom_step==entry_zoom && !state.workspace_authoring.baseline_render_valid && !state.workspace_authoring.font_theme_pending_changes);
    fail_after=-1;
    assert(mem_console_workspace_authoring_apply_font_theme_button(&state.workspace_authoring,&state,&r,&ui,KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_INC));
    assert(state.text_zoom_step==entry_zoom+1 && state.workspace_authoring.baseline_render.backend_state==entry);
    capture(&r,&ui,&vk,argv[1],"preview",1200,800);
    /* Multiple preview sets exercise three simultaneously live banks (75 sources). */
    for(int i=0;i<8;i++) assert(mem_console_app_apply_text_settings(&r,&ui,&state,state.theme_preset_id,(CoreFontPresetId)(i%CORE_FONT_PRESET_COUNT),i%4));
    fail_after=0;int calls=open_calls;
    assert(mem_console_workspace_authoring_apply_overlay_button(&state.workspace_authoring,&state,&r,&ui,KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_CANCEL));
    assert(open_calls==calls && r.backend_state==entry && state.text_zoom_step==entry_zoom && state.font_preset_id==entry_font && state.pane_prefs_dirty==1);
    fail_after=-1;capture(&r,&ui,&vk,argv[1],"cancel",1200,800);
    state.pane_prefs_dirty=0;enter(&r,&ui);
    assert(mem_console_app_apply_text_settings(&r,&ui,&state,state.theme_preset_id,CORE_FONT_PRESET_IDE,2));
    assert(mem_console_workspace_authoring_apply_overlay_button(&state.workspace_authoring,&state,&r,&ui,KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_APPLY));
    assert(state.pane_prefs_dirty && !state.workspace_authoring.baseline_render_valid);
    assert(mem_console_prefs_save(prefs,&state).code==CORE_OK);
    seed_state(&reopened,"fixture.sqlite");assert(mem_console_prefs_load(prefs,&reopened).code==CORE_OK);
    assert(reopened.text_zoom_step==2 && reopened.font_preset_id==CORE_FONT_PRESET_IDE);
    capture(&r,&ui,&vk,argv[1],"applied",1200,800);
    /* Every shipped preset and supported zoom step, with actual production text measurements. */
    int measurements=0;
    for(int f=0;f<CORE_FONT_PRESET_COUNT;f++) for(int z=-4;z<=5;z++) {
        assert(mem_console_app_apply_text_settings(&r,&ui,&state,state.theme_preset_id,(CoreFontPresetId)f,z));
        for(int role=0;role<CORE_FONT_ROLE_COUNT;role++) for(int tier=0;tier<CORE_FONT_TEXT_SIZE_COUNT;tier++) {
            KitRenderTextMetrics m;assert(kit_render_measure_text(&r,(CoreFontRoleId)role,(CoreFontTextSizeTier)tier,"Ag Il1 MWmw café Ω",&m).code==CORE_OK);
            assert(m.width_px>0 && m.height_px>0);measurements++;
        }
    }
    assert(mem_console_app_apply_text_settings(&r,&ui,&state,state.theme_preset_id,CORE_FONT_PRESET_IDE,0));
    /* Database preference reapplication uses the same guarded settings owner. */
    CoreMemDb db={0};char old_db[4096],next_db[4096],bad_db[4096],path[4096];
    snprintf(old_db,sizeof(old_db),"%s/old.sqlite",argv[1]);
    snprintf(next_db,sizeof(next_db),"%s/next.sqlite",argv[1]);
    snprintf(bad_db,sizeof(bad_db),"%s/bad-font.sqlite",argv[1]);
    assert(core_memdb_open(old_db,&db).code==CORE_OK);
    assert(mem_console_build_prefs_path(next_db,path,sizeof(path)));
    (void)state_set_font_preset(&reopened,CORE_FONT_PRESET_DAW_DEFAULT);(void)state_set_text_zoom_step(&reopened,-2);
    assert(mem_console_prefs_save(path,&reopened).code==CORE_OK);
    int path_valid=0,signature_valid=0;uint64_t signature=0;
    enter(&r,&ui);
    assert(mem_console_app_apply_text_settings(&r,&ui,&state,state.theme_preset_id,CORE_FONT_PRESET_IDE,1));
    assert(state.workspace_authoring.baseline_render_valid);
    assert(mem_console_app_switch_active_db(&db,&state,&r,&ui,next_db,"",0,path,sizeof(path),&path_valid,&signature_valid,&signature).code==CORE_OK);
    assert(!state.workspace_authoring.active && !state.workspace_authoring.baseline_render_valid);
    assert(state.font_preset_id==CORE_FONT_PRESET_DAW_DEFAULT && state.text_zoom_step==-2);
    char bad_prefs[4096];assert(mem_console_build_prefs_path(bad_db,bad_prefs,sizeof(bad_prefs)));
    (void)state_set_font_preset(&reopened,CORE_FONT_PRESET_IDE);(void)state_set_text_zoom_step(&reopened,3);
    assert(mem_console_prefs_save(bad_prefs,&reopened).code==CORE_OK);
    void *last_good=r.backend_state;fail_after=2;
    assert(mem_console_app_switch_active_db(&db,&state,&r,&ui,bad_db,"",0,path,sizeof(path),&path_valid,&signature_valid,&signature).code!=CORE_OK);
    assert(r.backend_state==last_good && state.font_preset_id==r.font.id && state.text_zoom_step==kit_render_text_zoom_step(&r));
    fail_after=-1;assert(core_memdb_close(&db).code==CORE_OK);
    assert(mem_console_app_apply_text_settings(&r,&ui,&state,state.theme_preset_id,CORE_FONT_PRESET_IDE,0));
    SDL_SetWindowSize(window,960,640);SDL_PumpEvents();SDL_Delay(40);
    assert(vk_renderer_recreate_swapchain(&vk,window)==VK_SUCCESS);vk_renderer_set_logical_size(&vk,960,640);
    capture(&r,&ui,&vk,argv[1],"compact",960,640);
    const VkRuntimeCapabilityReport *report=vk_runtime_get_capability_report(&vk.context.device->runtime);
    assert(report&&report->validation_enabled&&!report->validation_warning_count&&!report->validation_error_count);
    printf("eCho text settings PASS: drawable=%dx%d; 6 loader refusals, DB preference switch, exact Cancel/no open, dirty preservation, Apply/reopen, %d measurements, compact resize; validation clean\n",dw,dh,measurements);
    kit_render_context_shutdown(&r);vk_renderer_shutdown(&vk);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();return 0;
}
