# mem_console Architecture

Last updated: 2026-05-30

## Ownership Model
- `app`: process startup, app orchestration, high-level lifecycle ownership.
- `runtime`: active execution state, refresh pipelines, runtime pacing, refresh metrics publication, and DB picker/path modal ownership.
- `db`: DB-facing read/write/query/filter operations.
- `ui`: pane-level rendering and interface interactions.
- `layout`: pane geometry and layout configuration.

## Dependency Shape
- app coordinates runtime/db/ui/layout lanes.
- runtime and db lanes depend on shared core libraries.
- ui lane depends on shared `kit_render` and `kit_ui`.
- shared libraries are vendored under `third_party/codework_shared/`.
- async runtime lanes additionally depend on shared `core_workers`, `core_queue`, and `core_wake`.

## Build/Verify Contract
- standard local gates:
  - `make -C mem_console`
  - `make -C mem_console test`
  - `make -C mem_console run-data-path-contract-checks`
  - `make -C mem_console run-headless-smoke`
  - `make -C mem_console run-graph-contract-checks`
  - `make -C mem_console visual-harness`
  - `make -C mem_console package-desktop-self-test`

## Lifecycle Structure (Current)
- entrypoint delegates from `src/app/mem_console.c` to `mem_console_app_main(...)`
- stage-owned orchestration is implemented in `src/app/mem_console_app_main.c`
- stage order is guarded through explicit transition checks before each lifecycle phase
- run-loop orchestration lives in `src/app/mem_console_app_loop.c`
- input intake/normalize/route/invalidate helpers live in `src/app/mem_console_app_loop_input.c`
- runtime refresh helper ownership lives in `src/runtime/mem_console_runtime_refresh.c`
- DB picker, path modal text editing, and open/create path-build contract ownership live in `src/runtime/mem_console_state_db_picker.c`
