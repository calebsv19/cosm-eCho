# eCho Docs

This directory tracks the scaffold-oriented documentation lanes for `eCho`.

Repository and source-level identifiers still use `mem_console`.

## Files
- `current_truth.md`: implemented behavior and structure that is live now.
- `future_intent.md`: near and medium-term intended structure/behavior.
- `architecture.md`: subsystem ownership and lifecycle shape.
- `migration.md`: scaffold standardization phase tracker and verification contract.
- `desktop_packaging.md`: `.app` packaging contract, launcher behavior, and validation workflow.
- `memory_check_audit.md`: opt-in fisiCs memory-check audit command and latest clean graph allocation result.

## Current Emphasis
- async refresh/runtime-loop hardening is part of the shipped host contract now
- graph inspection is beyond the original phase-3 shell:
  - edge-kind filters
  - node-kind filters
  - project pod overlays
- packaging docs must reflect the current multi-arch Intel staging lane rather than the older single-dist contract
