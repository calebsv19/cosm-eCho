# mem_console

`mem_console` is the standalone Memory Console host for the Memory DB system.

This is a top-level program host, not a reusable shared kit.

Its job is to remain the first interactive proving ground for:

- `core_memdb`
- `kit_render`
- `kit_ui`
- `kit_graph_struct` (active in current Phase 4 graph-mode rollout)

## Current State

The current app is a windowed interactive console with the graph/runtime/pane shell, theme/font refinement, async refresh hardening, and packaging follow-up lanes already landed.

Right now it proves:
- the intended build and link shape is in place
- the host opens a target Memory DB through `core_memdb`
- the app runs a real SDL + Vulkan + `kit_render` frame loop
- the first split-pane shell renders with `kit_ui`
- the console now uses the shared additive `kit_ui` text-tier hooks for clearer hierarchy
- the console now uses `kit_ui` theme-scale style sync so control spacing/density follows the active shared theme preset
- the console now supports the standard shared live theme-cycle shortcut (`Cmd/Ctrl+Shift+T` forward, `Cmd/Ctrl+Shift+Y` backward)
- the left pane now supports app-owned typed search and visible filtered result rows
- the left pane now includes project-scope quick-filter chips (`ALL PROJECTS` + multi-select project toggles) for scoped browsing
- the result list is synchronously queried through `core_memdb` and supports click-to-select
- the result list now uses scalable query windowing (`LIMIT` + `OFFSET`) so large match sets can be traversed through scrolling
- the list scroll range now supports top-anchoring final rows (last row can be scrolled to top of viewport)
- result rows now include project tags in labels when project metadata is present
- the right pane now exposes create + explicit title/body edit flows
- title/body editing now runs through dedicated edit modes with explicit save/cancel actions (search text remains filter-oriented)
- active text input now supports cursor movement/edit keys and paste, with debounced search refresh
- right-pane actions are now grouped into compact memory/graph control rows above the graph area so core read/navigation space stays primary
- the detail pane now exposes `pinned` and `canonical` toggle actions for the selected memory
- the detail pane now renders a DB-backed one-hop graph preview for the selected memory
- graph node click now selects that memory and refreshes the detail pane
- graph mode now has explicit `GRAPH MODE` toggle and `REFRESH GRAPH` controls
- advanced graph filter/settings controls now collapse when graph mode is off (progressive disclosure)
- graph mode now includes a bounded edge-kind filter segmented control (`ALL`, `SUPPORTS`, `DEPENDS`, `REFS`, `SUMMARY`, `RELATED`)
- graph mode now includes node-kind filter controls and project-scope pod overlays for fuller neighborhood inspection
- detail pane now shows compact selected-node connection summaries in the top metadata row (right side of title/id area)
- graph edges now render compact kind labels so connection semantics are visible in preview
- graph edges touching selected node now use subtle directional tinting (outbound=green-tinted white, inbound=red-tinted white, bidirectional=white)
- graph preview layout is now cached by graph-signature and viewport bounds so draw/click paths reuse one computed layout instead of recomputing twice
- graph preview now draws routed orthogonal edge polylines from `kit_graph_struct` route helpers (boundary-attached endpoints)
- graph edge labels now use route-aware placement with bounded density controls (auto overlap-cull on denser neighborhoods and zoom-threshold hide policy)
- graph edge/label hit navigation now uses shared `kit_graph_struct` helpers for deterministic edge-click selection
- graph viewport now supports wheel zoom + drag pan with drag-release click suppression to avoid accidental node selection while panning
- graph camera now uses explicit screen<->world transforms with cursor-anchored zoom behavior
- graph node rectangles now render at stable on-screen size across zoom levels while position/camera movement remains zoom-aware
- repeated directed links between the same two nodes now render as separate routed lanes (parallel edge separation)
- top-level pane geometry now routes through a pane adapter backed by shared `core_pane` split-solve logic
- pane splitters are now draggable for left/right and right-top/right-bottom sizing in-app
- pane layout ratios now persist in the app-local prefs pack (`<db_path>.ui.pack`) and restore on startup with safe fallback clamping
- Workspace Authoring is now attached through shared `kit_workspace_authoring`: `Alt+C` then `Alt+V` enters active authoring, `Tab` switches pane vs. full-screen Font/Theme overlays, `Enter` applies, and `Esc`/toggle-out cancels previews
- authoring Font/Theme changes preview live through the existing `core_theme`, `core_font`, and text-zoom state, while normal runtime keeps no persistent authoring HUD
- accepted authoring changes persist through the existing per-DB `.ui.pack`; canceled previews restore the entry baseline
- root pane surfaces/seams now render in a dedicated chrome pass with pane-owned seam policy (IDE-style single-owner boundaries)
- top-level frame draw now enforces pane clip scopes for left/detail/graph paths to prevent cross-pane overdraw
- compact UI density pass now applies tighter default margins/insets/row heights and a compact `kit_ui` style override for IDE-like space usage
- graph click selection now requires click-start inside graph viewport for deterministic node/edge pick behavior
- the UI can synchronously reload a DB summary through the existing shared DB boundary
- runtime theme and font preset cycling are live and visible in-app
- current panel/background/row colors are resolved through shared theme tokens
- text rendering runs through the shared TTF-first `kit_render` path with fallback retained
- section text draw paths now use stable state-backed buffers (not transient stack strings) to prevent command-buffer glyph corruption
- periodic async DB refresh is now wired through shared runtime libs (`core_workers` + `core_queue` + `core_wake`) with main-thread apply safety guards
- idle-loop pacing now uses timed waits to reduce churn when no input/work is pending
- redraw invalidation reasons (`input/layout/theme/content/background`) now drive frame scheduling so the app sleeps when no redraw is needed
- input routing and invalidation accounting now run through explicit IR1-style intake/normalize/route/invalidate helper seams in `src/app/mem_console_app_loop_input.c`
- refresh requests now coalesce while a worker refresh is in-flight, keeping latest intent
- refresh intent matching/coalescing now includes selected project-filter sets (not only search/selection/offset)
- runtime refresh payload comparison and metrics publication now live in dedicated runtime helper seams
- runtime observability counters are now surfaced in the left pane for async submitted/applied/dropped/error/coalesced status
- optional kernel-bridge evaluation mode is available (`--kernel-bridge`) and surfaces compact kernel telemetry in the left pane
- theme/font preset selection now persists through an app-local `.pack` prefs file (`<db_path>.ui.pack`) using `core_pack`

It does not yet provide a full multiline editor widget, richer styling controls, or real authoring module insertion beyond the shared add-module stub.

Current source layout:
- `src/app/`:
  - lifecycle/bootstrap/loop, event/action dispatch, and Workspace Authoring host integration (`mem_console.c`, `mem_console_app_main.c`, `mem_console_app_loop.c`, `mem_console_workspace_authoring_host.c`)
- `src/runtime/`:
  - runtime state/path/prefs/worker lanes (`mem_console_state*.c`, `mem_console_prefs.c`, `mem_console_prefs_app_io.c`, `mem_console_runtime.c`)
- `src/db/`:
  - DB query/filter/read/mutation paths (`mem_console_db*.c`)
- `src/layout/`:
  - pane split and density config (`mem_console_pane_layout.c`, `mem_console_layout_config.c`)
- `src/ui/`:
  - frame/chrome/left/detail/hud composition and shared Workspace Authoring overlay rendering
- `src/ui/graph/`:
  - graph viewport/camera/draw/controls/layout/overlay/pods (`mem_console_ui_graph_layout_focus_helpers.c` included)
- `include/mem_console/`:
  - public state/types/runtime contracts used across lanes

## Build

Shared runtime/modules are vendored in-repo at:

- `third_party/codework_shared/`

Scaffold docs lane:
- `mem_console/docs/README.md`
- `mem_console/docs/current_truth.md`
- `mem_console/docs/future_intent.md`
- `mem_console/docs/architecture.md`
- `mem_console/docs/migration.md`

```sh
make -C mem_console
```

Quick run:

```sh
make -C mem_console run
```

`run` launches from the repo root so shared asset-relative paths (fonts, demo assets) resolve consistently.

Run with explicit demo DB:

```sh
make -C mem_console run-demo
```

Pass extra args:

```sh
make -C mem_console run RUN_ARGS="--db /tmp/mem_console_phase7.sqlite"
make -C mem_console run RUN_ARGS="--db /tmp/mem_console_phase7.sqlite --kernel-bridge"
```

### Shared Subtree Update

```bash
git -C mem_console fetch shared-upstream main
git -C mem_console subtree pull --prefix=third_party/codework_shared shared-upstream main --squash
```

Rebuild check:

```bash
make -C mem_console clean && make -C mem_console
```

Verification gates:

```bash
make -C mem_console test
make -C mem_console run-headless-smoke
make -C mem_console visual-harness
```

## Run

```sh
./mem_console/build/mem_console
./mem_console/build/mem_console --db /path/to/memory.sqlite
./mem_console/build/mem_console --db /path/to/memory.sqlite --kernel-bridge
```

Default behavior:
- running without `--db` resolves in this order:
  1. `CODEWORK_MEMDB_PATH` (if set)
  2. last-used DB path from app prefs (`~/Library/Application Support/MemConsole/runtime/mem_console.app.pack` on macOS)
  3. fallback default DB (`~/Library/Application Support/MemConsole/runtime/default.sqlite` on macOS, otherwise `mem_console/data/default.sqlite`)
- parent directories are created automatically for the chosen DB path before opening/creating the SQLite file
- passing `--db /path/to/file.sqlite` is the intended way to create or switch to a project-specific DB
- the left pane now includes `LOAD DB` and `NEW DB` buttons that open an in-app path-entry modal
  - `LOAD DB` is reference mode: entered path is used directly (no hidden rewrite)
  - `LOAD DB` now also lists discovered `.sqlite` files from `input_root` for click/keyboard selection (`Up/Down`)
  - `NEW DB` accepts either:
    - a bare name (`my_db`) -> created at `<input_root>/my_db.sqlite`
    - an explicit path (`/path/to/my_db` or `~/path/to/my_db`) -> created at that explicit path (`.sqlite` implied if missing)
  - `~/...` paths are expanded to the current user home directory
  - confirming the modal switches the active visual session to that DB, creating missing parent directories and the DB file when needed
- UI preset prefs are read from/written to `<db_path>.ui.pack` for whichever DB path is active
  - includes theme/font preset selection plus pane layout ratios
- app-level startup prefs are stored separately from DB UI prefs so the console can remember the last active DB path
  - app prefs default path follows output-root: `<output_root>/mem_console.app.pack`
  - legacy fallback read is preserved for `~/.local/share/mem_console/mem_console.app.pack`
- input-root controls are available in the left pane and via keyboard:
  - `Cmd/Ctrl+I`: open input-root path modal (edit/apply)
  - `Cmd/Ctrl+B`: open native folder chooser and set input root

Run targets:
- `make -C mem_console run` respects the normal startup resolution order, so it reopens the saved last-used DB when one exists
- `make -C mem_console run-demo` uses `mem_console/demo/demo_mem_console.sqlite`
- demo helper scripts default to the demo DB unless `CODEWORK_MEMDB_PATH` is set

Runtime shortcuts:
- `Cmd/Ctrl+Shift+T`: cycle theme preset forward
- `Cmd/Ctrl+Shift+Y`: cycle theme preset backward
- `Cmd/Ctrl+Shift+U`: cycle font preset forward
- `Cmd/Ctrl+Shift+I`: cycle font preset backward
- `Cmd/Ctrl+I`: open input-root editor modal
- `Cmd/Ctrl+B`: open native input-root folder chooser

Large-list scroll audit helpers:
- `mem_console/demo/seed_large_list.sh`
- `mem_console/demo/LARGE_LIST_AUDIT.md`

Demo reset helper:
- `mem_console/demo/reset_demo_db.sh`
  - now seeds a connected memory/link dataset for immediate graph validation

## Near-Term Target

The next implementation steps are:
- add richer link editing affordances on top of the current graph mode controls
- finish graph-mode audit closure and Codex skill packaging validation
