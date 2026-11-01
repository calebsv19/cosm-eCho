# Shared UI rendering and text in Main Edit

Last updated: 2026-10-31

The retained Main Edit uses vk_renderer 1.6.0 / vk_runtime 0.6.0 and kit_ui
0.18.0. Its original accepted renderer source baseline is
`854b51ff57c756b4565021fe759c27a459efbfd3`. The font refinement imports only
kit_render 0.17.0 as an exact, bounded development module snapshot;
[the source pin](../tools/text-render-source-pin.json) binds all 25 module files.
This is uncommitted source development, with canonical and installed adoption
remaining separate. Other shared modules keep their prior pins.

Text opts into the shared native-pixel policy. Glyphs rasterize at the drawable
scale, keep their native texel dimensions and snap origins to drawable pixels.
Logical measurement encloses the same raster used for painting. This avoids
stretching a high-density raster into low-density font bounds. Font families,
preset order, light hinting, kerning, roles/tiers and the centered vertical anchor
remain unchanged. Existing non-opt-in shared renderer users retain their policy.
Explicit destination resizing still scales text.

Startup prepares every font role/tier. Font and zoom shortcuts and the existing
Font/Theme authoring controls prepare a complete replacement before changing the
active settings. Loader failures preserve the working settings and native rasters.
Authoring retains the entry font context until Apply or Cancel: Cancel restores
its exact font bank without opening files and preserves an earlier unsaved pane
preference; Apply releases the bank and uses the existing accepted-only preference
writer. Database switching also uses guarded font preparation when applying the
new database's UI preferences. A font failure retains the already switched data
and working presentation and reports the failure.

The existing authoring chord, shortcuts, Apply/Cancel controls, shipped presets
and supported zoom range remain unchanged. Custom font/theme creation controls
remain their existing disabled/stub surfaces.

## Verification

Build into a fresh contained output root, preserving existing consumer builds:

```sh
make PREPARATION_ROOT=/private/tmp/echo-text-proof-UNIQUE \
     BUILD_ROOT=/private/tmp/echo-text-proof-UNIQUE \
     all text-settings-harness
```

`text-settings-harness` builds a native lifecycle test. Run its generated
`mem_console_text_settings_test` with a fresh output-directory argument, an
isolated runtime/cache and the Vulkan validation layer. It uses task-owned fixture
databases and loader fault injection. It covers startup, shortcut and preview
failure, exact Cancel, dirty preservation, Apply/save/reload, database preference
switches, both presets, all ten zoom steps, all 25 role/tier pairs and compact resize.

The existing UI interaction test covers the six editable field presentations,
measured hit geometry, Unicode scalar/preedit storage, pane input ownership,
splitters and lifecycle cancellation. The maintained renderer verifier checks
exact source pins, startup/resize/restart and zero validation warnings/errors.
Its layer manifest uses an absolute library path, avoiding a global dylib override.
Main Edit package self-test checks package isolation without refreshing Desktop.

October 31 qualification passed at actual 1× and 2× drawable sizes. The pixel
oracle also passed cold/cached/wrapped text at logical-to-drawable ratios 1,
1.25, 1.5 and 2. Targeted ASan/UBSan covered the changed native font/settings
owners. Fractional ratios are controlled renderer tests, not physical monitor
transitions. Glyph coverage, grapheme/bidi/shaping, native IME/clipboard workflows,
Linux/Windows and human visual acceptance remain separate checks.
