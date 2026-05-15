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
	@$(MAKE) BUILD_TOOLCHAIN="$(TEST_TOOLCHAIN)" run-graph-contract-checks

run-headless-smoke: $(BIN)
	@BIN_PATH="$(BIN)" ./tests/run_headless_smoke.sh

run-data-path-contract-checks:
	./tests/run_data_path_contract_checks.sh

run-graph-contract-checks:
	./tests/run_graph_contract_checks.sh

visual-harness: $(BIN)
	@echo "visual harness build gate ready: $(BIN)"
	@echo "launch manual UI validation with: make -C mem_console run-demo"
