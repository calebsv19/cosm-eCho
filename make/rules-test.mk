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
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-db-mutation-contract-checks
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-state-boundary-contract-checks
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-runtime-refresh-contract-checks
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-graph-contract-checks
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-detail-relationship-contract-checks
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-package-diagnostic-contract-checks
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-demo-helper-safety-contract-checks
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-item-mutation-test
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-relationship-mutation-test
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-browse-filter-contract-checks
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-visual-fixture-contract-checks
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-visual-artifact-contract-checks

run-headless-smoke: $(BIN)
	@BIN_PATH="$(BIN)" ./tests/run_headless_smoke.sh

run-data-path-contract-checks:
	@mkdir -p $(TARGET_BUILD_ROOT)/tests
	@echo "Compiling mem_console path contract test..."
	@$(HOST_CC) $(CFLAGS) $(INC) \
		tests/mem_console_path_contract_test.c \
		src/runtime/mem_console_state_paths.c \
		$(CORE_BASE_DIR)/src/core_base.c \
		-o $(TARGET_BUILD_ROOT)/tests/mem_console_path_contract_test \
		$(SDL_LIBS) $(SDL_TTF_LIBS) $(VULKAN_LIBS) $(APPLE_FW) -lm || (echo "mem_console path contract test compile failed."; exit 1)
	@echo "Running mem_console path contract test..."
	@$(TARGET_BUILD_ROOT)/tests/mem_console_path_contract_test || (echo "mem_console path contract test failed."; exit 1)
	./tests/run_data_path_contract_checks.sh

run-db-mutation-contract-checks:
	@bash ./tests/run_db_mutation_contract_checks.sh

run-state-boundary-contract-checks:
	@mkdir -p $(TARGET_BUILD_ROOT)/tests
	@echo "Compiling mem_console state boundary contract test..."
	@$(HOST_CC) $(CFLAGS) $(INC) \
		tests/mem_console_state_boundary_test.c \
		src/runtime/mem_console_state_roles.c \
		src/app/mem_console_action_roles.c \
		-o $(TARGET_BUILD_ROOT)/tests/mem_console_state_boundary_test \
		-lm || (echo "mem_console state boundary contract test compile failed."; exit 1)
	@echo "Running mem_console state boundary contract test..."
	@$(TARGET_BUILD_ROOT)/tests/mem_console_state_boundary_test || (echo "mem_console state boundary contract test failed."; exit 1)
	@bash ./tests/run_state_boundary_contract_checks.sh

run-runtime-refresh-contract-checks:
	@mkdir -p $(TARGET_BUILD_ROOT)/tests
	@echo "Compiling mem_console runtime refresh intent test..."
	@$(HOST_CC) $(CFLAGS) $(INC) \
		tests/mem_console_runtime_refresh_intent_test.c \
		src/runtime/mem_console_runtime_refresh.c \
		$(CORE_BASE_DIR)/src/core_base.c \
		-o $(TARGET_BUILD_ROOT)/tests/mem_console_runtime_refresh_intent_test \
		-lm || (echo "mem_console runtime refresh intent test compile failed."; exit 1)
	@echo "Running mem_console runtime refresh intent test..."
	@$(TARGET_BUILD_ROOT)/tests/mem_console_runtime_refresh_intent_test || (echo "mem_console runtime refresh intent test failed."; exit 1)
	@bash ./tests/run_runtime_refresh_contract_checks.sh

run-graph-contract-checks:
	@mkdir -p $(TARGET_BUILD_ROOT)/tests
	@echo "Compiling mem_console graph layout model test..."
	@$(HOST_CC) $(CFLAGS) $(INC) \
		tests/mem_console_graph_layout_model_test.c \
		src/ui/graph/mem_console_ui_graph_layout_web.c \
		-o $(TARGET_BUILD_ROOT)/tests/mem_console_graph_layout_model_test \
		-lm || (echo "mem_console graph layout model test compile failed."; exit 1)
	@echo "Running mem_console graph layout model test..."
	@$(TARGET_BUILD_ROOT)/tests/mem_console_graph_layout_model_test || (echo "mem_console graph layout model test failed."; exit 1)
	./tests/run_graph_contract_checks.sh

run-detail-relationship-contract-checks:
	./tests/run_detail_relationship_contract_checks.sh

run-package-diagnostic-contract-checks:
	./tests/run_package_diagnostic_contract_checks.sh

run-demo-helper-safety-contract-checks:
	@$(MAKE) -C $(CORE_MEMDB_DIR) tools
	./tests/run_demo_helper_safety_contract_checks.sh

run-item-mutation-test:
	@mkdir -p $(TARGET_BUILD_ROOT)/tests
	@echo "Compiling mem_console item mutation test..."
	@$(HOST_CC) $(CFLAGS) -DSQLITE_ENABLE_FTS5 $(INC) \
		tests/mem_console_item_mutation_test.c \
		src/db/mem_console_db_mutations.c \
		$(CORE_MEMDB_DIR)/src/core_memdb.c \
		$(CORE_MEMDB_DIR)/external/sqlite3.c \
		$(CORE_BASE_DIR)/src/core_base.c \
		-o $(TARGET_BUILD_ROOT)/tests/mem_console_item_mutation_test \
		$(SDL_LIBS) -lm || (echo "mem_console item mutation test compile failed."; exit 1)
	@echo "Running mem_console item mutation test..."
	@$(TARGET_BUILD_ROOT)/tests/mem_console_item_mutation_test || (echo "mem_console item mutation test failed."; exit 1)

run-relationship-mutation-test:
	@mkdir -p $(TARGET_BUILD_ROOT)/tests
	@echo "Compiling mem_console relationship mutation test..."
	@$(HOST_CC) $(CFLAGS) -DSQLITE_ENABLE_FTS5 $(INC) \
		tests/mem_console_relationship_mutation_test.c \
		src/db/mem_console_db_relationship_mutations.c \
		$(CORE_MEMDB_DIR)/src/core_memdb.c \
		$(CORE_MEMDB_DIR)/external/sqlite3.c \
		$(CORE_BASE_DIR)/src/core_base.c \
		-o $(TARGET_BUILD_ROOT)/tests/mem_console_relationship_mutation_test \
		-lm || (echo "mem_console relationship mutation test compile failed."; exit 1)
	@echo "Running mem_console relationship mutation test..."
	@$(TARGET_BUILD_ROOT)/tests/mem_console_relationship_mutation_test || (echo "mem_console relationship mutation test failed."; exit 1)

run-browse-filter-contract-checks:
	@mkdir -p $(TARGET_BUILD_ROOT)/tests
	@echo "Compiling mem_console browse filter test..."
	@$(HOST_CC) $(CFLAGS) -DSQLITE_ENABLE_FTS5 $(INC) \
		tests/mem_console_browse_filter_test.c \
		src/db/mem_console_db_reads.c \
		src/db/mem_console_db_filters.c \
		$(CORE_MEMDB_DIR)/src/core_memdb.c \
		$(CORE_MEMDB_DIR)/external/sqlite3.c \
		$(CORE_BASE_DIR)/src/core_base.c \
		-o $(TARGET_BUILD_ROOT)/tests/mem_console_browse_filter_test \
		-lm || (echo "mem_console browse filter test compile failed."; exit 1)
	@echo "Running mem_console browse filter test..."
	@$(TARGET_BUILD_ROOT)/tests/mem_console_browse_filter_test || (echo "mem_console browse filter test failed."; exit 1)
	./tests/run_browse_filter_contract_checks.sh

run-visual-fixture-contract-checks:
	./tests/run_visual_fixture_contract_checks.sh

run-visual-artifact-contract-checks: $(BIN)
	./tests/run_visual_artifact_contract_checks.sh

visual-harness: $(BIN)
	@echo "visual harness build gate ready: $(BIN)"
	@echo "launch manual UI validation with: make -C mem_console run-demo"

visual-artifact: $(BIN)
	./demo/render_visual_artifact.sh

visual-fixture-capture: $(BIN)
	./demo/capture_visual_graph_fixture.sh
