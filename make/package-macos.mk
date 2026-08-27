package-build-lane:
	@$(MAKE) BUILD_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(PACKAGE_BIN)"

package-desktop: package-build-lane
	@echo "Preparing desktop package..."
	@rm -rf "$(PACKAGE_APP_DIR)"
	@rm -rf "$(DEPRECATED_PACKAGE_APP_DIR)"
	@mkdir -p "$(PACKAGE_MACOS_DIR)" "$(PACKAGE_RESOURCES_DIR)" "$(PACKAGE_FRAMEWORKS_DIR)"
	@cp "$(PACKAGE_INFO_PLIST_SRC)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier $(PACKAGE_BUNDLE_ID)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Set :CFBundleName $(PACKAGE_DISPLAY_NAME)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Add :CFBundleDisplayName string $(PACKAGE_DISPLAY_NAME)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $(PACKAGE_PROGRAM_VERSION)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $(PACKAGE_PROGRAM_VERSION)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Add :MemConsolePackageProfile string $(PACKAGE_PROFILE)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Add :MemConsoleRuntimeNamespace string $(PACKAGE_RUNTIME_NAMESPACE)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Add :MemConsoleLogNamespace string $(PACKAGE_LOG_NAMESPACE)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Add :MemConsoleBuildLabel string $(PACKAGE_BUILD_LABEL)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@cp "$(PACKAGE_BIN)" "$(PACKAGE_MACOS_DIR)/mem-console-bin"
	@cp "$(PACKAGE_LAUNCHER_SRC)" "$(PACKAGE_MACOS_DIR)/mem-console-launcher"
	@chmod +x "$(PACKAGE_MACOS_DIR)/mem-console-bin" "$(PACKAGE_MACOS_DIR)/mem-console-launcher"
	@PACKAGE_DEP_SEARCH_ROOTS="$(TARGET_DEP_SEARCH_ROOTS)" "$(PACKAGE_DYLIB_BUNDLER)" "$(PACKAGE_MACOS_DIR)/mem-console-bin" "$(PACKAGE_FRAMEWORKS_DIR)"
	@mkdir -p "$(PACKAGE_RESOURCES_DIR)/data"
	@if [ -f "$(PACKAGE_DEFAULT_DB_SRC)" ]; then \
		cp "$(PACKAGE_DEFAULT_DB_SRC)" "$(PACKAGE_RESOURCES_DIR)/data/default.sqlite"; \
	else \
		$(MAKE) -C "$(CORE_MEMDB_DIR)" tools; \
		mkdir -p "$(dir $(PACKAGE_GENERATED_DEFAULT_DB))"; \
		rm -f "$(PACKAGE_GENERATED_DEFAULT_DB)"; \
		"$(CORE_MEMDB_DIR)/build/mem_cli" list --db "$(PACKAGE_GENERATED_DEFAULT_DB)" >/dev/null; \
		cp "$(PACKAGE_GENERATED_DEFAULT_DB)" "$(PACKAGE_RESOURCES_DIR)/data/default.sqlite"; \
		echo "Generated clean package DB seed: $(PACKAGE_GENERATED_DEFAULT_DB)"; \
	fi
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
	@if [ "$(PACKAGE_EMBED_BUILD_IDENTITY)" = "1" ]; then \
		python3 "$(MEW1_TOOL)" write-identity \
			--output "$(PACKAGE_RESOURCES_DIR)/build_identity.json" \
			--source-root "$(CURDIR)" \
			--binary "$(PACKAGE_MACOS_DIR)/mem-console-bin" \
			--profile "$(PACKAGE_PROFILE)" \
			--program mem_console \
			--product eCho \
			--version "$(PACKAGE_PROGRAM_VERSION)" \
			--architecture "$(TARGET_ARCH)" \
			--toolchain "$(PACKAGE_TOOLCHAIN)" \
			--build-label "$(PACKAGE_BUILD_LABEL)"; \
	fi
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" --timestamp=none "$(PACKAGE_APP_DIR)"
	@codesign --verify --deep --strict "$(PACKAGE_APP_DIR)"
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
	@extra_data="$$(find "$(PACKAGE_RESOURCES_DIR)/data" -type f ! -name default.sqlite -print -quit)"; \
	[ -z "$$extra_data" ] || (echo "Unexpected packaged data sidecar: $$extra_data"; exit 1)
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
	@"$(PACKAGE_MACOS_DIR)/mem-console-launcher" --self-test || (echo "package-desktop self-test failed; launcher config follows."; "$(PACKAGE_MACOS_DIR)/mem-console-launcher" --print-config; exit 1)
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

package-desktop-main-edit:
	@test -f "$(MEW1_TOOL)" || (echo "Missing shared MEW1 helper: $(MEW1_TOOL)"; exit 1)
	@before="$$(python3 "$(MEW1_TOOL)" fingerprint --repo "$(CURDIR)")"; \
	$(MAKE) package-desktop-smoke \
		DIST_DIR="$(MAIN_EDIT_DIST_DIR)" \
		PACKAGE_APP_NAME="$(MAIN_EDIT_APP_NAME)" \
		PACKAGE_DISPLAY_NAME="$(MAIN_EDIT_DISPLAY_NAME)" \
		PACKAGE_BUNDLE_ID="$(MAIN_EDIT_BUNDLE_ID)" \
		PACKAGE_PROFILE="$(MAIN_EDIT_PROFILE)" \
		PACKAGE_RUNTIME_NAMESPACE="$(MAIN_EDIT_RUNTIME_NAMESPACE)" \
		PACKAGE_LOG_NAMESPACE="$(MAIN_EDIT_LOG_NAMESPACE)" \
		PACKAGE_BUILD_LABEL="$(MAIN_EDIT_BUILD_LABEL)" \
		PACKAGE_EMBED_BUILD_IDENTITY=1 || exit 1; \
	after="$$(python3 "$(MEW1_TOOL)" fingerprint --repo "$(CURDIR)")"; \
	if [ "$$before" != "$$after" ]; then \
		rm -rf "$(MAIN_EDIT_APP_DIR)"; \
		echo "Source changed during Main Edit packaging; discarded generated package."; \
		exit 1; \
	fi
	@echo "Main Edit desktop package ready: $(MAIN_EDIT_APP_DIR)"

package-desktop-main-edit-self-test: package-desktop-main-edit
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :CFBundleIdentifier' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_BUNDLE_ID)"
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :CFBundleDisplayName' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_DISPLAY_NAME)"
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :MemConsolePackageProfile' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_PROFILE)"
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :MemConsoleRuntimeNamespace' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_RUNTIME_NAMESPACE)"
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :MemConsoleLogNamespace' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_LOG_NAMESPACE)"
	@test -f "$(MAIN_EDIT_APP_DIR)/Contents/Resources/build_identity.json"
	@python3 "$(MEW1_TOOL)" verify-identity \
		--identity "$(MAIN_EDIT_APP_DIR)/Contents/Resources/build_identity.json" \
		--source-root "$(CURDIR)" \
		--binary "$(MAIN_EDIT_APP_DIR)/Contents/MacOS/mem-console-bin" \
		--profile "$(MAIN_EDIT_PROFILE)" \
		--program mem_console \
		--product eCho \
		--version "$(PACKAGE_PROGRAM_VERSION)"
	@set -e; \
	fake_home="$(CURDIR)/$(TARGET_BUILD_ROOT)/package-main-edit-self-test/home"; \
	isolated_runtime="$(CURDIR)/$(TARGET_BUILD_ROOT)/package-main-edit-self-test/runtime"; \
	isolated_logs="$(CURDIR)/$(TARGET_BUILD_ROOT)/package-main-edit-self-test/logs"; \
	rm -rf "$$fake_home" "$$isolated_runtime" "$$isolated_logs"; \
	mkdir -p "$$fake_home" "$$isolated_runtime" "$$isolated_logs"; \
	HOME="$$fake_home" MEM_CONSOLE_RUNTIME_DIR="$$isolated_runtime" MEM_CONSOLE_LOG_DIR="$$isolated_logs" "$(MAIN_EDIT_APP_DIR)/Contents/MacOS/mem-console-launcher" --self-test; \
	config="$$(HOME="$$fake_home" MEM_CONSOLE_RUNTIME_DIR="$$isolated_runtime" MEM_CONSOLE_LOG_DIR="$$isolated_logs" "$(MAIN_EDIT_APP_DIR)/Contents/MacOS/mem-console-launcher" --print-config)"; \
	printf '%s\n' "$$config"; \
	printf '%s\n' "$$config" | grep -Fqx "MEM_CONSOLE_PACKAGE_PROFILE=$(MAIN_EDIT_PROFILE)"; \
	printf '%s\n' "$$config" | grep -Fqx "MEM_CONSOLE_RUNTIME_NAMESPACE=$(MAIN_EDIT_RUNTIME_NAMESPACE)"; \
	printf '%s\n' "$$config" | grep -Fqx "MEM_CONSOLE_LOG_NAMESPACE=$(MAIN_EDIT_LOG_NAMESPACE)"
	@codesign --verify --deep --strict "$(MAIN_EDIT_APP_DIR)"
	@echo "package-desktop-main-edit-self-test passed."

package-desktop-main-edit-refresh: package-desktop-main-edit-self-test
	@test "$(MAIN_EDIT_DESKTOP_APP_DIR)" != "$(DESKTOP_APP_DIR)" || (echo "Refusing canonical Desktop destination"; exit 1)
	@mkdir -p "$(dir $(MAIN_EDIT_PROCESS_RECEIPT))"
	@python3 "$(MEW1_TOOL)" process-audit --match "$(MAIN_EDIT_DISPLAY_NAME)" --path "$(MAIN_EDIT_DESKTOP_APP_DIR)" > "$(MAIN_EDIT_PROCESS_RECEIPT)"
	@if grep -Fq '"running": true' "$(MAIN_EDIT_PROCESS_RECEIPT)"; then \
		echo "Refusing to replace a running $(MAIN_EDIT_APP_NAME); process receipt: $(MAIN_EDIT_PROCESS_RECEIPT)"; \
		exit 1; \
	fi
	@mkdir -p "$$(dirname "$(MAIN_EDIT_DESKTOP_APP_DIR)")"
	@rm -rf "$(MAIN_EDIT_DESKTOP_APP_DIR)"
	@/usr/bin/ditto "$(MAIN_EDIT_APP_DIR)" "$(MAIN_EDIT_DESKTOP_APP_DIR)"
	@echo "Refreshed $(MAIN_EDIT_APP_NAME) at $(MAIN_EDIT_DESKTOP_APP_DIR)"
