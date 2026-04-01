# mem_console Desktop Packaging

Last updated: 2026-04-01

## Bundle Contract
- output app: `dist/MemConsole.app`
- launcher: `Contents/MacOS/mem-console-launcher`
- runtime binary: `Contents/MacOS/mem-console-bin`
- bundled resources:
  - `Contents/Resources/data/default.sqlite`
  - `Contents/Resources/shared/assets/fonts/*`
  - `Contents/Resources/vk_renderer/shaders/*`
  - `Contents/Resources/shaders/*`

## Make Targets
- `make -C mem_console package-desktop`
- `make -C mem_console package-desktop-smoke`
- `make -C mem_console package-desktop-self-test`
- `make -C mem_console package-desktop-copy-desktop`
- `make -C mem_console package-desktop-sync`
- `make -C mem_console package-desktop-open`
- `make -C mem_console package-desktop-remove`
- `make -C mem_console package-desktop-refresh`

## Launcher Runtime Contract
- `--print-config` dumps active paths and env configuration.
- `--self-test` verifies app binary, plist, DB seed, shared fonts, and Vulkan shader bundles.
- startup logs go to `~/Library/Logs/MemConsole/launcher.log` (tmp fallback).
- launcher seeds `~/.local/share/mem_console/default.sqlite` from bundled `default.sqlite` when user DB does not yet exist.
- default DB env for runtime:
  - `CODEWORK_MEMDB_PATH=~/.local/share/mem_console/default.sqlite`
- default shader root env:
  - `VK_RENDERER_SHADER_ROOT=<App>/Contents/Resources`

## Validation Flow
1. `make -C mem_console clean && make -C mem_console`
2. `make -C mem_console test`
3. `make -C mem_console run-headless-smoke`
4. `make -C mem_console visual-harness`
5. `make -C mem_console package-desktop-self-test`
6. `make -C mem_console package-desktop-refresh`
7. `/Users/<user>/Desktop/MemConsole.app/Contents/MacOS/mem-console-launcher --print-config`
8. `open /Users/<user>/Desktop/MemConsole.app`
9. `tail -n 120 ~/Library/Logs/MemConsole/launcher.log`
