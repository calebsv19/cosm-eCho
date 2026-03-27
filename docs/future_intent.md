# mem_console Future Intent

Last updated: 2026-03-27

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
