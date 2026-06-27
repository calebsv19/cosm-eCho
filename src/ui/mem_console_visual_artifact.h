#ifndef MEM_CONSOLE_VISUAL_ARTIFACT_H
#define MEM_CONSOLE_VISUAL_ARTIFACT_H

#include "kit_render.h"

int mem_console_visual_artifact_capture_if_requested(const KitRenderCommandBuffer *commands,
                                                     uint32_t width_px,
                                                     uint32_t height_px);
int mem_console_visual_artifact_run_cli(int argc, char **argv);

#endif
