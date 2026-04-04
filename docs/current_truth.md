# mem_console Current Truth

Last updated: 2026-04-02

## Program Identity
- Repo/program directory: `mem_console`
- Canonical symbol/file prefix: `mem_console`
- Private planning bucket: `docs/private_program_docs/memory_console/`

## Top-Level Layout (Current)
- required scaffold floor:
  - `docs/`
  - `src/`
  - `include/`
  - `tests/`
  - `build/` (created by build workflow)
- currently present optional/support lanes:
  - `data/`
  - `demo/`
  - `tmp/`
  - `third_party/`
  - `ide_files/`

## Subsystem Lanes (Current)
- `src/app/`: app bootstrap, loop coordination, action/event dispatch.
- `src/runtime/`: runtime state/prefs/refresh loop behavior.
- `src/db/`: memory DB reads/writes/filter paths.
- `src/ui/`: UI frame/chrome/sections.
- `src/ui/graph/`: graph viewport, draw, controls, camera, layout.
- `src/layout/`: pane/layout config and split behavior.

## Lifecycle Entry (Current)
- Entrypoint is a thin delegate in `src/app/mem_console.c`:
  - `return mem_console_app_main(argc, argv);`
- Canonical lifecycle wrapper is now active in `src/app/mem_console_app_main.c` with explicit stage handlers:
  - `mem_console_app_bootstrap`
  - `mem_console_app_config_load`
  - `mem_console_app_state_seed`
  - `mem_console_app_subsystems_init`
  - `mem_console_runtime_start`
  - run-loop stage via `mem_console_app_run_loop(...)`
  - `mem_console_app_shutdown`
- Stage-order guardrails are enforced through explicit lifecycle transitions.
- wrapper diagnostics normalization is now active (`W2`):
  - lifecycle stage-order violations now include both stage label and function context.
  - wrapper-level error taxonomy is logged for lifecycle boundary failures.
  - final wrapper exit summary now logs stage, exit code, dispatch status, and wrapper error code.

## Verification Contract (Current)
- `make -C mem_console clean && make -C mem_console`
- `make -C mem_console test`
- `make -C mem_console run-headless-smoke`
- `make -C mem_console visual-harness`
- desktop packaging verification:
  - `make -C mem_console package-desktop`
  - `make -C mem_console package-desktop-smoke`
  - `make -C mem_console package-desktop-self-test`
  - `make -C mem_console package-desktop-refresh`

## Desktop Packaging Contract (Current)
- standardized package target lane is implemented:
  - `package-desktop`
  - `package-desktop-smoke`
  - `package-desktop-self-test`
  - `package-desktop-copy-desktop`
  - `package-desktop-sync`
  - `package-desktop-open`
  - `package-desktop-remove`
  - `package-desktop-refresh`
- launcher entrypoint: `tools/packaging/macos/mem-console-launcher`
- launcher diagnostics:
  - `--print-config`
  - `--self-test`
  - startup logfile: `~/Library/Logs/MemConsole/launcher.log`
- packaged runtime resource lanes:
  - `Contents/Resources/data/default.sqlite` (seed DB)
  - `Contents/Resources/shared/assets/fonts/*`
  - `Contents/Resources/vk_renderer/shaders/*`
  - `Contents/Resources/shaders/*`
- launcher seeds writable user DB at `~/.local/share/mem_console/default.sqlite` from bundled `default.sqlite` on first launch (unless `CODEWORK_MEMDB_PATH` override is provided).

## Post-Scaffold Font Size Pass (Current)
- keyboard text zoom is implemented:
  - `Cmd/Ctrl +` => zoom in
  - `Cmd/Ctrl -` => zoom out
  - `Cmd/Ctrl 0` => reset zoom
- zoom state is persisted in UI prefs (`MCFG` schema `v11`) via `text_zoom_step`.
- startup and DB-switch paths reapply theme/font/zoom and UI density consistently.

## Wrapper Contract State
- cross-program wrapper initiative status:
  - `W0` complete
  - `W1` complete for `mem_console`
  - `W2` complete for `mem_console`
- execution note:
  - `../docs/private_program_docs/memory_console/2026-04-02_mem_console_w1_w2_wrapper_hardening.md`

## Source Hygiene (Current)
- historical `_backup` directories were removed from active `src/` lanes.
- migration snapshots are preserved at:
  - `tmp/migration_backups/src/app/_backup/`
  - `tmp/migration_backups/src/db/_backup/`
  - `tmp/migration_backups/src/runtime/_backup/`
  - `tmp/migration_backups/src/ui/graph/_backup/`
- project `.gitignore` now covers transient lanes used in migration/runtime tooling:
  - `tmp/`
  - `ide_files/`
  - `demo/*.sqlite`
