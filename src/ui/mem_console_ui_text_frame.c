#include "mem_console_ui_common.h"
#include <string.h>

/* The application submits one UI frame on its UI thread. Copy every transient
 * caption at queue time; resetting happens only at the next frame boundary. */
static char text_storage[262144];
static size_t text_used;
void mem_console_ui_text_frame_begin(void) { text_used=0; }
CoreResult mem_console_ui_frame_text(const char *source, const char **out) {
    if (!source || !out) return (CoreResult){CORE_ERR_INVALID_ARG,"invalid frame text"};
    size_t count=strlen(source)+1;
    if (count>sizeof(text_storage)-text_used)
        return (CoreResult){CORE_ERR_INVALID_ARG,"UI frame text storage exhausted"};
    memcpy(text_storage+text_used,source,count);
    *out=text_storage+text_used; text_used+=count;
    return core_result_ok();
}
CoreResult mem_console_ui_push_text(KitRenderFrame *frame, const KitRenderTextCommand *command) {
    if (!command) return (CoreResult){CORE_ERR_INVALID_ARG,"missing text command"};
    KitRenderTextCommand owned=*command;
    CoreResult result=mem_console_ui_frame_text(command->text,&owned.text);
    if (result.code!=CORE_OK) return result;
    return kit_render_push_text(frame,&owned);
}
