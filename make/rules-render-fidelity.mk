# Link the target-specific copies used by the normal native application.
RENDER_FIDELITY_BIN := $(TARGET_BUILD_ROOT)/tests/render-fidelity
RENDER_FIDELITY_OUTPUT_DIR ?= $(TARGET_BUILD_ROOT)/render-fidelity
RENDER_FIDELITY_SOURCES := $(KIT_RENDER_DIR)/tests/kit_render_fidelity_vk_test.c $(KIT_RENDER_DIR)/tests/kit_render_fidelity_oracle.c
RENDER_FIDELITY_LIBS := $(KIT_RENDER_LIB) $(VK_RENDERER_LIB) $(VK_RUNTIME_LIB) $(CORE_THEME_LIB) $(CORE_FONT_LIB) $(CORE_BASE_LIB)

$(RENDER_FIDELITY_BIN): $(RENDER_FIDELITY_SOURCES) $(KIT_RENDER_DIR)/tests/kit_render_fidelity_fixture.h $(RENDER_FIDELITY_LIBS)
	mkdir -p "$(dir $@)"
	$(HOST_CC) $(ARCH_FLAGS) $(CFLAGS) $(INC) $(RENDER_FIDELITY_SOURCES) $(RENDER_FIDELITY_LIBS) $(VULKAN_LIBS) $(SDL_LIBS) $(filter-out -lSDL2,$(SDL_TTF_LIBS)) $(APPLE_FW) -lm -o $@

.PHONY: render-fidelity-harness render-fidelity-self-test
render-fidelity-harness: $(RENDER_FIDELITY_BIN)

render-fidelity-self-test: vulkan-rollout-contract render-fidelity-harness
	mkdir -p "$(RENDER_FIDELITY_OUTPUT_DIR)"
	$(if $(filter Darwin,$(shell uname -s)),DYLD_LIBRARY_PATH=$(TARGET_HOMEBREW_PREFIX)/lib,) ./$(RENDER_FIDELITY_BIN) "$(RENDER_FIDELITY_OUTPUT_DIR)"
