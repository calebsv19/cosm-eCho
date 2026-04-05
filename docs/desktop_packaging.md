# mem_console Desktop Packaging

Last updated: 2026-04-04

## Bundle Contract
- output app: `dist/eCho.app`
- launcher: `Contents/MacOS/mem-console-launcher`
- runtime binary: `Contents/MacOS/mem-console-bin`
- bundled frameworks include Vulkan runtime closure:
  - `Contents/Frameworks/libvulkan.1.dylib`
  - `Contents/Frameworks/libMoltenVK.dylib`
- bundled resources:
  - `Contents/Resources/data/default.sqlite`
  - `Contents/Resources/shared/assets/fonts/*`
  - `Contents/Resources/vk_renderer/shaders/*`
  - `Contents/Resources/shaders/*`

## Make Targets
- local packaging:
  - `make -C mem_console package-desktop`
  - `make -C mem_console package-desktop-smoke`
  - `make -C mem_console package-desktop-self-test`
  - `make -C mem_console package-desktop-refresh`
- release readiness:
  - `make -C mem_console release-contract`
  - `make -C mem_console release-bundle-audit`
  - `make -C mem_console release-sign APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)"`
  - `make -C mem_console release-notarize APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="cosm-notary"`
  - `make -C mem_console release-distribute APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="cosm-notary"`

## Launcher Runtime Contract
- `--print-config` dumps active paths and env configuration.
- `--self-test` verifies app binary, plist, DB seed, shared fonts, Vulkan shader bundles, runtime ICD, and bundled MoltenVK.
- startup logs go to `~/Library/Logs/MemConsole/launcher.log` (tmp fallback).
- launcher runtime root:
  - `MEM_CONSOLE_RUNTIME_DIR=~/Library/Application Support/MemConsole/runtime` (tmp fallback)
- seeded runtime DB path:
  - `CODEWORK_MEMDB_PATH=<runtime>/data/default.sqlite`
- Vulkan runtime env:
  - `VK_RENDERER_SHADER_ROOT=<runtime>/vk_renderer`
  - `VK_ICD_FILENAMES=<runtime>/vk/MoltenVK_icd.json`
  - `VK_DRIVER_FILES=<runtime>/vk/MoltenVK_icd.json`
  - `MOLTENVK_DYLIB=<App>/Contents/Frameworks/libMoltenVK.dylib`

## Validation Flow
1. `make -C mem_console clean && make -C mem_console`
2. `make -C mem_console test`
3. `make -C mem_console run-headless-smoke`
4. `make -C mem_console visual-harness`
5. `make -C mem_console release-bundle-audit`
6. `make -C mem_console package-desktop-refresh`
7. `/Users/<user>/Desktop/eCho.app/Contents/MacOS/mem-console-launcher --print-config`
8. `open /Users/<user>/Desktop/eCho.app`
9. `tail -n 120 ~/Library/Logs/MemConsole/launcher.log`
