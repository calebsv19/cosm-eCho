# mem_console Current Truth

Last updated: 2026-05-04

## Program Identity
- Repository/program directory: `mem_console`
- Canonical symbol/file prefix: `mem_console`
- Primary private planning bucket:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/memory_console/`

## Current Shipped State
- Lifecycle-wrapper app entry is active with explicit stage handlers and stage-order guardrails.
- Wrapper diagnostics normalization lane is complete and stable.
- Runtime DB/UI/graph/layout lanes are structurally separated and stable.
- Async refresh/runtime pacing is a live contract:
  - worker-backed refresh requests
  - in-flight coalescing for latest intent
  - timed idle waits with redraw-reason scheduling
  - surfaced refresh observability counters in the left pane
- Input routing/invalidation is explicitly split through app-loop helper seams (`intake -> normalize -> route -> invalidate`).
- Graph inspection now includes:
  - one-hop preview with routed orthogonal edges
  - bounded edge-kind filters
  - node-kind filters
  - scope-full project pod overlays
- DB switching/input-root flows are active in-app:
  - `LOAD DB` and `NEW DB` path modal flow
  - discovered `.sqlite` selection from `input_root`
  - app-level startup prefs remain separate from per-DB UI prefs

## Structure
- Required lanes: `docs/`, `src/`, `include/`, `tests/`, `build/`
- Support lanes: `data/`, `demo/`, `tmp/`, `third_party/`, `ide_files/`
- Active subsystems:
  - `src/app`, `src/runtime`, `src/db`, `src/ui`, `src/ui/graph`, `src/layout`
- Recent helper seams added in the live worktree:
  - `src/app/mem_console_app_loop_input.c`
  - `src/runtime/mem_console_runtime_refresh.c`
  - corresponding internal headers for loop/runtime decomposition

## Verification Contract
- Core gates:
  - `make -C mem_console clean && make -C mem_console`
  - `make -C mem_console test`
  - `make -C mem_console run-data-path-contract-checks`
  - `make -C mem_console run-headless-smoke`
  - `make -C mem_console visual-harness`
- Packaging gates:
  - `make -C mem_console package-desktop`
  - `make -C mem_console package-desktop-smoke`
  - `make -C mem_console package-desktop-self-test`
  - `make -C mem_console package-desktop-refresh`
- Release gates:
  - `make -C mem_console release-contract`
  - `make -C mem_console release-bundle-audit`
  - `make -C mem_console release-verify ...`
  - `make -C mem_console release-distribute ...`
  - `make -C mem_console release-desktop-refresh ...`

## Packaging and Launcher Contract
- Standardized package/release target graph is active.
- Launcher diagnostics include `--print-config`, `--self-test`, and startup logfile output.
- Optional icon contract is active via `PACKAGE_APP_ICON_SRC` / `PACKAGE_APP_ICONSET_SRC`.
- Multi-arch Intel packaging lane is complete through local staging + shader-runtime follow-up:
  - target-scoped build/package roots under `build/targets/<target-triple>/...`
  - architecture-tagged release artifacts
  - launcher now seeds real runtime shader copies for Intel retest safety

## Current Boundary
- Maintain lifecycle/diagnostic wrapper stability while continuing graph/UI affordance hardening under the existing verification contract.
- Keep the completed Intel packaging plan archived rather than leaving it in the active private bucket.

## History and Deep Lane References
- Full execution history is in:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/memory_console/`
- This file is the compressed public current-state contract.
