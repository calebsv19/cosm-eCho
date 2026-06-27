# eCho Source Orientation

This tree holds app-local source for `mem_console`, the eCho Memory Console
host. Shared semantics stay in `third_party/codework_shared/`; public app
contracts stay in `include/mem_console/`.

## Lanes

- `app/`: process entry, lifecycle stages, run-loop orchestration, top-level
  action dispatch, DB-switch coordination, theme/app event routing, optional
  kernel-bridge entry, and Workspace Authoring host integration.
- `runtime/`: app/session state helpers, DB path policy, DB picker/modal state,
  app and per-DB prefs, async refresh payload comparison, runtime pacing, and
  render-facing state derivation helpers.
- `db/`: `core_memdb` query/read/mutation adapters, browse filters,
  relationship mutations, graph loading, graph sorting, and DB-facing contract
  helpers.
- `layout/`: pane split solving and compact density/layout configuration.
- `ui/`: frame composition, chrome, left/detail/HUD surfaces, shared text/draw
  helpers, browse controls, relationship/detail rendering, and Workspace
  Authoring overlay drawing.
- `ui/graph/`: graph panel, viewport/camera, controls, HUD, geometry, draw
  submission, mode policy, layout variants, overlays, and project-pod
  rendering.

## Ownership Rules

- `app/` coordinates phases and actions; it should not own DB query policy,
  render-label formatting, or durable prefs serialization.
- Action status text is an app-local UX boundary: action-branch messages in
  `mem_console_app_actions.c`, loop diagnostics, and DB-switch status messages
  should route through the local formatted status helper instead of open-coded
  `status_line` writes plus redraw marks.
- `runtime/` owns active app state interpretation and app-local persistence
  policy. Keep render derivation read-only with respect to authoritative state.
- `db/` owns SQL-facing behavior over `core_memdb`. Keep arbitrary SQL or
  caller-provided column/path policy out of UI and action callers.
- `ui/` and `ui/graph/` submit draw work and handle view-local hit decisions.
  Derived text and backing storage should come from stable state/storage views,
  not from frame-local temporary strings queued into render commands.
- `layout/` owns geometry policy only. It should not accumulate app actions,
  DB reads, or renderer submission logic.
- `include/mem_console/` is the cross-lane public contract. Prefer private
  headers inside `src/` when a helper is only used by one lane.

## Active Boundaries

- The current no-UI boundary-hardening lane has already centralized DB path
  policy, DB mutation flag APIs, left/detail render-state derivation, action
  roles, and graph draw-label derivation.
- The R1 duplication pass is closed. `R1-S1` through `R1-S5` consolidated
  app-owned status formatting across actions, app-loop diagnostics, DB
  switching, theme/input/authoring messages, startup/app-seed messages, and
  graph-local diagnostic/HUD/anchor visibility text. Runtime async refresh and
  default state seed status remain runtime/state-owned. DB path policy, UI
  prefs path helpers, and render-label derivation remain reuse-deferred because
  their canonical helpers already exist.
- R2 app-state ownership has started. `R2-S1` routes input-target mutation
  through `mem_console_input_target_set(...)`; UI/app code may read
  `state->input_target` for active-control rendering and text routing, but
  focus changes should use the runtime helper.
- `R2-S2` routes pane prefs dirty/clean mutation through
  `mem_console_pane_prefs_mark_dirty(...)` and
  `mem_console_pane_prefs_mark_clean(...)`; save-decision code may still read
  `state->pane_prefs_dirty`.
- `R2-S3` routes selected-memory and graph-center fallback mutation through
  `mem_console_selection_set(...)`, `mem_console_selection_clear(...)`,
  `mem_console_graph_center_set(...)`, `mem_console_selection_center_on(...)`,
  and `mem_console_selection_apply_refreshed(...)`. Inspect-only UI selection
  still avoids graph recentering; navigation and visual-review seeding still
  set graph center explicitly.
- `R2-S4` keeps async refresh pending and in-flight intent mutation
  runtime-owned through helper-routed store/apply/clear paths, with
  `run-runtime-refresh-contract-checks` wired into the normal test suite.
- `R2-S5` keeps live graph viewport pan/zoom mutation graph-camera-owned
  through `graph_camera_store_live_viewport(...)` and
  `graph_camera_pan_live_viewport_by_screen_delta(...)`, while persisted
  viewport capture/restore stays prefs-version-owned through
  `prefs_store_graph_viewport_from_state(...)` and
  `prefs_apply_graph_viewport_to_state(...)`.
- `R2` is closed as of 2026-06-26.
- `R3-S1` prefs and DB-switch diagnostics are complete: UI prefs load/save, app
  prefs save, DB directory creation, DB open/switch, pending DB switch, pane
  prefs save, and input-root app prefs save failures now route through
  operation/path/result status text with contract checks guarding the old generic
  messages from returning.
- `R3-S2` post-mutation refresh diagnostics are complete: create/edit/flag and
  relationship write failures remain distinct from follow-up refresh failures,
  which now report `Refresh after <mutation> failed` with contract coverage.
- `R3-S3` async refresh latest-error visibility is complete: the runtime stores
  and publishes the latest bounded async refresh error, the existing async
  counter summary appends `last error: ...` only after a failure, and the left
  panel displays that runtime-owned summary through state-role derivation.
- `R3-S4` graph and relationship diagnostics are complete: graph action
  failures now report selected/center/filter/limit/hop context, relationship
  action failures report selected/link/target context, and validation messages
  distinguish missing selection, missing relationship, and unavailable graph
  layout cases.
- `R3-S5` package and launcher diagnostics are complete: launcher config output
  is shared by `--print-config` and successful `--self-test`, self-test
  failures report the failed path plus log/runtime/DB/shader context, and the
  packaged self-test failure path prints launcher config for operator readback.
- `R3` is closed as of 2026-06-26.
- `R4-S1` DB path parent-segment hardening is complete: DB path policy now
  rejects parent-directory segments in every DB path, including absolute paths,
  and the path contract test guards that security boundary.
- `R4-S2` demo/helper target DB default safety is complete: destructive demo
  helpers no longer default to `CODEWORK_MEMDB_PATH`, and demo helper writes are
  limited to demo/build `.sqlite` paths unless explicitly opted in.
- `R4-S3` Memory DB mutation boundary hardening is complete: item updates now
  require a returned selected active item before success or FTS sync, and stale
  or archived selected-item writes fail closed.
- `R4-S4` local private/generated artifact boundary hardening is complete:
  desktop packaging copies only `data/default.sqlite` into bundle resources and
  rejects local data sidecars during package smoke.
- `R4-S5` package/release artifact boundary closeout is complete:
  `release-bundle-audit` now writes a bundle manifest and rejects
  private/generated paths, packaged `.ui.pack` sidecars, and unexpected data
  sidecars before release artifact creation.
- `R4` is closed as of 2026-06-26.
- `R5` testability audit is complete: the broad test aggregate is strong, while
  browse/filter behavior was the first queued improvement because it was still
  mostly source-string guarded rather than temp-DB behavior tested.
- `R5-S1` is complete: `mem_console_browse_filter_test.c` now proves
  pinned-only, canonical-only, kind-cycle, and pagination query behavior
  through the normal browse-filter contract target.
- `R5-S2` is complete: `mem_console_runtime_refresh_intent_test.c` now proves
  browse-filter intent capture, mismatch detection, and refreshed-state browse
  field application through the normal runtime-refresh contract target.
- `R5-S3` is complete: `mem_console_graph_layout_model_test.c` now proves the
  WEB graph layout keeps the selected root centered and separates disconnected
  components through the normal graph contract target.
- `R5-S4` is complete: `capture_visual_graph_fixture.sh --plan-only` now
  writes a no-launch capture plan and manifest for expected `FOCUS`, `PODS`,
  and `WEB` screenshot/log/JSON artifacts through the normal visual-fixture
  contract target.
- `R5-S5` is complete: R5 is closed after behavior-backed coverage for
  browse/filter, runtime refresh intent, graph layout model, and visual fixture
  capture artifact contracts.
- `R6-S2` is complete: `make -C mem_console visual-artifact` seeds the
  deterministic visual graph fixture, runs an app-owned first frame through the
  null render backend, and writes
  `visual_artifacts/mem_console_first_frame_web.svg`.
- `R6-S3` is complete: `run-visual-artifact-contract-checks` verifies the
  source-run success lines and SVG metadata without desktop capture.
- `R6-S4` is complete: the R0-R6 cycle is archived after source-run artifact
  proof, artifact contract coverage, broad test, package self-test, and
  release-impact dry-run readback.
- Future source changes should start from a fresh scoped plan unless they are
  part of the separate no-UI boundary hardening lane.
- Future graph/product polish should start from a fresh active plan rather than
  extending the archived MCU1/MCG1 graph plans.

## R0 Guidance

Do not move source files during an R0 orientation slice unless a path is
actively misleading. This tree is already split by role; the main R0 value is
making ownership and next-pass boundaries easy to find before R1 duplication,
R2 app-state, and later refinement passes.
