# mem_console Architecture

Last updated: 2026-03-27

## Ownership Model
- `app`: process startup, app orchestration, high-level lifecycle ownership.
- `runtime`: active execution state, refresh pipelines, runtime pacing.
- `db`: DB-facing read/write/query/filter operations.
- `ui`: pane-level rendering and interface interactions.
- `layout`: pane geometry and layout configuration.

## Dependency Shape
- app coordinates runtime/db/ui/layout lanes.
- runtime and db lanes depend on shared core libraries.
- ui lane depends on shared `kit_render` and `kit_ui`.
- shared libraries are vendored under `third_party/codework_shared/`.

## Build/Verify Contract
- standard local gates:
  - `make -C mem_console`
  - `make -C mem_console test`
  - `make -C mem_console run-headless-smoke`
  - `make -C mem_console visual-harness`

## Lifecycle Structure (Current)
- entrypoint delegates from `src/app/mem_console.c` to `mem_console_app_main(...)`
- stage-owned orchestration is implemented in `src/app/mem_console_app_main.c`
- stage order is guarded through explicit transition checks before each lifecycle phase
