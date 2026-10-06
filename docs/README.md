# eCho Docs

This directory tracks the scaffold-oriented documentation lanes for `eCho`.

Repository and source-level identifiers still use `mem_console`.

## Files
- `current_truth.md`: implemented behavior and structure that is live now.
- `future_intent.md`: near and medium-term intended structure/behavior.
- `architecture.md`: subsystem ownership and lifecycle shape.
- `migration.md`: scaffold standardization phase tracker and verification contract.
- `desktop_packaging.md`: `.app` packaging contract, launcher behavior, and validation workflow.
- `main_edit_worktree.md`: persistent implementation worktree, isolated package identity, and adoption gates.
- `memory_check_audit.md`: opt-in fisiCs memory-check audit command and latest clean graph allocation result.
- `render_fidelity.md`: shared Vulkan command contract and host-linked 1x/2x image proof.

## Current Emphasis
- async refresh/runtime-loop hardening is part of the shipped host contract now
- graph inspection is beyond the original phase-3 shell:
  - edge-kind filters
  - node-kind filters
  - project pod overlays
- packaging docs must reflect the current multi-arch Intel staging lane rather than the older single-dist contract


## Font/Theme interaction reference

See [Shared button interaction](ui_interaction.md) for the optional focus,
keyboard and press-origin capture contract, host ownership, current shared pin,
and `make ui-interaction-self-test`. The retained Main Edit inventory now includes runtime/authoring/modal buttons and bounded text fields; pane gestures and text presentation retain explicit host owners.


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
