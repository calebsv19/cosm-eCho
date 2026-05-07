# mem_console Desktop Packaging

Last updated: 2026-05-04

## Bundle Contract
- output app:
  - local default lane still exposes `dist/eCho.app`
  - multi-arch/release lane uses `build/targets/<target-triple>/dist/eCho.app`
- launcher: `Contents/MacOS/mem-console-launcher`
- runtime binary: `Contents/MacOS/mem-console-bin`
- bundled frameworks include Vulkan runtime closure:
  - `Contents/Frameworks/libvulkan.1.dylib`
  - `Contents/Frameworks/libMoltenVK.dylib`
- bundled resources:
  - `Contents/Resources/data/default.sqlite`
  - optional `Contents/Resources/AppIcon.icns` when `PACKAGE_APP_ICON_SRC` or `PACKAGE_APP_ICONSET_SRC` is provided
  - `Contents/Resources/shared/assets/fonts/*`
  - `Contents/Resources/vk_renderer/shaders/*`
  - `Contents/Resources/shaders/*`

Default local icon store:
- `mem_console/tools/packaging/macos/local_app_icon/AppIcon.icns`
- `mem_console/tools/packaging/macos/local_app_icon/AppIcon.iconset`

## Make Targets
- local packaging:
  - `make -C mem_console package-desktop`
  - `make -C mem_console package-desktop-smoke`
  - `make -C mem_console package-desktop-self-test`
  - `make -C mem_console package-desktop-refresh`
  - `make -C mem_console package-desktop-refresh PACKAGE_APP_ICON_SRC="/absolute/path/to/echo.icns"`
  - `make -C mem_console package-desktop-refresh PACKAGE_APP_ICONSET_SRC="/absolute/path/to/echo.iconset"`

Plain `make -C mem_console package-desktop-refresh` and `package-desktop-self-test` now look in that local store first. The local icon store is gitignored so refreshed icon copies do not dirty the normal repo worktree.
- release readiness:
  - `make -C mem_console release-contract`
  - `make -C mem_console release-bundle-audit`
  - `make -C mem_console release-sign APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)"`
  - `make -C mem_console release-notarize APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="cosm-notary"`
  - `make -C mem_console release-distribute APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="cosm-notary"`

Multi-arch release lane:
- target contract:
  - `TARGET_OS`
  - `TARGET_ARCH`
  - `TARGET_VARIANT`
  - `TARGET_TRIPLE`
- Intel/ad-hoc examples:
  - `make -C mem_console release-contract TARGET_ARCH=x86_64 BUILD_TOOLCHAIN=clang PACKAGE_TOOLCHAIN=clang`
  - `HOME=/private/tmp/codex-mem-console-x86-home make -C mem_console package-desktop-self-test TARGET_ARCH=x86_64 BUILD_TOOLCHAIN=clang PACKAGE_TOOLCHAIN=clang`
  - `HOME=/private/tmp/codex-mem-console-x86-home make -C mem_console release-artifact TARGET_ARCH=x86_64 BUILD_TOOLCHAIN=clang PACKAGE_TOOLCHAIN=clang`

## Launcher Runtime Contract
- `--print-config` dumps active paths and env configuration.
- `--self-test` verifies app binary, plist, DB seed, shared fonts, Vulkan shader bundles, runtime ICD, and bundled MoltenVK.
- when icon inputs are provided, packaging bundles `AppIcon.icns` and declares `CFBundleIconFile=AppIcon`.
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
- Intel shader follow-up:
  - launcher now seeds runtime copies for both `runtime/shaders` and `runtime/vk_renderer`
  - `VK_RENDERER_SHADER_ROOT` points at the runtime root so relative shader lookups remain valid on staged Intel handoff packages

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

Note:
- a fresh clone will still need an `AppIcon.icns` copied into `tools/packaging/macos/local_app_icon/` before plain packaging picks it up, because that lane is intentionally ignored.
