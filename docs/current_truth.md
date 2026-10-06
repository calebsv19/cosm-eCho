# eCho Current Truth

## 2026-10-06 source adoption

Canonical source now includes the accumulated Main Edit UI work and the runtime
isolation/empty-seed repairs. VERSION is 0.4.0 for authorized local preparation;
public release remains 0.3.0 and installed applications are unchanged. Release
packaging uses a create-only job root and isolated synthetic runtime. Earlier
rollout notes below describe their original Main Edit-only proof boundaries;
they do not describe the current canonical source adoption state.


Last updated: 2026-10-04

## Program Identity
- Product name: `eCho`
- Repository/program directory: `mem_console`
- Canonical symbol/file prefix: `mem_console`
- Primary private planning bucket:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/memory_console/`

## Main Edit Rounded UI Slice (2026-10-04)

The retained Main Edit lane adopts shared commit
`e5887348657f782e6094cdeeb095259dedb32bf8` through a managed subtree update.
The common DB/browser/graph inspector buttons use `kit_ui`'s compact rounded appearance
and state/style resolver instead of local square border strips. App-owned
hit bounds, action dispatch, caption roles/vertical anchors, and persistence stay local.
Button labels use measured horizontal centering in the common adapter.

Minimums for this slice are `kit_ui 0.11.3`, `kit_render 0.14.5`,
`vk_renderer 1.4.0`, and `vk_runtime 0.6.0`. Positive rectangle radii now reach
native solid Vulkan geometry with drawable-scale edge coverage; previously the
Vulkan adapter discarded them. Frame-buffer growth also retains recorded draw
storage until its frame fence completes. Other control groups and native draw
calls have not been migrated by this slice.

This describes the Main Edit development package. Canonical adoption, release
version changes, and publication are separate steps. See [shared UI rendering](ui_rendering.md)
for checks and a visual review route.

## Current Shipped State
- The persistent implementation lane is
  `<CodeWork>/_worktrees/mem_console_main_edit` on
  `codex/mem-console-main-edit`. Its `eCho Main Edit.app` package is isolated by
  bundle identifier, runtime/log namespace, target-specific artifact root, and
  an embedded source/binary identity. Canonical adoption remains a separate
  clean fast-forward gate.
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
- Active DB startup resolution preserves `--db`, an explicit
  `CODEWORK_MEMDB_PATH`, then a DB saved in the selected runtime's own app
  preferences. The launcher's generated default is used only without a selection.
- `MEM_CONSOLE_RUNTIME_DIR` selects the mutable runtime root; Main Edit defaults
  to the `MemConsole-Main-Edit` namespace. Isolated roots never fall back to
  standard/legacy app preferences. The standard default keeps legacy compatibility.
- Path roots are normalized together. An explicit runtime root cannot be replaced
  by a saved output root; `input_root` falls back to the selected DB parent when
  no explicit input-root hint survives normalization. Invalid runtime roots fail.
- See [runtime isolation](runtime_isolation.md) for exact selection and backup
  boundaries. No database schema or stored preference format changes are required.
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
- Package seeds are generated from schema code in a fresh temporary directory,
  checked for integrity, schema version 6 and zero application records, and
  copied into bundle resources with exclusive creation. Ignored `data/` files
  and `PACKAGE_DEFAULT_DB_SRC` are never seed inputs. Additional data sidecars
  remain rejected in package smoke.
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

## 2026-10-04 Main Edit render fidelity

The retained development lane adopts shared `f80f91d`, with `kit_render 0.14.6`,
`vk_renderer 1.5.0`, `kit_ui 0.12.0`, `kit_workspace_authoring 0.5.2` and
`vk_runtime 0.6.0`. Command transforms, texture UVs and RGBA tint have native
Vulkan conformance coverage through the application's copied shared archives.
See [Native render command fidelity](render_fidelity.md) for repeatable gates
and the source/package/interactive-review boundaries.

## Font/Theme interaction follow-up

Main Edit now adopts optional shared button focus/capture and rounded Font/Theme control state.
See [Font/Theme interaction](ui_interaction.md) for scope, shortcuts and proof gates.
The composite source pin advances to the interaction snapshot; Vulkan transforms, UVs and tint remain unchanged.


## 2026-10-04 complete button surface adoption

Main Edit now uses `kit_ui 0.14.1` surface snapshots across the inventoried button surfaces, with the shared `b1c67d7` source pin. Left root/DB/load/refresh controls, browse filters, project chips and item rows; graph labels/edge limit/hops/kinds/view/sort/flags and action HUD; relationship Add/navigation/kind/delete; graph legend filters; DB-modal rows/actions; common Font/Theme and top authoring controls. DB-modal and authoring scopes exclude background buttons. Text fields explicitly take keyboard ownership while pointer controls remain available.

See [the current interaction reference](ui_interaction.md) for exact scope, replay gates, capture proof and remaining text-edit/pane/native-Linux boundaries. This is local Development adoption; app VERSION and canonical/production release state are unchanged.


## 2026-10-05 bounded text editing and modal focus

Search, title, body, DB/input-root path, graph edge limit and relationship target now use `kit_ui 0.15.1` bounded editing in Main Edit. Title Enter saves and Escape cancels; body Enter inserts a newline and Ctrl/Cmd+Enter saves; DB Enter confirms and Escape cancels. Search Enter refreshes; numeric Enter uses the existing graph/relationship actions. Numeric fields reject a mixed invalid paste atomically. Only committed search edits trigger debounce invalidation. Opening a DB modal ends title/body editing by existing host policy; closing it returns to Search, not an abandoned text editor.

See [Text editing and modal focus](ui_text_focus.md) for exact ownership, tests and limitations. Text presentation, native IME/platform acceptance, pane composition and wider program adoption remain later slices. Earlier dated milestones describe their historical state.


## 2026-10-05 shared text presentation

Search, title, body, DB/input-root path, graph edge limit and relationship target now adopt `kit_ui 0.16.0` measured text presentation in Main Edit. Six distinct UI-owned slots prevent queued text aliasing. Body editing uses measured scalar hard wrapping and explicit newlines; read-only bodies retain their existing layout. DB editing uses the full draft with horizontal caret-follow instead of an ellipsis string and duplicate byte-selection offsets. Numeric validation and database/session actions retain their owners.

See [Shared text presentation](ui_text_presentation.md) for storage ownership, output gates and residual limits. The next behavioral slice is mixed field/button traversal; native IME/session/clipboard qualification and pane composition follow before wider adoption. Earlier dated milestones remain historical.

## 2026-10-05 pane composition follow-on

Echo navigation/detail/graph use a shared composition snapshot after existing CorePane layout solve. Nested pane clips also constrain button registration. A caller-owned pane pointer owner prevents a press in one section from becoming a release gesture in another and cancels on modal/authoring/splitter takeover. Existing product headers stay inside content, with no extra title row.

See [pane composition adoption](ui_pane_composition.md). Pane composition now has
a shared foundation; the next boundary is pane-host lifecycle/dispatch and
transactional splitter/focus takeover. Mixed traversal is kit-only preparation;
native caret anchoring is integrated, with OS IME acceptance still pending.

## Pane host behavior

Echo gives metadata, relationships and body separate visible/input clips. Splitters use current parent spans, defer preference writes to accepted commit and restore all four ratios on cancel. GRAPH header REFRESH dispatches the existing domain action.

See [pane host behavior](ui_pane_host.md). Shared source ddc9fee6e17482dcd64cf777d7a105b7ed9b157d. Next: fullscreen/window lifecycle qualification in the trio, then one-program-at-a-time migration. Product mixed traversal, human native IME acceptance and generic docking remain follow-on work.

## Fullscreen/window lifecycle acceptance — 2026-10-05

Accepted shared source `854b51ff57c756b4565021fe759c27a459efbfd3` supplies `kit_ui 0.18.0` optional SDL window
observation, coordinate mapping, F11 desktop-fullscreen and bounded native proof;
`vk_renderer 1.6.0` resets fences only before submission, consumes suboptimal
acquired images and performs bounded out-of-date recovery. Hosts own their loops,
window/resource lifetimes, layouts, domain state, edit restoration and persistence.
Logical size, actual drawable size and an explicitly bounded render extent are
separate. Resize/move/display/maximize/restore/hide/minimize/focus loss cancel stale
pane/control ownership before consumers. Non-presentable windows defer submissions.

Orchestra and Echo use logical command UI coordinates. DataLab retains SDL drawing
and Vulkan canvas presentation, including a bounded 4096x4096 canvas; large
presentation extents scale that canvas with matched paint/input geometry. Both
DataLab viewer and startup picker adopt the lifecycle; the plain SDL viewer path
also passes. Native macOS actual loops pass all eight stages (initial, resize,
fullscreen, windowed, hidden, shown, minimized, restored), six captures each and
continued rendering, with zero Vulkan validation warnings/errors. DataLab also
passes a 5000x1440 drawable. Unit fault injection proves acquire-out-of-date and
suboptimal fence behavior; production-linked pane tests prove canceled payload
and revision restoration for all ten invalidating event kinds.

Scope is the proving trio retained Main Edit lanes. Canonical/stable programs and
app VERSION remain unchanged. macOS proof does not qualify Linux/Windows,
exclusive fullscreen, external-monitor migration, device-loss recovery, human IME
workflow or every program. The native proof drives SDL desktop fullscreen; it does
not automate clicking macOS's green window control. Next: apply the established
migration recipe to one selected program, then qualify its actual UI/native paths.
Mixed field/button traversal and OS IME sessions remain bounded follow-on work;
generalized docking/provider insertion/persistence needs its own contract.
