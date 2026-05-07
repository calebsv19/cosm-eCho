HOST_CC ?= cc
FISICS_CC ?= /Users/calebsv/Desktop/CodeWork/fisiCs/fisics
BUILD_TOOLCHAIN ?= clang
PACKAGE_TOOLCHAIN ?= $(BUILD_TOOLCHAIN)
TEST_TOOLCHAIN ?= clang
RELEASE_TOOLCHAIN ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2
PKG_CONFIG ?= pkg-config
TARGET_CONTRACT_HELPER ?= ../bin/desktop_release_target_contract.sh
HOST_ARCH := $(strip $(shell "$(TARGET_CONTRACT_HELPER)" get host_arch))
TARGET_OS_INPUT := $(TARGET_OS)
TARGET_ARCH_INPUT := $(TARGET_ARCH)
TARGET_VARIANT_INPUT := $(TARGET_VARIANT)
TARGET_OS ?= $(strip $(shell TARGET_OS="$(TARGET_OS_INPUT)" TARGET_ARCH="$(TARGET_ARCH_INPUT)" TARGET_VARIANT="$(TARGET_VARIANT_INPUT)" "$(TARGET_CONTRACT_HELPER)" get target_os))
TARGET_ARCH ?= $(strip $(shell TARGET_OS="$(TARGET_OS_INPUT)" TARGET_ARCH="$(TARGET_ARCH_INPUT)" TARGET_VARIANT="$(TARGET_VARIANT_INPUT)" "$(TARGET_CONTRACT_HELPER)" get target_arch))
TARGET_VARIANT ?= $(strip $(shell TARGET_OS="$(TARGET_OS_INPUT)" TARGET_ARCH="$(TARGET_ARCH_INPUT)" TARGET_VARIANT="$(TARGET_VARIANT_INPUT)" "$(TARGET_CONTRACT_HELPER)" get target_variant))
TARGET_TRIPLE := $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get target_triple))
RELEASE_PLATFORM := $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get release_platform))
RELEASE_ARCH := $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get release_arch))
TARGET_HOMEBREW_PREFIX ?= $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get homebrew_prefix))
TARGET_ALT_HOMEBREW_PREFIX ?= $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get alt_homebrew_prefix))
TARGET_PKG_CONFIG_LIBDIR ?= $(TARGET_HOMEBREW_PREFIX)/lib/pkgconfig:$(TARGET_HOMEBREW_PREFIX)/share/pkgconfig
TARGET_DEP_SEARCH_ROOTS ?= $(TARGET_HOMEBREW_PREFIX):$(TARGET_ALT_HOMEBREW_PREFIX)
ARCH_FLAGS := -arch $(TARGET_ARCH)
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
KIT_WORKSPACE_AUTHORING_DIR ?= $(SHARED_ROOT)/kit/kit_workspace_authoring
VK_RENDERER_DIR ?= $(SHARED_ROOT)/vk_renderer

VULKAN_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags vulkan 2>/dev/null)
SDL_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_TTF_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags sdl2_ttf 2>/dev/null)
ifeq ($(strip $(VULKAN_CFLAGS)),)
  VULKAN_CFLAGS := -I$(TARGET_HOMEBREW_PREFIX)/include
endif
ifeq ($(strip $(SDL_CFLAGS)),)
  SDL_CFLAGS := -I$(TARGET_HOMEBREW_PREFIX)/include/SDL2
endif
ifeq ($(strip $(SDL_TTF_CFLAGS)),)
  SDL_TTF_CFLAGS := -I$(TARGET_HOMEBREW_PREFIX)/include/SDL2
endif
VULKAN_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs vulkan 2>/dev/null)
SDL_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs sdl2 2>/dev/null)
SDL_TTF_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs sdl2_ttf 2>/dev/null)
ifeq ($(strip $(VULKAN_LIBS)),)
  VULKAN_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -lvulkan
endif
ifeq ($(strip $(SDL_LIBS)),)
  SDL_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -lSDL2
endif
ifeq ($(strip $(SDL_TTF_LIBS)),)
  SDL_TTF_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -lSDL2_ttf
endif
APPLE_FW := -framework Metal -framework QuartzCore -framework Cocoa -framework IOKit -framework CoreVideo

INC = -Isrc -Iinclude -Iinclude/mem_console -I$(CORE_MEMDB_DIR)/include -I$(CORE_PACK_DIR)/include -I$(CORE_TIME_DIR)/include -I$(CORE_QUEUE_DIR)/include -I$(CORE_SCHED_DIR)/include -I$(CORE_JOBS_DIR)/include -I$(CORE_WORKERS_DIR)/include -I$(CORE_WAKE_DIR)/include -I$(CORE_KERNEL_DIR)/include -I$(CORE_PANE_DIR)/include -I$(KIT_UI_DIR)/include -I$(KIT_GRAPH_STRUCT_DIR)/include -I$(KIT_WORKSPACE_AUTHORING_DIR)/include -I$(KIT_RENDER_DIR)/include -I$(VK_RENDERER_DIR)/include -I$(CORE_BASE_DIR)/include -I$(CORE_THEME_DIR)/include -I$(CORE_FONT_DIR)/include $(VULKAN_CFLAGS) $(SDL_CFLAGS) $(SDL_TTF_CFLAGS)

BUILD_ROOT := build
TARGET_BUILD_ROOT := $(BUILD_ROOT)/targets/$(TARGET_TRIPLE)
SHARED_BUILD_DIR := $(TARGET_BUILD_ROOT)/shared
TOOLCHAIN_BUILD_ROOT := $(TARGET_BUILD_ROOT)/toolchains
OBJ_DIR := $(TOOLCHAIN_BUILD_ROOT)/$(BUILD_TOOLCHAIN)/obj
BIN_DIR := $(TOOLCHAIN_BUILD_ROOT)/$(BUILD_TOOLCHAIN)/bin
BIN := $(BIN_DIR)/mem_console
PACKAGE_BIN := $(TOOLCHAIN_BUILD_ROOT)/$(PACKAGE_TOOLCHAIN)/bin/mem_console
DIST_DIR := $(TARGET_BUILD_ROOT)/dist
PACKAGE_APP_NAME := eCho.app
PACKAGE_APP_DIR := $(DIST_DIR)/$(PACKAGE_APP_NAME)
PACKAGE_CONTENTS_DIR := $(PACKAGE_APP_DIR)/Contents
PACKAGE_MACOS_DIR := $(PACKAGE_CONTENTS_DIR)/MacOS
PACKAGE_RESOURCES_DIR := $(PACKAGE_CONTENTS_DIR)/Resources
PACKAGE_FRAMEWORKS_DIR := $(PACKAGE_CONTENTS_DIR)/Frameworks
PACKAGE_APP_ICON_NAME := AppIcon
PACKAGE_APP_ICON_FILE := $(PACKAGE_APP_ICON_NAME).icns
PACKAGE_LOCAL_ICON_DIR := tools/packaging/macos/local_app_icon
PACKAGE_APP_ICON_SRC ?= $(PACKAGE_LOCAL_ICON_DIR)/$(PACKAGE_APP_ICON_FILE)
PACKAGE_APP_ICONSET_SRC ?= $(PACKAGE_LOCAL_ICON_DIR)/$(PACKAGE_APP_ICON_NAME).iconset
PACKAGE_BUNDLED_ICON_PATH := $(PACKAGE_RESOURCES_DIR)/$(PACKAGE_APP_ICON_FILE)
PACKAGE_INFO_PLIST_SRC := tools/packaging/macos/Info.plist
PACKAGE_LAUNCHER_SRC := tools/packaging/macos/mem-console-launcher
PACKAGE_DYLIB_BUNDLER := tools/packaging/macos/bundle-dylibs.sh
DESKTOP_APP_DIR ?= $(HOME)/Desktop/$(PACKAGE_APP_NAME)
DEPRECATED_APP_NAME := MemConsole.app
DEPRECATED_PACKAGE_APP_DIR := $(DIST_DIR)/$(DEPRECATED_APP_NAME)
DEPRECATED_DESKTOP_APP_DIR := $(HOME)/Desktop/$(DEPRECATED_APP_NAME)
PACKAGE_ADHOC_SIGN_IDENTITY ?= -
RELEASE_VERSION_FILE ?= VERSION
RELEASE_VERSION ?= $(strip $(shell cat "$(RELEASE_VERSION_FILE)" 2>/dev/null))
ifeq ($(RELEASE_VERSION),)
RELEASE_VERSION := 0.1.0
endif
RELEASE_CHANNEL ?= stable
RELEASE_PRODUCT_NAME := eCho
RELEASE_PROGRAM_KEY := mem_console
RELEASE_BUNDLE_ID := com.cosm.echo
RELEASE_ARTIFACT_BASENAME := $(RELEASE_PRODUCT_NAME)-$(RELEASE_VERSION)-$(RELEASE_PLATFORM)-$(RELEASE_ARCH)-$(RELEASE_CHANNEL)
RELEASE_DIR := $(BUILD_ROOT)/release
RELEASE_APP_ZIP := $(RELEASE_DIR)/$(RELEASE_ARTIFACT_BASENAME).zip
RELEASE_MANIFEST := $(RELEASE_DIR)/$(RELEASE_ARTIFACT_BASENAME).manifest.txt
RELEASE_CODESIGN_IDENTITY ?= $(if $(strip $(APPLE_SIGN_IDENTITY)),$(APPLE_SIGN_IDENTITY),$(PACKAGE_ADHOC_SIGN_IDENTITY))
APPLE_SIGN_IDENTITY ?=
APPLE_NOTARY_PROFILE ?=
APPLE_TEAM_ID ?=
STAPLE_MAX_ATTEMPTS ?= 6
STAPLE_RETRY_DELAY_SEC ?= 15
APP_SRCS := src/app/mem_console.c \
	src/app/mem_console_app_main.c \
	src/app/mem_console_app_actions.c \
	src/app/mem_console_app_db_switch.c \
	src/app/mem_console_app_events.c \
	src/app/mem_console_app_loop.c \
	src/app/mem_console_app_loop_input.c \
	src/app/mem_console_app_theme.c \
	src/app/mem_console_workspace_authoring_host.c \
	src/app/mem_console_kernel_bridge.c \
	src/db/mem_console_db.c \
	src/db/mem_console_db_graph_sort.c \
	src/db/mem_console_db_filters.c \
	src/db/mem_console_db_mutations.c \
	src/db/mem_console_db_reads.c \
	src/runtime/mem_console_prefs.c \
	src/runtime/mem_console_prefs_app_io.c \
	src/runtime/mem_console_runtime.c \
	src/runtime/mem_console_runtime_refresh.c \
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
	src/ui/mem_console_workspace_authoring_overlay.c \
	src/ui/mem_console_ui_left_panel.c \
	src/ui/mem_console_ui_left_section.c \
	src/ui/graph/mem_console_ui_graph.c \
	src/ui/graph/mem_console_ui_graph_camera.c \
	src/ui/graph/mem_console_ui_graph_controls.c \
	src/ui/graph/mem_console_ui_graph_draw.c \
	src/ui/graph/mem_console_ui_graph_geometry.c \
	src/ui/graph/mem_console_ui_graph_hud.c \
	src/ui/graph/mem_console_ui_graph_layout.c \
	src/ui/graph/mem_console_ui_graph_layout_focus_helpers.c \
	src/ui/graph/mem_console_ui_graph_overlay.c \
	src/ui/graph/mem_console_ui_graph_project_pods.c \
	src/ui/graph/mem_console_ui_graph_types.c \
	src/ui/graph/mem_console_ui_graph_panel.c

APP_OBJS := $(patsubst src/%.c,$(OBJ_DIR)/src/%.o,$(APP_SRCS))
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

.PHONY: all clean run run-demo vk-renderer-lib test run-headless-smoke run-data-path-contract-checks visual-harness package-build-lane package-desktop package-desktop-smoke package-desktop-self-test package-desktop-copy-desktop package-desktop-sync package-desktop-open package-desktop-remove package-desktop-refresh release-contract release-clean release-build release-bundle-audit release-sign release-verify release-verify-signed release-notarize release-staple release-verify-notarized release-artifact release-distribute release-desktop-refresh FORCE

all: $(BIN)

FORCE:

$(OBJ_DIR) $(BIN_DIR) $(COMPILER_STAMP_DIR):
	mkdir -p $@

$(COMPILER_STAMP): $(TOOLCHAIN_DEP) | $(COMPILER_STAMP_DIR)
	@printf '%s\n' "$(APP_CC)" > "$@"

$(SHARED_BUILD_DIR):
	@mkdir -p "$@"

$(OBJ_DIR)/src/%.o: src/%.c $(COMPILER_STAMP) | $(OBJ_DIR)
	@mkdir -p "$(dir $@)"
	$(APP_CC) $(CFLAGS) $(INC) $(if $(filter clang,$(BUILD_TOOLCHAIN)),$(ARCH_FLAGS),) -c "$<" -o "$@"

define build_copy_static_lib
$($(1)_LIB): FORCE $(3) | $(SHARED_BUILD_DIR)
	$(MAKE) -C $($(1)_DIR) clean $(2)
	PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" PKG_CONFIG="$(PKG_CONFIG)" $(MAKE) -C $($(1)_DIR) CC="$(SHARED_CC)" $(2)
	cp "$$($(1)_LIB_SRC)" "$$@"
endef

$(eval $(call build_copy_static_lib,KIT_RENDER,KIT_RENDER_ENABLE_VK=1,))
$(eval $(call build_copy_static_lib,KIT_UI,KIT_RENDER_ENABLE_VK=1,$(KIT_RENDER_LIB)))
$(eval $(call build_copy_static_lib,KIT_GRAPH_STRUCT,KIT_RENDER_ENABLE_VK=1,$(KIT_UI_LIB)))
$(eval $(call build_copy_static_lib,KIT_WORKSPACE_AUTHORING,,$(CORE_BASE_LIB) $(CORE_THEME_LIB) $(CORE_FONT_LIB) $(CORE_PANE_LIB) $(KIT_RENDER_LIB)))
$(eval $(call build_copy_static_lib,CORE_BASE,,))
$(eval $(call build_copy_static_lib,CORE_THEME,,$(CORE_BASE_LIB)))
$(eval $(call build_copy_static_lib,CORE_FONT,,$(CORE_BASE_LIB)))
$(eval $(call build_copy_static_lib,CORE_QUEUE,,))
$(eval $(call build_copy_static_lib,CORE_TIME,,))
$(eval $(call build_copy_static_lib,CORE_SCHED,,))
$(eval $(call build_copy_static_lib,CORE_JOBS,,))
$(eval $(call build_copy_static_lib,CORE_WORKERS,,$(CORE_QUEUE_LIB)))
$(eval $(call build_copy_static_lib,CORE_WAKE,,))
$(eval $(call build_copy_static_lib,CORE_KERNEL,,$(CORE_TIME_LIB) $(CORE_SCHED_LIB) $(CORE_JOBS_LIB) $(CORE_WAKE_LIB) $(CORE_QUEUE_LIB)))
$(eval $(call build_copy_static_lib,CORE_MEMDB,,$(CORE_BASE_LIB)))
$(eval $(call build_copy_static_lib,CORE_PANE,,))
$(eval $(call build_copy_static_lib,CORE_PACK,,$(CORE_BASE_LIB)))
$(eval $(call build_copy_static_lib,VK_RENDERER,,))

$(BIN): $(APP_OBJS) $(APP_SHARED_LIBS) | $(BIN_DIR)
	$(HOST_CC) $(ARCH_FLAGS) $(CFLAGS) $(INC) $(APP_OBJS) $(APP_SHARED_LIBS) $(VULKAN_LIBS) $(SDL_LIBS) $(SDL_TTF_LIBS) $(APPLE_FW) -lm -o $@

RUN_ARGS ?=
REPO_ROOT := ..
DEMO_DB ?= mem_console/demo/demo_mem_console.sqlite

run: $(BIN)
	cd $(REPO_ROOT) && ./mem_console/$(BIN) $(RUN_ARGS)

run-demo: $(BIN)
	cd $(REPO_ROOT) && ./mem_console/$(BIN) --db $(DEMO_DB) $(RUN_ARGS)

test:
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-headless-smoke
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-data-path-contract-checks

run-headless-smoke: $(BIN)
	@BIN_PATH="$(BIN)" ./tests/run_headless_smoke.sh

run-data-path-contract-checks:
	./tests/run_data_path_contract_checks.sh

visual-harness: $(BIN)
	@echo "visual harness build gate ready: $(BIN)"
	@echo "launch manual UI validation with: make -C mem_console run-demo"

package-build-lane:
	@$(MAKE) BUILD_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(PACKAGE_BIN)"

package-desktop: package-build-lane
	@echo "Preparing desktop package..."
	@rm -rf "$(PACKAGE_APP_DIR)"
	@rm -rf "$(DEPRECATED_PACKAGE_APP_DIR)"
	@mkdir -p "$(PACKAGE_MACOS_DIR)" "$(PACKAGE_RESOURCES_DIR)" "$(PACKAGE_FRAMEWORKS_DIR)"
	@cp "$(PACKAGE_INFO_PLIST_SRC)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@cp "$(PACKAGE_BIN)" "$(PACKAGE_MACOS_DIR)/mem-console-bin"
	@cp "$(PACKAGE_LAUNCHER_SRC)" "$(PACKAGE_MACOS_DIR)/mem-console-launcher"
	@chmod +x "$(PACKAGE_MACOS_DIR)/mem-console-bin" "$(PACKAGE_MACOS_DIR)/mem-console-launcher"
	@PACKAGE_DEP_SEARCH_ROOTS="$(TARGET_DEP_SEARCH_ROOTS)" "$(PACKAGE_DYLIB_BUNDLER)" "$(PACKAGE_MACOS_DIR)/mem-console-bin" "$(PACKAGE_FRAMEWORKS_DIR)"
	@if [ -d "data" ]; then cp -R data "$(PACKAGE_RESOURCES_DIR)/"; else mkdir -p "$(PACKAGE_RESOURCES_DIR)/data"; fi
	@mkdir -p "$(PACKAGE_RESOURCES_DIR)/data"
	@mkdir -p "$(PACKAGE_RESOURCES_DIR)/shared/assets/fonts"
	@cp -R "$(SHARED_ROOT)/assets/fonts/." "$(PACKAGE_RESOURCES_DIR)/shared/assets/fonts/"
	@if [ -f "$(PACKAGE_APP_ICON_SRC)" ]; then \
		cp "$(PACKAGE_APP_ICON_SRC)" "$(PACKAGE_BUNDLED_ICON_PATH)"; \
		echo "Bundled app icon from $(PACKAGE_APP_ICON_SRC)"; \
	elif [ -d "$(PACKAGE_APP_ICONSET_SRC)" ]; then \
		iconutil -c icns "$(PACKAGE_APP_ICONSET_SRC)" -o "$(PACKAGE_BUNDLED_ICON_PATH)"; \
		echo "Bundled app icon from $(PACKAGE_APP_ICONSET_SRC)"; \
	else \
		echo "Warning: no app icon input found; continuing without bundled AppIcon.icns"; \
	fi
	@mkdir -p "$(PACKAGE_RESOURCES_DIR)/vk_renderer" "$(PACKAGE_RESOURCES_DIR)/shaders"
	@cp -R "$(VK_RENDERER_DIR)/shaders" "$(PACKAGE_RESOURCES_DIR)/vk_renderer/"
	@cp -R "$(VK_RENDERER_DIR)/shaders/." "$(PACKAGE_RESOURCES_DIR)/shaders/"
	@for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
		[ -f "$$dylib" ] || continue; \
		codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" --timestamp=none "$$dylib"; \
	done
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" --timestamp=none "$(PACKAGE_MACOS_DIR)/mem-console-bin"
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" --timestamp=none "$(PACKAGE_MACOS_DIR)/mem-console-launcher"
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" --timestamp=none "$(PACKAGE_APP_DIR)"
	@echo "Desktop package ready: $(PACKAGE_APP_DIR)"

package-desktop-smoke: package-desktop
	@test -x "$(PACKAGE_MACOS_DIR)/mem-console-launcher" || (echo "Missing launcher"; exit 1)
	@test -x "$(PACKAGE_MACOS_DIR)/mem-console-bin" || (echo "Missing app binary"; exit 1)
	@test -f "$(PACKAGE_CONTENTS_DIR)/Info.plist" || (echo "Missing Info.plist"; exit 1)
	@test -f "$(PACKAGE_FRAMEWORKS_DIR)/libvulkan.1.dylib" || (echo "Missing bundled libvulkan"; exit 1)
	@test -f "$(PACKAGE_FRAMEWORKS_DIR)/libMoltenVK.dylib" || (echo "Missing bundled libMoltenVK"; exit 1)
	@if [ -f "$(PACKAGE_APP_ICON_SRC)" ] || [ -d "$(PACKAGE_APP_ICONSET_SRC)" ]; then \
		test -f "$(PACKAGE_BUNDLED_ICON_PATH)" || (echo "Missing bundled AppIcon.icns"; exit 1); \
	fi
	@test -f "$(PACKAGE_RESOURCES_DIR)/data/default.sqlite" || (echo "Missing default sqlite"; exit 1)
	@test -f "$(PACKAGE_RESOURCES_DIR)/shared/assets/fonts/Montserrat-Regular.ttf" || (echo "Missing shared font"; exit 1)
	@test -f "$(PACKAGE_RESOURCES_DIR)/vk_renderer/shaders/textured.vert.spv" || (echo "Missing bundled vk shader"; exit 1)
	@test -f "$(PACKAGE_RESOURCES_DIR)/shaders/textured.vert.spv" || (echo "Missing bundled runtime shader"; exit 1)
	@actual_archs="$$(/usr/bin/lipo -archs "$(PACKAGE_MACOS_DIR)/mem-console-bin" 2>/dev/null || true)"; \
	printf '%s\n' "$$actual_archs" | /usr/bin/grep -qw "$(TARGET_ARCH)" || (echo "Unexpected app binary archs: $$actual_archs"; exit 1)
	@for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
		[ -f "$$dylib" ] || continue; \
		dylib_archs="$$(/usr/bin/lipo -archs "$$dylib" 2>/dev/null || true)"; \
		printf '%s\n' "$$dylib_archs" | /usr/bin/grep -qw "$(TARGET_ARCH)" || (echo "Unexpected dylib archs for $$dylib: $$dylib_archs"; exit 1); \
	done
	@echo "package-desktop-smoke passed."

package-desktop-self-test: package-desktop-smoke
	@"$(PACKAGE_MACOS_DIR)/mem-console-launcher" --self-test || (echo "package-desktop self-test failed."; exit 1)
	@echo "package-desktop-self-test passed."

package-desktop-copy-desktop: package-desktop
	@mkdir -p "$(dir $(DESKTOP_APP_DIR))"
	@rm -rf "$(DESKTOP_APP_DIR)"
	@rm -rf "$(DEPRECATED_DESKTOP_APP_DIR)"
	@/usr/bin/ditto "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
	@echo "Copied $(PACKAGE_APP_NAME) to $(DESKTOP_APP_DIR)"

package-desktop-sync: package-desktop-copy-desktop
	@echo "Desktop package synchronized: $(DESKTOP_APP_DIR)"

package-desktop-open: package-desktop
	@open "$(PACKAGE_APP_DIR)"

package-desktop-remove:
	@rm -rf "$(DESKTOP_APP_DIR)"
	@rm -rf "$(DEPRECATED_DESKTOP_APP_DIR)"
	@echo "Removed desktop app copy: $(DESKTOP_APP_DIR)"

package-desktop-refresh: package-desktop
	@mkdir -p "$(dir $(DESKTOP_APP_DIR))"
	@rm -rf "$(DESKTOP_APP_DIR)"
	@rm -rf "$(DEPRECATED_DESKTOP_APP_DIR)"
	@/usr/bin/ditto "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
	@echo "Refreshed $(PACKAGE_APP_NAME) at $(DESKTOP_APP_DIR)"

release-contract:
	@echo "HOST_ARCH=$(HOST_ARCH)"
	@echo "TARGET_OS=$(TARGET_OS)"
	@echo "TARGET_ARCH=$(TARGET_ARCH)"
	@echo "TARGET_VARIANT=$(TARGET_VARIANT)"
	@echo "TARGET_TRIPLE=$(TARGET_TRIPLE)"
	@echo "RELEASE_PLATFORM=$(RELEASE_PLATFORM)"
	@echo "RELEASE_ARCH=$(RELEASE_ARCH)"
	@echo "TARGET_HOMEBREW_PREFIX=$(TARGET_HOMEBREW_PREFIX)"
	@echo "TARGET_PKG_CONFIG_LIBDIR=$(TARGET_PKG_CONFIG_LIBDIR)"
	@echo "RELEASE_PROGRAM_KEY=$(RELEASE_PROGRAM_KEY)"
	@echo "RELEASE_PRODUCT_NAME=$(RELEASE_PRODUCT_NAME)"
	@echo "RELEASE_BUNDLE_ID=$(RELEASE_BUNDLE_ID)"
	@echo "RELEASE_VERSION=$(RELEASE_VERSION)"
	@echo "RELEASE_CHANNEL=$(RELEASE_CHANNEL)"
	@test "$(RELEASE_PRODUCT_NAME)" = "eCho" || (echo "Unexpected release product"; exit 1)
	@test "$(RELEASE_PROGRAM_KEY)" = "mem_console" || (echo "Unexpected release program key"; exit 1)
	@test "$(RELEASE_BUNDLE_ID)" = "com.cosm.echo" || (echo "Unexpected release bundle id"; exit 1)
	@test -f "$(RELEASE_VERSION_FILE)" || (echo "Missing VERSION file"; exit 1)
	@echo "release-contract passed."

release-clean:
	@rm -rf "$(RELEASE_DIR)"
	@echo "release-clean complete."

release-build:
	@$(MAKE) BUILD_TOOLCHAIN="$(RELEASE_TOOLCHAIN)" PACKAGE_TOOLCHAIN="$(RELEASE_TOOLCHAIN)" TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" package-desktop-self-test
	@echo "release-build complete."

release-bundle-audit: release-build
	@mkdir -p "$(RELEASE_DIR)"
	@/usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$(PACKAGE_CONTENTS_DIR)/Info.plist" > "$(RELEASE_DIR)/bundle_id.txt"
	@test "$$(cat "$(RELEASE_DIR)/bundle_id.txt")" = "$(RELEASE_BUNDLE_ID)" || (echo "Bundle identifier mismatch"; exit 1)
	@binary_archs="$$(/usr/bin/lipo -archs "$(PACKAGE_MACOS_DIR)/mem-console-bin" 2>/dev/null || true)"; \
		printf '%s\n' "$$binary_archs" | /usr/bin/grep -qw "$(TARGET_ARCH)" || (echo "Unexpected release binary archs: $$binary_archs"; exit 1); \
		printf '%s\n' "$$binary_archs" > "$(RELEASE_DIR)/lipo_mem_console_bin.txt"
	@otool -L "$(PACKAGE_MACOS_DIR)/mem-console-bin" > "$(RELEASE_DIR)/otool_mem_console_bin.txt"
	@for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
		[ -f "$$dylib" ] || continue; \
		out="$(RELEASE_DIR)/otool_$$(basename "$$dylib").txt"; \
		arch_out="$(RELEASE_DIR)/lipo_$$(basename "$$dylib").txt"; \
		dylib_archs="$$(/usr/bin/lipo -archs "$$dylib" 2>/dev/null || true)"; \
		printf '%s\n' "$$dylib_archs" | /usr/bin/grep -qw "$(TARGET_ARCH)" || (echo "Unexpected release dylib archs for $$dylib: $$dylib_archs"; exit 1); \
		printf '%s\n' "$$dylib_archs" > "$$arch_out"; \
		otool -L "$$dylib" > "$$out"; \
	done
	@! rg -q '/opt/homebrew|/usr/local|/Users/' "$(RELEASE_DIR)"/otool_*.txt || (echo "Found non-portable dylib linkage"; exit 1)
	@! rg -q '@rpath/' "$(RELEASE_DIR)"/otool_*.txt || (echo "Found unresolved @rpath dylib linkage"; exit 1)
	@"$(PACKAGE_MACOS_DIR)/mem-console-launcher" --print-config > "$(RELEASE_DIR)/print_config.txt"
	@rg -q '^MEM_CONSOLE_RUNTIME_DIR=' "$(RELEASE_DIR)/print_config.txt" || (echo "Missing MEM_CONSOLE_RUNTIME_DIR in launcher config"; exit 1)
	@rg -q '^VK_ICD_FILENAMES=' "$(RELEASE_DIR)/print_config.txt" || (echo "Missing VK_ICD_FILENAMES in launcher config"; exit 1)
	@echo "release-bundle-audit passed."

release-sign: release-bundle-audit
	@test -n "$(RELEASE_CODESIGN_IDENTITY)" || (echo "Missing signing identity"; exit 1)
	@echo "Signing with identity: $(RELEASE_CODESIGN_IDENTITY)"
	@if [ "$(RELEASE_CODESIGN_IDENTITY)" = "-" ]; then \
		for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
			[ -f "$$dylib" ] || continue; \
			codesign --force --sign "$(RELEASE_CODESIGN_IDENTITY)" --timestamp=none "$$dylib"; \
		done; \
		codesign --force --sign "$(RELEASE_CODESIGN_IDENTITY)" --timestamp=none "$(PACKAGE_MACOS_DIR)/mem-console-bin"; \
		codesign --force --sign "$(RELEASE_CODESIGN_IDENTITY)" --timestamp=none "$(PACKAGE_MACOS_DIR)/mem-console-launcher"; \
		codesign --force --sign "$(RELEASE_CODESIGN_IDENTITY)" --timestamp=none "$(PACKAGE_APP_DIR)"; \
	else \
		for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
			[ -f "$$dylib" ] || continue; \
			codesign --force --timestamp --options runtime --sign "$(RELEASE_CODESIGN_IDENTITY)" "$$dylib"; \
		done; \
		codesign --force --timestamp --options runtime --sign "$(RELEASE_CODESIGN_IDENTITY)" "$(PACKAGE_MACOS_DIR)/mem-console-bin"; \
		codesign --force --timestamp --options runtime --sign "$(RELEASE_CODESIGN_IDENTITY)" "$(PACKAGE_MACOS_DIR)/mem-console-launcher"; \
		codesign --force --timestamp --options runtime --sign "$(RELEASE_CODESIGN_IDENTITY)" "$(PACKAGE_APP_DIR)"; \
	fi
	@echo "release-sign complete."

release-verify: release-sign
	@codesign --verify --deep --strict "$(PACKAGE_APP_DIR)"
	@if [ "$(RELEASE_CODESIGN_IDENTITY)" = "-" ]; then \
		echo "release-verify note: ad-hoc identity in use; skipping spctl Gatekeeper assessment"; \
	else \
		set +e; spctl_out="$$(spctl --assess --type execute --verbose=2 "$(PACKAGE_APP_DIR)" 2>&1)"; spctl_rc=$$?; set -e; \
		echo "$$spctl_out"; \
		if [ $$spctl_rc -eq 0 ]; then \
			:; \
		elif printf '%s' "$$spctl_out" | rg -q 'source=Unnotarized Developer ID'; then \
			echo "release-verify passed (pre-notary signed state)."; \
		else \
			echo "release-verify failed."; \
			exit $$spctl_rc; \
		fi; \
	fi
	@echo "release-verify passed."

release-verify-signed: release-verify
	@echo "release-verify-signed passed."

release-notarize: release-sign
	@test -n "$(APPLE_NOTARY_PROFILE)" || (echo "Missing APPLE_NOTARY_PROFILE"; exit 1)
	@mkdir -p "$(RELEASE_DIR)"
	@ditto -c -k --keepParent "$(PACKAGE_APP_DIR)" "$(RELEASE_APP_ZIP)"
	@xcrun notarytool submit "$(RELEASE_APP_ZIP)" --keychain-profile "$(APPLE_NOTARY_PROFILE)" --wait --output-format json > "$(RELEASE_DIR)/notary_submit.json"
	@rg -q '"status"[[:space:]]*:[[:space:]]*"Accepted"' "$(RELEASE_DIR)/notary_submit.json" || (cat "$(RELEASE_DIR)/notary_submit.json" && echo "Notary submission was not accepted" && exit 1)
	@echo "release-notarize passed."

release-staple: release-notarize
	@attempt=1; \
	while [ $$attempt -le $(STAPLE_MAX_ATTEMPTS) ]; do \
		if xcrun stapler staple "$(PACKAGE_APP_DIR)" && xcrun stapler validate "$(PACKAGE_APP_DIR)"; then \
			echo "release-staple passed."; \
			exit 0; \
		fi; \
		echo "staple attempt $$attempt/$(STAPLE_MAX_ATTEMPTS) failed; retrying in $(STAPLE_RETRY_DELAY_SEC)s"; \
		sleep $(STAPLE_RETRY_DELAY_SEC); \
		attempt=$$((attempt+1)); \
	done; \
	echo "release-staple failed."; \
	exit 1

release-verify-notarized: release-staple
	@spctl --assess --type execute --verbose=2 "$(PACKAGE_APP_DIR)"
	@xcrun stapler validate "$(PACKAGE_APP_DIR)"
	@echo "release-verify-notarized passed."

release-artifact: release-verify
	@mkdir -p "$(RELEASE_DIR)"
	@ditto -c -k --keepParent "$(PACKAGE_APP_DIR)" "$(RELEASE_APP_ZIP)"
	@shasum -a 256 "$(RELEASE_APP_ZIP)" > "$(RELEASE_APP_ZIP).sha256"
	@{ \
		echo "product=$(RELEASE_PRODUCT_NAME)"; \
		echo "program=$(RELEASE_PROGRAM_KEY)"; \
		echo "host_arch=$(HOST_ARCH)"; \
		echo "target_os=$(TARGET_OS)"; \
		echo "target_arch=$(TARGET_ARCH)"; \
		echo "target_variant=$(TARGET_VARIANT)"; \
		echo "target_triple=$(TARGET_TRIPLE)"; \
		echo "release_platform=$(RELEASE_PLATFORM)"; \
		echo "release_arch=$(RELEASE_ARCH)"; \
		echo "version=$(RELEASE_VERSION)"; \
		echo "channel=$(RELEASE_CHANNEL)"; \
		echo "bundle_id=$(RELEASE_BUNDLE_ID)"; \
		echo "zip=$(RELEASE_APP_ZIP)"; \
		echo "sha256=$$(cut -d' ' -f1 "$(RELEASE_APP_ZIP).sha256")"; \
	} > "$(RELEASE_MANIFEST)"
	@echo "release-artifact complete: $(RELEASE_APP_ZIP)"

release-distribute: release-artifact
	@echo "release-distribute passed."

release-desktop-refresh: release-distribute
	@mkdir -p "$$(dirname "$(DESKTOP_APP_DIR)")"
	@rm -rf "$(DESKTOP_APP_DIR)"
	@rm -rf "$(DEPRECATED_DESKTOP_APP_DIR)"
	@cp -R "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
	@spctl --assess --type execute --verbose=2 "$(DESKTOP_APP_DIR)"
	@echo "release-desktop-refresh passed."

clean:
	rm -rf $(BUILD_ROOT)
