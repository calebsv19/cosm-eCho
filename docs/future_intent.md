# mem_console Future Intent

Last updated: 2026-05-04

## Scaffold Alignment Targets
- lock lifecycle around a canonical app wrapper (`mem_console_app_main`) and named startup stages
- remove active-lane `_backup` directories from `src/` after parity verification
- keep subsystem ownership explicit and behavior-split as files grow

## Startup Shape Target
- preserve a consistent stage order:
  1. bootstrap
  2. config load
  3. state seed
  4. subsystem init
  5. runtime start
  6. run loop
  7. shutdown

## Cross-Program Wrapper Initiative
- `W0` complete (canonical wrapper contract frozen)
- `W1` complete for `mem_console` (typed stage/context wrapper shape already aligned)
- `W2` complete for `mem_console` (structured wrapper diagnostics normalization + wrapper exit summary logging)
- execution note:
  - `../../docs/private_program_docs/memory_console/2026-04-02_mem_console_w1_w2_wrapper_hardening.md`

## Optional Lane Policy
- keep optional top-level lanes when needed and justified:
  - `assets/`
  - `config/`
  - `external/`
- avoid introducing new top-level directories without explicit review.

## Migration Mode
- iterative program-by-program migration (no ecosystem big-bang rewrite)
- copy-first, non-destructive structure changes
- docs updated when behavior/structure meaning changes

## Desktop Packaging Follow-Up
- release-readiness lane is now complete for `mem_console` (`MC-RL0` through `MC-RL5`):
  - standardized `package-desktop*` + `release-*` targets are landed.
  - launcher runtime model is hardened for writable runtime root + Vulkan ICD exports.
  - bundle id and product naming are locked for release (`eCho`, `com.cosm.echo`).
  - notarization evidence refreshed: `eCho-0.1.0-macOS-stable.zip` accepted (submission `a2d52469-89db-4117-b263-f30b7d94e7b0`).
  - release dependency hardening: `release-staple` requires `release-notarize`.
- next packaging posture:
  - maintenance-only parity with ecosystem release contract updates.
  - keep Desktop/Finder launch + `release-verify-notarized` evidence in packaging-affecting closeout gates.
  - Intel validation/retest evidence should live as release-follow-up history, not as an active implementation lane once the handoff plan is archived.

## Runtime Loop and Refresh Intent
- keep the app-loop/input/runtime split explicit:
  - `mem_console_app_loop.c` should stay as orchestrator
  - input normalization/routing/invalidation stays in `mem_console_app_loop_input.c`
  - async refresh payload/intent helpers stay in `mem_console_runtime_refresh.c`
- continue using helper extraction when loop/runtime files grow, instead of re-embedding orchestration inline

## Graph Inspection Intent
- preserve current graph exploration strengths:
  - edge-kind filters
  - node-kind filters
  - project pod overlays
  - camera pan/zoom and routed edge previews
- next graph work should add richer editing/inspection affordances without weakening deterministic refresh behavior

## Large-File Maintenance Posture
- continue behavior-preserving helper extraction when modules exceed readability/size thresholds.
- keep function ownership docs in sync when helper modules move between `src/runtime/` and `src/ui/graph/`.


## Shared interaction follow-on

Main Edit has adopted the shared button interaction contract on the common
Font/Theme authoring controls. Extend the same optional context to normal
runtime HUD/inspector controls after reviewing each input scope and shortcut
owner. Text-entry/modal composition, pane layout and native Linux qualification
remain separate slices. See [the implemented reference](ui_interaction.md).


## 2026-10-04 complete button surface adoption

Main Edit now uses `kit_ui 0.14.1` surface snapshots across the inventoried button surfaces, with the shared `b1c67d7` source pin. Left root/DB/load/refresh controls, browse filters, project chips and item rows; graph labels/edge limit/hops/kinds/view/sort/flags and action HUD; relationship Add/navigation/kind/delete; graph legend filters; DB-modal rows/actions; common Font/Theme and top authoring controls. DB-modal and authoring scopes exclude background buttons. Text fields explicitly take keyboard ownership while pointer controls remain available.

See [the current interaction reference](ui_interaction.md) for exact scope, replay gates, capture proof and remaining text-edit/pane/native-Linux boundaries. This is local Development adoption; app VERSION and canonical/production release state are unchanged.


## 2026-10-05 bounded text editing and modal focus

Search, title, body, DB/input-root path, graph edge limit and relationship target now use `kit_ui 0.15.1` bounded editing in Main Edit. Title Enter saves and Escape cancels; body Enter inserts a newline and Ctrl/Cmd+Enter saves; DB Enter confirms and Escape cancels. Search Enter refreshes; numeric Enter uses the existing graph/relationship actions. Numeric fields reject a mixed invalid paste atomically. Only committed search edits trigger debounce invalidation. Opening a DB modal ends title/body editing by existing host policy; closing it returns to Search, not an abandoned text editor.

See [Text editing and modal focus](ui_text_focus.md) for exact ownership, tests and limitations. Text presentation, native IME/platform acceptance, pane composition and wider program adoption remain later slices. Earlier dated milestones describe their historical state.


## 2026-10-05 shared text presentation

Search, title, body, DB/input-root path, graph edge limit and relationship target now adopt `kit_ui 0.16.0` measured text presentation in Main Edit. Six distinct UI-owned slots prevent queued text aliasing. Body editing uses measured scalar hard wrapping and explicit newlines; read-only bodies retain their existing layout. DB editing uses the full draft with horizontal caret-follow instead of an ellipsis string and duplicate byte-selection offsets. Numeric validation and database/session actions retain their owners.

See [Shared text presentation](ui_text_presentation.md) for storage ownership, output gates and residual limits. The next behavioral slice is mixed field/button traversal; native IME/session/clipboard qualification and pane composition follow before wider adoption. Earlier dated milestones remain historical.
