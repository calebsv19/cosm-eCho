# mem_console Current Truth

Last updated: 2026-04-10

## Program Identity
- Repo/program directory: `mem_console`
- Canonical symbol/file prefix: `mem_console`
- Private planning bucket: `../../docs/private_program_docs/memory_console/`

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
- `make -C mem_console run-data-path-contract-checks`
- `make -C mem_console run-headless-smoke`
- `make -C mem_console visual-harness`
- desktop packaging verification:
  - `make -C mem_console package-desktop`
  - `make -C mem_console package-desktop-smoke`
  - `make -C mem_console package-desktop-self-test`
  - `make -C mem_console package-desktop-refresh`
- release readiness verification:
  - `make -C mem_console release-contract`
  - `make -C mem_console release-bundle-audit`
  - `make -C mem_console release-verify APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)"`
  - `make -C mem_console release-distribute APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="cosm-notary"`
  - `make -C mem_console release-desktop-refresh APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="cosm-notary"`

## Desktop Packaging Contract (Current)
- standardized package + release-readiness lane is implemented:
  - `package-desktop`
  - `package-desktop-smoke`
  - `package-desktop-self-test`
  - `package-desktop-copy-desktop`
  - `package-desktop-sync`
  - `package-desktop-open`
  - `package-desktop-remove`
  - `package-desktop-refresh`
  - `release-contract`
  - `release-clean`
  - `release-build`
  - `release-bundle-audit`
  - `release-sign`
  - `release-verify`
  - `release-verify-signed`
  - `release-notarize`
  - `release-staple`
  - `release-verify-notarized`
  - `release-artifact`
  - `release-distribute`
  - `release-desktop-refresh`
- launcher entrypoint: `tools/packaging/macos/mem-console-launcher`
- dylib bundler entrypoint: `tools/packaging/macos/bundle-dylibs.sh`
- launcher diagnostics:
  - `--print-config`
  - `--self-test`
  - startup logfile: `~/Library/Logs/MemConsole/launcher.log`
- packaged runtime resource lanes:
  - `Contents/Resources/data/default.sqlite` (seed DB)
  - `Contents/Resources/shared/assets/fonts/*`
  - `Contents/Resources/vk_renderer/shaders/*`
  - `Contents/Resources/shaders/*`
- launcher runtime contract now uses writable runtime root:
  - `~/Library/Application Support/MemConsole/runtime` (tmp fallback)
  - seeded DB path: `<runtime>/data/default.sqlite`
  - runtime Vulkan ICD path: `<runtime>/vk/MoltenVK_icd.json`
- release identity:
  - product app name: `eCho.app`
  - bundle id: `com.cosm.echo`
  - notarized artifact lane: `build/release/eCho-<version>-macOS-stable.zip`
  - release chain hardening: `release-staple` now depends on `release-notarize` to guarantee ticket availability before stapling.

## Post-Scaffold Font Size Pass (Current)
- keyboard text zoom is implemented:
  - `Cmd/Ctrl +` => zoom in
  - `Cmd/Ctrl -` => zoom out
  - `Cmd/Ctrl 0` => reset zoom
- zoom state is persisted in UI prefs (`MCFG` schema `v11`) via `text_zoom_step`.
- startup and DB-switch paths reapply theme/font/zoom and UI density consistently.

## Data Path Contract Progress (Current)
- `S1` foundation is active:
  - explicit runtime contract fields: `input_root`, `output_root`, `active_db_path`
  - app prefs (`MCAP`) now persist roots + active DB with v1 backward compatibility
  - startup and DB-switch keep these lanes synchronized
- `S2` input-root control lane is active:
  - left-pane root status line + `EDIT ROOT` / `BROWSE ROOT` controls
  - keyboard-first controls:
    - `Cmd/Ctrl+I` => open input-root edit modal
    - `Cmd/Ctrl+B` => open native folder chooser for input root
- `S3` DB open/create matrix is active:
  - open/switch (`LOAD DB`) = `reference` mode (exact entered path)
  - create (`NEW DB`) supports:
    - bare DB name => create under `input_root` with `.sqlite`
    - explicit path => create at explicit path (`.sqlite` implied when missing)
- `S4` output-root migration is active:
  - app prefs path now resolves from active `output_root` (`<output_root>/mem_console.app.pack`)
  - startup load keeps compatibility fallback to legacy `~/.local/share/mem_console/mem_console.app.pack` if present
- `S5` verification closeout lane is active:
  - deterministic contract checker: `tests/run_data_path_contract_checks.sh`
  - checker is wired into `make -C mem_console test`
- `S6` DB picker usability refinement is active:
  - `LOAD DB` modal includes discovered `.sqlite` list from `input_root`
  - active DB summary visible in modal
  - click and `Up/Down` selection supported before confirm

## Wrapper Contract State
- cross-program wrapper initiative status:
  - `W0` complete
  - `W1` complete for `mem_console`
  - `W2` complete for `mem_console`
- execution note:
  - `../../docs/private_program_docs/memory_console/2026-04-02_mem_console_w1_w2_wrapper_hardening.md`

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
- large-file decomposition follow-up (2026-04-09) split oversized lanes into helper modules:
  - `src/runtime/mem_console_prefs_app_io.c` + `src/runtime/mem_console_prefs_app_io_internal.h` now own app-prefs path/load/save I/O helpers.
  - `src/ui/graph/mem_console_ui_graph_layout_focus_helpers.c` + `.h` now own focus-anchor layout and visible-edge filtering helpers.
