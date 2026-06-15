# mem_console Tests

This lane holds deterministic verification checks for `mem_console`.

## Current checks
- `run_headless_smoke.sh`: non-interactive binary smoke path used by:
  - `make -C mem_console test`
  - `make -C mem_console run-headless-smoke`
- `run_data_path_contract_checks.sh`: deterministic S1-S4 data-path contract conformance checks used by:
  - `make -C mem_console test`
  - `make -C mem_console run-data-path-contract-checks`
- `run_graph_contract_checks.sh`: deterministic S5 graph contract conformance checks used by:
  - `make -C mem_console test`
  - `make -C mem_console run-graph-contract-checks`
- `run_detail_relationship_contract_checks.sh`: deterministic MCU1-S2 selected-detail relationship inspector checks used by:
  - `make -C mem_console test`
  - `make -C mem_console run-detail-relationship-contract-checks`
- `mem_console_relationship_mutation_test.c`: temp-DB MCU1-S3 link create/cycle/remove mutation test used by:
  - `make -C mem_console test`
  - `make -C mem_console run-relationship-mutation-test`
- `run_browse_filter_contract_checks.sh`: deterministic MCU1-S4 browse/filter wiring checks used by:
  - `make -C mem_console test`
  - `make -C mem_console run-browse-filter-contract-checks`
