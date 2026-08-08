# eCho Current Truth

Last updated: 2026-08-07

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
    neighbors, larger root-neighborhood spacing, earlier emphasized labels,
    selected/center halos, primary-edge emphasis, and default first-frame
    camera fit over the selected root plus direct neighbors
  - `WEB` mode now uses a dedicated topology helper that separates visible
    connected components into islands and ranks bridge/high-degree
    cross-project nodes toward component centers
  - graph edge rendering now uses mode-specific label/emphasis rules so
    `FOCUS`, `PODS`, and `WEB` do not all label and mark edges the same way
  - graph edge/node draw labels are derived through a narrow render-state view
    before draw submission, with stable per-edge/per-node backing storage for
    queued text commands
  - graph/list/relationship single-click selection is inspect-only:
    selected detail and relationship rows refresh without changing the stable
    graph center or rebuilding graph topology
  - graph/list double-click remains the explicit recenter/reload gesture, and
    graph reload root priority follows `graph_center_item_id` before the
    currently inspected memory
  - a deterministic visual-review fixture/capture lane can launch the app
    directly into `FOCUS`, `PODS`, or `WEB` and capture screenshots for
    comparable graph-mode review
- Selected-memory detail now includes a relationship inspector:
  - bounded `mem_link` rows load beside the selected title/body
  - inbound/outbound groups are separated by link kind
  - rows show neighbor id, project, kind, and title
  - row clicks inspect the neighbor detail without recentering the graph
  - a compact target-id input can add a selected-memory outgoing `related`
    link to an active target memory
  - row-scoped `KIND` and `DEL` controls change or remove only links that
    touch the selected memory
  - detail title/meta and relationship display strings are derived through a
    narrow render-state view before draw submission, with stable backing
    storage for title lines, meta text, empty-state text, and relationship
    group/row labels
- Left browse is now a faceted investigation path:
  - search and project filters remain the base query controls
  - pinned-only, canonical-only, and kind-cycle facets narrow the list without
    changing the DB model
  - matching count and visible list windows both honor the same browse facets
  - async refresh captures browse facets as part of request intent so stale
    unfiltered results are not applied after a facet change
  - result rows show id, pinned/canonical flags, project, kind, compact updated
    time, and title in a stable scan order
  - left-panel display strings are derived through a narrow render-state view
    before draw submission, with dedicated stable backing storage for DB,
    input-root, schema, visible-count, status, and result-row labels
- DB switching/input-root flows are active in-app:
  - `LOAD DB` and `NEW DB` path modal flow
  - discovered `.sqlite` selection from `input_root`
  - startup, modal confirmation, and DB switch share the same DB-path policy:
    DB paths must end in `.sqlite`, avoid control characters, and avoid
    parent-directory segments in all DB paths
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
- Managed Vulkan adoption is source- and package-proven against canonical
  shared commit `60084f90564105983c7c74e862a299d8b6775347`:
  - the default vendored build now carries `vk_runtime 0.6.0` beneath
    `vk_renderer 1.3.1`; `SHARED_ROOT=../shared` remains a bounded development
    override rather than the normal build path
  - the renderer compatibility handles mirror the runtime-owned Vulkan
    instance, device, graphics queue, and present queue
  - `make -C mem_console vulkan-rollout-self-test` requires Khronos validation,
    draws nontrivial frames, reads back captures, performs a real drawable
    resize, and proves shutdown/restart lifecycle reuse
  - the Apple M2 proof recorded zero validation warnings/errors at startup,
    resize, and restart, with drawable extents changing from `1440x900` to
    `1800x1120` and a measured `2.000` render scale before and after resize
  - this is presentation/runtime lifecycle adoption only; eCho does not call
    the shared compute, residency, or timing workload APIs

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
  - `LOAD DB` uses the entered `.sqlite` path directly after validation
  - `NEW DB` creates a bare-name target under `input_root` and uses explicit
    `.sqlite` paths directly after validation
- App-level startup prefs stay separate from per-DB UI prefs:
  - app prefs default to `<output_root>/mem_console.app.pack`
  - per-DB UI prefs persist in `<db_path>.ui.pack`

## Structure
- Required lanes: `docs/`, `src/`, `include/`, `tests/`, `build/`
- Support lanes: `data/`, `demo/`, `tmp/`, `third_party/`, `ide_files/`
- Active subsystems:
  - `src/app`, `src/runtime`, `src/db`, `src/ui`, `src/ui/graph`, `src/layout`
- Source-lane ownership notes live in `mem_console/src/README.md`.
- Recent helper seams added in the live worktree:
  - `src/app/mem_console_app_loop_input.c`
  - `src/app/mem_console_app_status.c` keeps action, app-loop, and DB-switch
    status messages behind an app-local formatted status helper
  - `src/app/mem_console_action_roles.c`
  - `src/runtime/mem_console_runtime_refresh.c`
  - `src/runtime/mem_console_state_roles.c`
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
  - `make -C mem_console run-db-mutation-contract-checks`
  - `make -C mem_console run-state-boundary-contract-checks`
  - `make -C mem_console run-headless-smoke`
  - `make -C mem_console run-graph-contract-checks`
  - `make -C mem_console run-detail-relationship-contract-checks`
  - `make -C mem_console run-package-diagnostic-contract-checks`
  - `make -C mem_console run-relationship-mutation-test`
  - `make -C mem_console run-browse-filter-contract-checks`
  - `make -C mem_console run-visual-fixture-contract-checks`
  - `make -C mem_console vulkan-rollout-contract`
  - `make -C mem_console vulkan-rollout-self-test`
  - `make -C mem_console visual-harness` builds the visual runtime target and
    prints readiness output, but does not execute the interactive shell
  - `make -C mem_console visual-fixture-capture` builds a deterministic graph
    fixture, launches the app in `FOCUS` / `PODS` / `WEB`, and captures windows
    under `_private_workspace_artifacts/desktop_capture/`
  - `mem_console/demo/capture_visual_graph_fixture.sh --plan-only --out-root <path>`
    writes the expected capture plan and manifest rows without launching the
    app or requiring `desktop_capture`
- R6 demo audit is complete:
  - current demo/proof surfaces are `make -C mem_console run-demo`,
    `make -C mem_console run-headless-smoke`, `make -C mem_console test`,
    `make -C mem_console visual-harness`,
    `make -C mem_console visual-fixture-capture`, plan-only visual fixture
    artifact checks, `make -C mem_console visual-artifact`, and
    `make -C mem_console package-desktop-self-test`
  - `visual-harness` is a build/readiness and manual validation target; it does
    not produce a first-frame image artifact
  - `visual-fixture-capture` is GUI/desktop-capture operator evidence; its
    plan-only mode is the cheap artifact-contract check
  - `visual-artifact` is the source-run first-frame baseline: it runs the
    deterministic visual graph fixture through the app-owned frame path with
    the null render backend, writes a nonblank SVG command-stream artifact under
    ignored `mem_console/visual_artifacts/`, reports the final artifact path,
    and fails on missing, empty, or zero-visible-command output
  - `run-visual-artifact-contract-checks` is the no-desktop-capture contract
    gate for that route and is included in `make -C mem_console test`
- R5 testability audit is complete:
  - current strengths are broad aggregate coverage plus compiled/temp-DB proof
    for DB path, item mutation, relationship mutation, browse/filter behavior,
    runtime refresh intent, graph layout model behavior, and state-role/render
    derivation
  - R5-S1 browse/filter coverage is behavior-backed:
    `run-browse-filter-contract-checks` now runs a temp-DB query-results probe
    for pinned-only, canonical-only, kind-cycle, and pagination behavior before
    the source-string wiring guard
  - R5-S2 runtime refresh intent coverage is behavior-backed:
    `run-runtime-refresh-contract-checks` now runs a no-UI compiled probe for
    browse-filter intent capture, mismatch detection, and refreshed-state
    browse field application before the source-string ownership guard
  - R5-S3 graph layout model coverage is behavior-backed:
    `run-graph-contract-checks` now runs a no-UI WEB layout model probe for
    selected-root centering and disconnected component separation before the
    broad graph source-contract guard
  - R5-S4 visual fixture capture artifact coverage is behavior-backed:
    `run-visual-fixture-contract-checks` now proves the no-launch capture plan
    and manifest rows for `FOCUS`, `PODS`, and `WEB` expected screenshot/log/JSON
    outputs before any desktop capture is required
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
- Launcher diagnostics include `--print-config`, `--self-test`, startup logfile
  output, failed-path self-test context, and package self-test config readback.
- R4-S4 package data artifact hardening is complete: desktop packaging copies
  only `data/default.sqlite` into bundle resources and rejects additional data
  sidecars in package smoke.
- R4-S5 release artifact boundary hardening is complete:
  `release-bundle-audit` writes `bundle_manifest.txt` and rejects
  private/generated root names, packaged `.ui.pack` sidecars, and unexpected
  `Resources/data` files before release artifact creation.
- Optional icon contract is active via `PACKAGE_APP_ICON_SRC` / `PACKAGE_APP_ICONSET_SRC`.
- Multi-arch Intel packaging lane is complete through local staging + shader-runtime follow-up:
  - target-scoped build/package roots under `build/targets/<target-triple>/...`
  - architecture-tagged release artifacts
  - launcher now seeds real runtime shader copies for Intel retest safety

## Current Boundary
- Graph/visualizer work is at a stable post-`MCU1` baseline; the completed
  visualizer and graph-overhaul plans are archived in the private
  `memory_console` bucket.
- Future graph-as-control polish should start as a new scoped plan rather than
  extending the archived `MCU1` or `MCG1` lanes.
- Workspace Authoring baseline attach and operator visual acceptance are
  complete and archived in the private `memory_console` bucket.
- The no-UI boundary-hardening lane has completed DB path policy, DB mutation,
  left-panel render derivation, detail-pane render derivation, and action-role
  first passes, plus graph draw-label render derivation; graph HUD/status
  derived strings now route through the graph-local status helper, while
  broader SDL input-router cleanup remains a later scoped boundary.
- The named R0-R6 scaffold refinement pass series is complete and archived in
  the private `memory_console` bucket as of 2026-06-27. The cycle closed
  app-local status-formatting duplication, app-state mutation ownership,
  diagnostics, DB path/demo/mutation/package security boundaries, behavior
  coverage, and demo proof. The R6 closeout adds a source-run first-frame
  `visual-artifact` route plus a visual artifact contract gate; the completed
  proof set includes `make -C mem_console visual-artifact`,
  `make -C mem_console run-visual-artifact-contract-checks`, broad
  `make -C mem_console test`, and packaged self-test. The current public
  behavior contract remains unchanged.

## History and Deep Lane References
- Full execution history is in:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/memory_console/`
- This file is the compressed public current-state contract.
