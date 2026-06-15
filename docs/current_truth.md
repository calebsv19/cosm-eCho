# eCho Current Truth

Last updated: 2026-06-14

## Program Identity
- Product name: `eCho`
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
  - `FOCUS` mode with stronger selected-root composition, ranked hop-1
    neighbors, earlier emphasized labels, and de-emphasized non-selected edges
  - `WEB` mode now uses a dedicated topology helper that separates visible
    connected components into islands and ranks bridge/high-degree
    cross-project nodes toward component centers
- Selected-memory detail now includes a relationship inspector:
  - bounded `mem_link` rows load beside the selected title/body
  - inbound/outbound groups are separated by link kind
  - rows show neighbor id, project, kind, and title
  - row clicks select the neighbor through the shared list/graph navigation
    transition and refresh graph/detail
  - a compact target-id input can add a selected-memory outgoing `related`
    link to an active target memory
  - row-scoped `KIND` and `DEL` controls change or remove only links that
    touch the selected memory
- Left browse is now a faceted investigation path:
  - search and project filters remain the base query controls
  - pinned-only, canonical-only, and kind-cycle facets narrow the list without
    changing the DB model
  - matching count and visible list windows both honor the same browse facets
  - async refresh captures browse facets as part of request intent so stale
    unfiltered results are not applied after a facet change
  - result rows show id, pinned/canonical flags, project, kind, compact updated
    time, and title in a stable scan order
- DB switching/input-root flows are active in-app:
  - `LOAD DB` and `NEW DB` path modal flow
  - discovered `.sqlite` selection from `input_root`
  - app-level startup prefs remain separate from per-DB UI prefs
- Workspace Authoring is active through shared `kit_workspace_authoring`:
  - normal runtime has no persistent authoring HUD
  - `Alt+C` then `Alt+V` toggles active authoring
  - active authoring captures reserved input
  - `Tab` cycles pane overlay and full-screen Font/Theme overlay
  - shared overlay button geometry and shared Font/Theme layout/hit actions are used
  - Font/Theme/text-size previews are live
  - `Enter` applies; `Esc` or toggle-out cancels and restores the entry baseline
  - accepted changes persist through the existing per-DB `.ui.pack`

## Runtime and Data Path Contract
- Active DB startup resolution is explicit:
  1. `CODEWORK_MEMDB_PATH` when set
  2. last-used DB path from app prefs under `<output_root>/mem_console.app.pack`
  3. fallback default DB at
     `~/Library/Application Support/MemConsole/runtime/default.sqlite` on
     macOS or `mem_console/data/default.sqlite` otherwise
- Path roots are normalized together:
  - `output_root` prefers the mutable app-data/runtime root when available
  - `input_root` falls back to the active DB parent when no explicit
    input-root hint survives normalization
- In-session `LOAD DB` and `NEW DB` keep the same contract:
  - `LOAD DB` uses the entered path directly
  - `NEW DB` creates a bare-name target under `input_root` and uses explicit
    paths directly
- App-level startup prefs stay separate from per-DB UI prefs:
  - app prefs default to `<output_root>/mem_console.app.pack`
  - per-DB UI prefs persist in `<db_path>.ui.pack`

## Structure
- Required lanes: `docs/`, `src/`, `include/`, `tests/`, `build/`
- Support lanes: `data/`, `demo/`, `tmp/`, `third_party/`, `ide_files/`
- Active subsystems:
  - `src/app`, `src/runtime`, `src/db`, `src/ui`, `src/ui/graph`, `src/layout`
- Recent helper seams added in the live worktree:
  - `src/app/mem_console_app_loop_input.c`
  - `src/runtime/mem_console_runtime_refresh.c`
  - `src/runtime/mem_console_state_db_picker.c`
  - corresponding internal headers for loop/runtime decomposition
- Workspace Authoring seams:
  - `include/mem_console/mem_console_workspace_authoring.h`
  - `src/app/mem_console_workspace_authoring_host.c`
  - `src/ui/mem_console_workspace_authoring_overlay.c`

## Verification Contract
- Core gates:
  - `make -C mem_console clean && make -C mem_console`
  - `make -C mem_console test` aggregates the headless, data-path, and graph
    contract checks
  - `make -C mem_console run-data-path-contract-checks`
  - `make -C mem_console run-headless-smoke`
  - `make -C mem_console run-graph-contract-checks`
  - `make -C mem_console run-detail-relationship-contract-checks`
  - `make -C mem_console run-relationship-mutation-test`
  - `make -C mem_console run-browse-filter-contract-checks`
  - `make -C mem_console visual-harness` builds the visual runtime target and
    prints readiness output, but does not execute the interactive shell
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
- Continue `MCU1-S6` with repeatable visual verification fixtures for graph
  readability across `FOCUS`, `PODS`, and `WEB`.
- Manual visual acceptance for the Workspace Authoring host remains a separate
  runtime check.

## History and Deep Lane References
- Full execution history is in:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/memory_console/`
- This file is the compressed public current-state contract.
