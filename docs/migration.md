# mem_console Scaffold Migration

Last updated: 2026-03-27

Primary private plan:
- `docs/private_program_docs/memory_console/2026-03-26_mem_console_scaffold_standardization_switchover_plan.md`

## Phase Status
| Phase | Status | Notes |
| --- | --- | --- |
| `MC-S0` | complete | baseline audit and compile baseline captured |
| `MC-S1` | complete | scaffold-required floor files/directories created |
| `MC-S2` | complete | verification contract targets added to `Makefile` |
| `MC-S3` | complete | lifecycle/symbol lock alignment applied (`mem_console_app_main`) |
| `MC-S4` | complete | active source `_backup` lanes moved to `tmp/migration_backups/` and removed from `src/` |
| `MC-S5` | complete | stabilization/docs/matrix close-out + temp-lane ignore/commit-policy alignment |

## Standard Verification Commands
- `make -C mem_console clean && make -C mem_console`
- `make -C mem_console test`
- `make -C mem_console run-headless-smoke`
- `make -C mem_console visual-harness`

## Notes
- `run-headless-smoke` is intentionally non-interactive and validates CLI smoke behavior without opening a windowed UI session.
- `visual-harness` is a build-gate alias that ensures the UI binary is available for manual validation runs.
- `MC-S4` backup quarantine path:
  - `tmp/migration_backups/src/...`
- `.gitignore` now includes transient/generated lanes used during scaffold migration:
  - `tmp/`
  - `ide_files/`
  - `demo/*.sqlite`
- scaffold completion commit naming policy:
  - after explicit user confirmation, use commit title: `Project Scaffold Standardization`

## Post-Scaffold Font Size Pass (2026-03-27)
- status: complete
- implemented:
  - text zoom shortcuts (`Cmd/Ctrl +`, `Cmd/Ctrl -`, `Cmd/Ctrl 0`)
  - persisted zoom step (`text_zoom_step`) in prefs schema `v11`
  - startup and DB-switch reapply of theme/font/zoom+density
- wrap-up commit title policy for this pass:
  - `Post-Scaffold Font Size Standardization`
