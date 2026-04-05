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
DIST_DIR := dist
PACKAGE_APP_NAME := eCho.app
PACKAGE_APP_DIR := $(DIST_DIR)/$(PACKAGE_APP_NAME)
PACKAGE_CONTENTS_DIR := $(PACKAGE_APP_DIR)/Contents
PACKAGE_MACOS_DIR := $(PACKAGE_CONTENTS_DIR)/MacOS
PACKAGE_RESOURCES_DIR := $(PACKAGE_CONTENTS_DIR)/Resources
PACKAGE_FRAMEWORKS_DIR := $(PACKAGE_CONTENTS_DIR)/Frameworks
PACKAGE_INFO_PLIST_SRC := tools/packaging/macos/Info.plist
PACKAGE_LAUNCHER_SRC := tools/packaging/macos/mem-console-launcher
PACKAGE_DYLIB_BUNDLER := tools/packaging/macos/bundle-dylibs.sh
DESKTOP_APP_DIR ?= $(HOME)/Desktop/$(PACKAGE_APP_NAME)
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
RELEASE_ARTIFACT_BASENAME := $(RELEASE_PRODUCT_NAME)-$(RELEASE_VERSION)-macOS-$(RELEASE_CHANNEL)
RELEASE_DIR := build/release
RELEASE_APP_ZIP := $(RELEASE_DIR)/$(RELEASE_ARTIFACT_BASENAME).zip
RELEASE_MANIFEST := $(RELEASE_DIR)/$(RELEASE_ARTIFACT_BASENAME).manifest.txt
RELEASE_CODESIGN_IDENTITY ?= $(if $(strip $(APPLE_SIGN_IDENTITY)),$(APPLE_SIGN_IDENTITY),$(PACKAGE_ADHOC_SIGN_IDENTITY))
APPLE_SIGN_IDENTITY ?=
APPLE_NOTARY_PROFILE ?=
APPLE_TEAM_ID ?=
STAPLE_MAX_ATTEMPTS ?= 6
STAPLE_RETRY_DELAY_SEC ?= 15
	SRC = src/app/mem_console.c \
		src/app/mem_console_app_main.c \
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

.PHONY: all clean run run-demo vk-renderer-lib test run-headless-smoke visual-harness package-desktop package-desktop-smoke package-desktop-self-test package-desktop-copy-desktop package-desktop-sync package-desktop-open package-desktop-remove package-desktop-refresh release-contract release-clean release-build release-bundle-audit release-sign release-verify release-verify-signed release-notarize release-staple release-verify-notarized release-artifact release-distribute release-desktop-refresh

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

test: run-headless-smoke

run-headless-smoke: $(BIN)
	./tests/run_headless_smoke.sh

visual-harness: $(BIN)
	@echo "visual harness build gate ready: $(BIN)"
	@echo "launch manual UI validation with: make -C mem_console run-demo"

package-desktop: $(BIN)
	@echo "Preparing desktop package..."
	@rm -rf "$(PACKAGE_APP_DIR)"
	@mkdir -p "$(PACKAGE_MACOS_DIR)" "$(PACKAGE_RESOURCES_DIR)" "$(PACKAGE_FRAMEWORKS_DIR)"
	@cp "$(PACKAGE_INFO_PLIST_SRC)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@cp "$(BIN)" "$(PACKAGE_MACOS_DIR)/mem-console-bin"
	@cp "$(PACKAGE_LAUNCHER_SRC)" "$(PACKAGE_MACOS_DIR)/mem-console-launcher"
	@chmod +x "$(PACKAGE_MACOS_DIR)/mem-console-bin" "$(PACKAGE_MACOS_DIR)/mem-console-launcher"
	@"$(PACKAGE_DYLIB_BUNDLER)" "$(PACKAGE_MACOS_DIR)/mem-console-bin" "$(PACKAGE_FRAMEWORKS_DIR)"
	@if [ -d "data" ]; then cp -R data "$(PACKAGE_RESOURCES_DIR)/"; else mkdir -p "$(PACKAGE_RESOURCES_DIR)/data"; fi
	@mkdir -p "$(PACKAGE_RESOURCES_DIR)/data"
	@mkdir -p "$(PACKAGE_RESOURCES_DIR)/shared/assets/fonts"
	@cp -R "$(SHARED_ROOT)/assets/fonts/." "$(PACKAGE_RESOURCES_DIR)/shared/assets/fonts/"
	@mkdir -p "$(PACKAGE_RESOURCES_DIR)/vk_renderer" "$(PACKAGE_RESOURCES_DIR)/shaders"
	@cp -R "$(VK_RENDERER_DIR)/shaders" "$(PACKAGE_RESOURCES_DIR)/vk_renderer/"
	@cp -R "$(VK_RENDERER_DIR)/shaders/." "$(PACKAGE_RESOURCES_DIR)/shaders/"
	@echo "Desktop package ready: $(PACKAGE_APP_DIR)"

package-desktop-smoke: package-desktop
	@test -x "$(PACKAGE_MACOS_DIR)/mem-console-launcher" || (echo "Missing launcher"; exit 1)
	@test -x "$(PACKAGE_MACOS_DIR)/mem-console-bin" || (echo "Missing app binary"; exit 1)
	@test -f "$(PACKAGE_CONTENTS_DIR)/Info.plist" || (echo "Missing Info.plist"; exit 1)
	@test -f "$(PACKAGE_FRAMEWORKS_DIR)/libvulkan.1.dylib" || (echo "Missing bundled libvulkan"; exit 1)
	@test -f "$(PACKAGE_FRAMEWORKS_DIR)/libMoltenVK.dylib" || (echo "Missing bundled libMoltenVK"; exit 1)
	@test -f "$(PACKAGE_RESOURCES_DIR)/data/default.sqlite" || (echo "Missing default sqlite"; exit 1)
	@test -f "$(PACKAGE_RESOURCES_DIR)/shared/assets/fonts/Montserrat-Regular.ttf" || (echo "Missing shared font"; exit 1)
	@test -f "$(PACKAGE_RESOURCES_DIR)/vk_renderer/shaders/textured.vert.spv" || (echo "Missing bundled vk shader"; exit 1)
	@test -f "$(PACKAGE_RESOURCES_DIR)/shaders/textured.vert.spv" || (echo "Missing bundled runtime shader"; exit 1)
	@echo "package-desktop-smoke passed."

package-desktop-self-test: package-desktop-smoke
	@"$(PACKAGE_MACOS_DIR)/mem-console-launcher" --self-test || (echo "package-desktop self-test failed."; exit 1)
	@echo "package-desktop-self-test passed."

package-desktop-copy-desktop: package-desktop
	@mkdir -p "$(dir $(DESKTOP_APP_DIR))"
	@rm -rf "$(DESKTOP_APP_DIR)"
	@cp -R "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
	@echo "Copied $(PACKAGE_APP_NAME) to $(DESKTOP_APP_DIR)"

package-desktop-sync: package-desktop-copy-desktop
	@echo "Desktop package synchronized: $(DESKTOP_APP_DIR)"

package-desktop-open: package-desktop
	@open "$(PACKAGE_APP_DIR)"

package-desktop-remove:
	@rm -rf "$(DESKTOP_APP_DIR)"
	@echo "Removed desktop app copy: $(DESKTOP_APP_DIR)"

package-desktop-refresh: package-desktop
	@mkdir -p "$(dir $(DESKTOP_APP_DIR))"
	@rm -rf "$(DESKTOP_APP_DIR)"
	@cp -R "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
	@echo "Refreshed $(PACKAGE_APP_NAME) at $(DESKTOP_APP_DIR)"

release-contract:
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
	@$(MAKE) package-desktop-self-test
	@echo "release-build complete."

release-bundle-audit: release-build
	@mkdir -p "$(RELEASE_DIR)"
	@/usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$(PACKAGE_CONTENTS_DIR)/Info.plist" > "$(RELEASE_DIR)/bundle_id.txt"
	@test "$$(cat "$(RELEASE_DIR)/bundle_id.txt")" = "$(RELEASE_BUNDLE_ID)" || (echo "Bundle identifier mismatch"; exit 1)
	@otool -L "$(PACKAGE_MACOS_DIR)/mem-console-bin" > "$(RELEASE_DIR)/otool_mem_console_bin.txt"
	@for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
		[ -f "$$dylib" ] || continue; \
		out="$(RELEASE_DIR)/otool_$$(basename "$$dylib").txt"; \
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
	@for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
		[ -f "$$dylib" ] || continue; \
		codesign --force --timestamp --options runtime --sign "$(RELEASE_CODESIGN_IDENTITY)" "$$dylib"; \
	done
	@codesign --force --timestamp --options runtime --sign "$(RELEASE_CODESIGN_IDENTITY)" "$(PACKAGE_MACOS_DIR)/mem-console-bin"
	@codesign --force --timestamp --options runtime --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" "$(PACKAGE_MACOS_DIR)/mem-console-launcher"
	@codesign --force --timestamp --options runtime --sign "$(RELEASE_CODESIGN_IDENTITY)" "$(PACKAGE_APP_DIR)"
	@echo "release-sign complete."

release-verify: release-sign
	@codesign --verify --deep --strict "$(PACKAGE_APP_DIR)"
	@set +e; spctl_out="$$(spctl --assess --type execute --verbose=2 "$(PACKAGE_APP_DIR)" 2>&1)"; spctl_rc=$$?; set -e; \
	echo "$$spctl_out"; \
	if [ $$spctl_rc -eq 0 ]; then \
		echo "release-verify passed."; \
	elif printf '%s' "$$spctl_out" | rg -q 'source=Unnotarized Developer ID'; then \
		echo "release-verify passed (pre-notary signed state)."; \
	else \
		echo "release-verify failed."; \
		exit $$spctl_rc; \
	fi

release-verify-signed: release-verify
	@echo "release-verify-signed passed."

release-notarize: release-sign
	@test -n "$(APPLE_NOTARY_PROFILE)" || (echo "Missing APPLE_NOTARY_PROFILE"; exit 1)
	@mkdir -p "$(RELEASE_DIR)"
	@ditto -c -k --keepParent "$(PACKAGE_APP_DIR)" "$(RELEASE_APP_ZIP)"
	@xcrun notarytool submit "$(RELEASE_APP_ZIP)" --keychain-profile "$(APPLE_NOTARY_PROFILE)" --wait --output-format json > "$(RELEASE_DIR)/notary_submit.json"
	@rg -q '"status"[[:space:]]*:[[:space:]]*"Accepted"' "$(RELEASE_DIR)/notary_submit.json" || (cat "$(RELEASE_DIR)/notary_submit.json" && echo "Notary submission was not accepted" && exit 1)
	@echo "release-notarize passed."

release-staple:
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

release-artifact: release-verify-notarized
	@mkdir -p "$(RELEASE_DIR)"
	@ditto -c -k --keepParent "$(PACKAGE_APP_DIR)" "$(RELEASE_APP_ZIP)"
	@shasum -a 256 "$(RELEASE_APP_ZIP)" > "$(RELEASE_APP_ZIP).sha256"
	@{ \
		echo "product=$(RELEASE_PRODUCT_NAME)"; \
		echo "program=$(RELEASE_PROGRAM_KEY)"; \
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
	@cp -R "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
	@spctl --assess --type execute --verbose=2 "$(DESKTOP_APP_DIR)"
	@echo "release-desktop-refresh passed."

clean:
	rm -rf $(OBJ_DIR)
