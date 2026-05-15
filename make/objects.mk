APP_OBJS := $(patsubst src/%.c,$(OBJ_DIR)/src/%.o,$(APP_SRCS))
APP_HEADERS := $(shell find include src -name '*.h' -type f 2>/dev/null)
CORE_BASE_LIB_SRC := $(CORE_BASE_DIR)/build/libcore_base.a
CORE_THEME_LIB_SRC := $(CORE_THEME_DIR)/build/libcore_theme.a
CORE_FONT_LIB_SRC := $(CORE_FONT_DIR)/build/libcore_font.a
CORE_PACK_LIB_SRC := $(CORE_PACK_DIR)/build/libcore_pack.a
CORE_TIME_LIB_SRC := $(CORE_TIME_DIR)/build/libcore_time.a
CORE_QUEUE_LIB_SRC := $(CORE_QUEUE_DIR)/build/libcore_queue.a
CORE_SCHED_LIB_SRC := $(CORE_SCHED_DIR)/build/libcore_sched.a
CORE_JOBS_LIB_SRC := $(CORE_JOBS_DIR)/build/libcore_jobs.a
CORE_WORKERS_LIB_SRC := $(CORE_WORKERS_DIR)/build/libcore_workers.a
CORE_WAKE_LIB_SRC := $(CORE_WAKE_DIR)/build/libcore_wake.a
CORE_KERNEL_LIB_SRC := $(CORE_KERNEL_DIR)/build/libcore_kernel.a
CORE_MEMDB_LIB_SRC := $(CORE_MEMDB_DIR)/build/libcore_memdb.a
CORE_PANE_LIB_SRC := $(CORE_PANE_DIR)/build/libcore_pane.a
KIT_RENDER_LIB_SRC := $(KIT_RENDER_DIR)/build/vk/libkit_render.a
KIT_UI_LIB_SRC := $(KIT_UI_DIR)/build/libkit_ui.a
KIT_GRAPH_STRUCT_LIB_SRC := $(KIT_GRAPH_STRUCT_DIR)/build/libkit_graph_struct.a
KIT_WORKSPACE_AUTHORING_LIB_SRC := $(KIT_WORKSPACE_AUTHORING_DIR)/build/libkit_workspace_authoring.a
VK_RENDERER_LIB_SRC := $(VK_RENDERER_DIR)/build/lib/libvkrenderer.a

CORE_BASE_LIB := $(SHARED_BUILD_DIR)/libcore_base.a
CORE_THEME_LIB := $(SHARED_BUILD_DIR)/libcore_theme.a
CORE_FONT_LIB := $(SHARED_BUILD_DIR)/libcore_font.a
CORE_PACK_LIB := $(SHARED_BUILD_DIR)/libcore_pack.a
CORE_TIME_LIB := $(SHARED_BUILD_DIR)/libcore_time.a
CORE_QUEUE_LIB := $(SHARED_BUILD_DIR)/libcore_queue.a
CORE_SCHED_LIB := $(SHARED_BUILD_DIR)/libcore_sched.a
CORE_JOBS_LIB := $(SHARED_BUILD_DIR)/libcore_jobs.a
CORE_WORKERS_LIB := $(SHARED_BUILD_DIR)/libcore_workers.a
CORE_WAKE_LIB := $(SHARED_BUILD_DIR)/libcore_wake.a
CORE_KERNEL_LIB := $(SHARED_BUILD_DIR)/libcore_kernel.a
CORE_MEMDB_LIB := $(SHARED_BUILD_DIR)/libcore_memdb.a
CORE_PANE_LIB := $(SHARED_BUILD_DIR)/libcore_pane.a
KIT_RENDER_LIB := $(SHARED_BUILD_DIR)/libkit_render.a
KIT_UI_LIB := $(SHARED_BUILD_DIR)/libkit_ui.a
KIT_GRAPH_STRUCT_LIB := $(SHARED_BUILD_DIR)/libkit_graph_struct.a
KIT_WORKSPACE_AUTHORING_LIB := $(SHARED_BUILD_DIR)/libkit_workspace_authoring.a
VK_RENDERER_LIB := $(SHARED_BUILD_DIR)/libvkrenderer.a

APP_SHARED_LIBS := \
	$(KIT_GRAPH_STRUCT_LIB) \
	$(KIT_WORKSPACE_AUTHORING_LIB) \
	$(KIT_UI_LIB) \
	$(KIT_RENDER_LIB) \
	$(VK_RENDERER_LIB) \
	$(CORE_MEMDB_LIB) \
	$(CORE_PACK_LIB) \
	$(CORE_PANE_LIB) \
	$(CORE_KERNEL_LIB) \
	$(CORE_WORKERS_LIB) \
	$(CORE_QUEUE_LIB) \
	$(CORE_SCHED_LIB) \
	$(CORE_JOBS_LIB) \
	$(CORE_WAKE_LIB) \
	$(CORE_TIME_LIB) \
	$(CORE_THEME_LIB) \
	$(CORE_FONT_LIB) \
	$(CORE_BASE_LIB)

ifeq ($(BUILD_TOOLCHAIN),clang)
APP_CC := $(HOST_CC)
TOOLCHAIN_DEP :=
else ifeq ($(BUILD_TOOLCHAIN),fisics)
APP_CC := $(FISICS_CC)
TOOLCHAIN_DEP := $(FISICS_CC)
else
$(error Unsupported BUILD_TOOLCHAIN '$(BUILD_TOOLCHAIN)'; expected clang or fisics)
endif

COMPILER_STAMP_DIR := $(TOOLCHAIN_BUILD_ROOT)/$(BUILD_TOOLCHAIN)/compiler
COMPILER_STAMP := $(COMPILER_STAMP_DIR)/$(BUILD_TOOLCHAIN).stamp
SHARED_CC := $(HOST_CC) $(ARCH_FLAGS)
