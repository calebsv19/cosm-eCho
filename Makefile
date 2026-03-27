CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2
PKG_CONFIG ?= pkg-config
SHARED_ROOT ?= third_party/codework_shared

CORE_BASE_DIR ?= $(SHARED_ROOT)/core/core_base
CORE_THEME_DIR ?= $(SHARED_ROOT)/core/core_theme
CORE_FONT_DIR ?= $(SHARED_ROOT)/core/core_font
CORE_PACK_DIR ?= $(SHARED_ROOT)/core/core_pack
CORE_TIME_DIR ?= $(SHARED_ROOT)/core/core_time
CORE_QUEUE_DIR ?= $(SHARED_ROOT)/core/core_queue
CORE_SCHED_DIR ?= $(SHARED_ROOT)/core/core_sched
CORE_JOBS_DIR ?= $(SHARED_ROOT)/core/core_jobs
CORE_WORKERS_DIR ?= $(SHARED_ROOT)/core/core_workers
CORE_WAKE_DIR ?= $(SHARED_ROOT)/core/core_wake
CORE_KERNEL_DIR ?= $(SHARED_ROOT)/core/core_kernel
CORE_MEMDB_DIR ?= $(SHARED_ROOT)/core/core_memdb
CORE_PANE_DIR ?= $(SHARED_ROOT)/core/core_pane
KIT_RENDER_DIR ?= $(SHARED_ROOT)/kit/kit_render
KIT_UI_DIR ?= $(SHARED_ROOT)/kit/kit_ui
KIT_GRAPH_STRUCT_DIR ?= $(SHARED_ROOT)/kit/kit_graph_struct
VK_RENDERER_DIR ?= $(SHARED_ROOT)/vk_renderer

VULKAN_CFLAGS := $(shell $(PKG_CONFIG) --cflags vulkan 2>/dev/null)
SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_TTF_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2_ttf 2>/dev/null)
ifeq ($(strip $(VULKAN_CFLAGS)),)
  VULKAN_CFLAGS := -I/opt/homebrew/include
endif
ifeq ($(strip $(SDL_CFLAGS)),)
  SDL_CFLAGS := -I/opt/homebrew/include/SDL2
endif
ifeq ($(strip $(SDL_TTF_CFLAGS)),)
  SDL_TTF_CFLAGS := -I/opt/homebrew/include/SDL2
endif
VULKAN_LIBS := $(shell $(PKG_CONFIG) --libs vulkan 2>/dev/null)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 2>/dev/null)
SDL_TTF_LIBS := $(shell $(PKG_CONFIG) --libs sdl2_ttf 2>/dev/null)
ifeq ($(strip $(VULKAN_LIBS)),)
  VULKAN_LIBS := -L/opt/homebrew/lib -lvulkan
endif
ifeq ($(strip $(SDL_LIBS)),)
  SDL_LIBS := -L/opt/homebrew/lib -lSDL2
endif
ifeq ($(strip $(SDL_TTF_LIBS)),)
  SDL_TTF_LIBS := -L/opt/homebrew/lib -lSDL2_ttf
endif
APPLE_FW := -framework Metal -framework QuartzCore -framework Cocoa -framework IOKit -framework CoreVideo

INC = -Isrc -Iinclude -Iinclude/mem_console -I$(CORE_MEMDB_DIR)/include -I$(CORE_PACK_DIR)/include -I$(CORE_TIME_DIR)/include -I$(CORE_QUEUE_DIR)/include -I$(CORE_SCHED_DIR)/include -I$(CORE_JOBS_DIR)/include -I$(CORE_WORKERS_DIR)/include -I$(CORE_WAKE_DIR)/include -I$(CORE_KERNEL_DIR)/include -I$(CORE_PANE_DIR)/include -I$(KIT_UI_DIR)/include -I$(KIT_GRAPH_STRUCT_DIR)/include -I$(KIT_RENDER_DIR)/include -I$(VK_RENDERER_DIR)/include -I$(CORE_BASE_DIR)/include -I$(CORE_THEME_DIR)/include -I$(CORE_FONT_DIR)/include $(VULKAN_CFLAGS) $(SDL_CFLAGS) $(SDL_TTF_CFLAGS)

OBJ_DIR = build
BIN = $(OBJ_DIR)/mem_console
	SRC = src/app/mem_console.c \
		src/app/mem_console_app_actions.c \
		src/app/mem_console_app_db_switch.c \
		src/app/mem_console_app_events.c \
		src/app/mem_console_app_loop.c \
		src/app/mem_console_app_theme.c \
		src/app/mem_console_kernel_bridge.c \
		src/db/mem_console_db.c \
	src/db/mem_console_db_filters.c \
	src/db/mem_console_db_mutations.c \
	src/db/mem_console_db_reads.c \
	src/runtime/mem_console_prefs.c \
	src/runtime/mem_console_runtime.c \
	src/runtime/mem_console_state.c \
	src/runtime/mem_console_state_core.c \
	src/runtime/mem_console_state_graph_filters.c \
	src/runtime/mem_console_state_paths.c \
	src/runtime/mem_console_state_project_filters.c \
	src/layout/mem_console_layout_config.c \
	src/layout/mem_console_pane_layout.c \
	src/ui/mem_console_ui.c \
	src/ui/mem_console_ui_chrome.c \
	src/ui/mem_console_ui_common.c \
	src/ui/mem_console_ui_detail_panel.c \
	src/ui/mem_console_ui_detail_section.c \
	src/ui/mem_console_ui_hud.c \
	src/ui/mem_console_ui_left_panel.c \
	src/ui/mem_console_ui_left_section.c \
	src/ui/graph/mem_console_ui_graph.c \
	src/ui/graph/mem_console_ui_graph_camera.c \
	src/ui/graph/mem_console_ui_graph_controls.c \
	src/ui/graph/mem_console_ui_graph_draw.c \
	src/ui/graph/mem_console_ui_graph_geometry.c \
	src/ui/graph/mem_console_ui_graph_hud.c \
	src/ui/graph/mem_console_ui_graph_layout.c \
	src/ui/graph/mem_console_ui_graph_overlay.c \
	src/ui/graph/mem_console_ui_graph_project_pods.c \
	src/ui/graph/mem_console_ui_graph_types.c \
	src/ui/graph/mem_console_ui_graph_panel.c

.PHONY: all clean run run-demo vk-renderer-lib

all: $(BIN)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(KIT_RENDER_DIR)/build/vk/libkit_render.a:
	$(MAKE) -C $(KIT_RENDER_DIR) KIT_RENDER_ENABLE_VK=1

$(KIT_UI_DIR)/build/libkit_ui.a: $(KIT_RENDER_DIR)/build/vk/libkit_render.a
	$(MAKE) -C $(KIT_UI_DIR) KIT_RENDER_ENABLE_VK=1

$(KIT_GRAPH_STRUCT_DIR)/build/libkit_graph_struct.a: $(KIT_UI_DIR)/build/libkit_ui.a
	$(MAKE) -C $(KIT_GRAPH_STRUCT_DIR) KIT_RENDER_ENABLE_VK=1

$(CORE_BASE_DIR)/build/libcore_base.a:
	$(MAKE) -C $(CORE_BASE_DIR)

$(CORE_THEME_DIR)/build/libcore_theme.a: $(CORE_BASE_DIR)/build/libcore_base.a
	$(MAKE) -C $(CORE_THEME_DIR)

$(CORE_FONT_DIR)/build/libcore_font.a: $(CORE_BASE_DIR)/build/libcore_base.a
	$(MAKE) -C $(CORE_FONT_DIR)

$(CORE_QUEUE_DIR)/build/libcore_queue.a:
	$(MAKE) -C $(CORE_QUEUE_DIR)

$(CORE_TIME_DIR)/build/libcore_time.a:
	$(MAKE) -C $(CORE_TIME_DIR)

$(CORE_SCHED_DIR)/build/libcore_sched.a:
	$(MAKE) -C $(CORE_SCHED_DIR)

$(CORE_JOBS_DIR)/build/libcore_jobs.a:
	$(MAKE) -C $(CORE_JOBS_DIR)

$(CORE_WORKERS_DIR)/build/libcore_workers.a: $(CORE_QUEUE_DIR)/build/libcore_queue.a
	$(MAKE) -C $(CORE_WORKERS_DIR)

$(CORE_WAKE_DIR)/build/libcore_wake.a:
	$(MAKE) -C $(CORE_WAKE_DIR)

$(CORE_KERNEL_DIR)/build/libcore_kernel.a: $(CORE_TIME_DIR)/build/libcore_time.a $(CORE_SCHED_DIR)/build/libcore_sched.a $(CORE_JOBS_DIR)/build/libcore_jobs.a $(CORE_WAKE_DIR)/build/libcore_wake.a $(CORE_QUEUE_DIR)/build/libcore_queue.a
	$(MAKE) -C $(CORE_KERNEL_DIR)

$(CORE_MEMDB_DIR)/build/libcore_memdb.a: $(CORE_BASE_DIR)/build/libcore_base.a
	$(MAKE) -C $(CORE_MEMDB_DIR)

$(CORE_PANE_DIR)/build/libcore_pane.a:
	$(MAKE) -C $(CORE_PANE_DIR)

$(CORE_PACK_DIR)/build/libcore_pack.a: $(CORE_BASE_DIR)/build/libcore_base.a
	$(MAKE) -C $(CORE_PACK_DIR)

vk-renderer-lib:
	$(MAKE) -C $(VK_RENDERER_DIR)

$(BIN): $(SRC) $(KIT_GRAPH_STRUCT_DIR)/build/libkit_graph_struct.a $(KIT_UI_DIR)/build/libkit_ui.a $(KIT_RENDER_DIR)/build/vk/libkit_render.a $(CORE_MEMDB_DIR)/build/libcore_memdb.a $(CORE_PACK_DIR)/build/libcore_pack.a $(CORE_PANE_DIR)/build/libcore_pane.a $(CORE_KERNEL_DIR)/build/libcore_kernel.a $(CORE_WORKERS_DIR)/build/libcore_workers.a $(CORE_QUEUE_DIR)/build/libcore_queue.a $(CORE_SCHED_DIR)/build/libcore_sched.a $(CORE_JOBS_DIR)/build/libcore_jobs.a $(CORE_WAKE_DIR)/build/libcore_wake.a $(CORE_TIME_DIR)/build/libcore_time.a vk-renderer-lib $(CORE_THEME_DIR)/build/libcore_theme.a $(CORE_FONT_DIR)/build/libcore_font.a $(CORE_BASE_DIR)/build/libcore_base.a | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INC) $(SRC) $(KIT_GRAPH_STRUCT_DIR)/build/libkit_graph_struct.a $(KIT_UI_DIR)/build/libkit_ui.a $(KIT_RENDER_DIR)/build/vk/libkit_render.a $(VK_RENDERER_DIR)/build/lib/libvkrenderer.a $(CORE_MEMDB_DIR)/build/libcore_memdb.a $(CORE_PACK_DIR)/build/libcore_pack.a $(CORE_PANE_DIR)/build/libcore_pane.a $(CORE_KERNEL_DIR)/build/libcore_kernel.a $(CORE_WORKERS_DIR)/build/libcore_workers.a $(CORE_QUEUE_DIR)/build/libcore_queue.a $(CORE_SCHED_DIR)/build/libcore_sched.a $(CORE_JOBS_DIR)/build/libcore_jobs.a $(CORE_WAKE_DIR)/build/libcore_wake.a $(CORE_TIME_DIR)/build/libcore_time.a $(CORE_THEME_DIR)/build/libcore_theme.a $(CORE_FONT_DIR)/build/libcore_font.a $(CORE_BASE_DIR)/build/libcore_base.a $(VULKAN_LIBS) $(SDL_LIBS) $(SDL_TTF_LIBS) $(APPLE_FW) -lm -o $@

RUN_ARGS ?=
REPO_ROOT := ..
DEMO_DB ?= mem_console/demo/demo_mem_console.sqlite

run: $(BIN)
	cd $(REPO_ROOT) && ./mem_console/$(BIN) $(RUN_ARGS)

run-demo: $(BIN)
	cd $(REPO_ROOT) && ./mem_console/$(BIN) --db $(DEMO_DB) $(RUN_ARGS)

clean:
	rm -rf $(OBJ_DIR)
