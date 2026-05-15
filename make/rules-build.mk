all: $(BIN)

FORCE:

$(OBJ_DIR) $(BIN_DIR) $(COMPILER_STAMP_DIR):
	mkdir -p $@

$(COMPILER_STAMP): $(TOOLCHAIN_DEP) | $(COMPILER_STAMP_DIR)
	@printf '%s\n' "$(APP_CC)" > "$@"

$(SHARED_BUILD_DIR):
	@mkdir -p "$@"

$(OBJ_DIR)/src/%.o: src/%.c $(APP_HEADERS) $(COMPILER_STAMP) | $(OBJ_DIR)
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

clean:
	rm -rf $(BUILD_ROOT)
