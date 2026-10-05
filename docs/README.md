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
and `make ui-interaction-self-test`. This is the retained Main Edit Font/Theme
surface only; ordinary runtime controls, text entry and panes retain their own
input paths.
