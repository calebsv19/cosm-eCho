# mem_console Tests

This lane holds deterministic verification checks for `mem_console`.

## Current checks
- `run_headless_smoke.sh`: non-interactive binary smoke path used by:
  - `make -C mem_console test`
  - `make -C mem_console run-headless-smoke`
- `run_data_path_contract_checks.sh`: deterministic S1-S4 data-path contract conformance checks used by:
  - `make -C mem_console test`
  - `make -C mem_console run-data-path-contract-checks`
- `run_db_mutation_contract_checks.sh`: DB mutation boundary checks that keep item flag writes enum-backed and constant-SQL only:
  - `make -C mem_console test`
  - `make -C mem_console run-db-mutation-contract-checks`
- `mem_console_state_boundary_test.c` and `run_state_boundary_contract_checks.sh`: no-UI state-role/render-derivation/action-role boundary checks for left-panel, detail-pane, and graph draw label derivation used by:
  - `make -C mem_console test`
  - `make -C mem_console run-state-boundary-contract-checks`
- `mem_console_runtime_refresh_intent_test.c` and `run_runtime_refresh_contract_checks.sh`: R5-S2 no-UI runtime refresh intent coverage for browse filter capture, intent mismatch detection, and refreshed-state browse field application, plus async refresh ownership checks that keep pending/in-flight refresh intent and latest-error summary publication helper-routed through `MemConsoleRuntime`:
  - `make -C mem_console test`
  - `make -C mem_console run-runtime-refresh-contract-checks`
- `mem_console_graph_layout_model_test.c` and `run_graph_contract_checks.sh`:
  R5-S3 no-UI graph layout model coverage for WEB selected-root centering and
  disconnected component separation, plus deterministic S5 graph contract
  conformance checks, R2-S5 graph viewport ownership guards, and R3-S4 graph
  diagnostic context guards used by:
  - `make -C mem_console test`
  - `make -C mem_console run-graph-contract-checks`
- `run_detail_relationship_contract_checks.sh`: deterministic MCU1-S2 selected-detail relationship inspector checks plus R3-S4 relationship diagnostic context guards used by:
  - `make -C mem_console test`
  - `make -C mem_console run-detail-relationship-contract-checks`
- `run_package_diagnostic_contract_checks.sh`: R3-S5 package/launcher
  diagnostic checks that keep launcher config output shared by `--print-config`
  and `--self-test`, preserve failed-path self-test messages, and keep package
  self-test failure output tied to launcher config readback:
  - `make -C mem_console test`
  - `make -C mem_console run-package-diagnostic-contract-checks`
- `run_demo_helper_safety_contract_checks.sh`: R4-S2 demo/helper DB target
  safety checks that keep destructive demo helpers from defaulting to
  `CODEWORK_MEMDB_PATH` and reject explicit live workspace DB targets unless
  the operator opts in:
  - `make -C mem_console test`
  - `make -C mem_console run-demo-helper-safety-contract-checks`
- `mem_console_item_mutation_test.c`: R4-S3 temp-DB item mutation test proving
  active item writes succeed while archived or stale selected-item writes fail
  closed and do not sync FTS rows:
  - `make -C mem_console test`
  - `make -C mem_console run-item-mutation-test`
- `mem_console_relationship_mutation_test.c`: temp-DB MCU1-S3 link create/cycle/remove mutation test used by:
  - `make -C mem_console test`
  - `make -C mem_console run-relationship-mutation-test`
- `mem_console_browse_filter_test.c` and `run_browse_filter_contract_checks.sh`: R5-S1 temp-DB browse/filter behavior coverage for pinned-only, canonical-only, kind-cycle, and pagination query results, plus deterministic MCU1-S4 source wiring checks, used by:
  - `make -C mem_console test`
  - `make -C mem_console run-browse-filter-contract-checks`
- `run_visual_fixture_contract_checks.sh`: deterministic visual fixture DB
  contract checks plus R5-S4 no-launch visual capture artifact plan checks for
  expected `FOCUS`, `PODS`, and `WEB` screenshot/log/JSON outputs:
  - `make -C mem_console test`
  - `make -C mem_console run-visual-fixture-contract-checks`
- `run_visual_artifact_contract_checks.sh`: R6-S3 source-run first-frame
  artifact contract checks that run `demo/render_visual_artifact.sh` into
  `build/test_visual_artifact_contract`, verify success-line reporting, and
  assert the SVG metadata/content is nonblank and frame-derived:
  - `make -C mem_console test`
  - `make -C mem_console run-visual-artifact-contract-checks`
