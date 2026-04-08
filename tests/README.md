# mem_console Tests

This lane holds deterministic verification checks for `mem_console`.

## Current checks
- `run_headless_smoke.sh`: non-interactive binary smoke path used by:
  - `make -C mem_console test`
  - `make -C mem_console run-headless-smoke`
- `run_data_path_contract_checks.sh`: deterministic S1-S4 data-path contract conformance checks used by:
  - `make -C mem_console test`
  - `make -C mem_console run-data-path-contract-checks`
