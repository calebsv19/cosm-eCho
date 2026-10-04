# Shared Rounded UI in Main Edit

Last updated: 2026-10-04

The managed shared snapshot is `e5887348657f782e6094cdeeb095259dedb32bf8`.
`kit_ui 0.11.3` supplies existing button semantics and compact appearance;
`kit_render 0.14.5` forwards positive radii; `vk_renderer 1.4.0` draws native
rounded solid geometry and keeps replaced vertex storage until frame completion.
`vk_runtime 0.6.0` continues to own Vulkan device/validation lifecycle.

The migrated surface is the common DB/browser/graph inspector buttons. This is a contained rendering adoption,
with application actions and interaction bounds still owned by the host.
Button labels are horizontally centered using the shared text measurement path;
font role/tier and the existing vertical anchor remain unchanged.

## Verification

Run in the retained Main Edit checkout:

```sh
make
make test
make run-headless-smoke
make visual-harness
make vulkan-rollout-self-test
make package-desktop-main-edit-self-test
```

The shared GPU image gate is:

```sh
make -C third_party/codework_shared/kit/kit_ui KIT_RENDER_ENABLE_VK=1 test-rounded-vk
```

It checks actual pixels, clipping, nested borders, radius clamping, alpha, shared
button appearance, drawable scale, frame-buffer growth, and fence reuse. GPU and
package boot checks require the macOS user session and validation layer. Unit and
headless checks remain separate from interactive acceptance.

## Visual Review

Launch `eCho Main Edit.app`. Inspect browser filter controls, the DB picker,
and graph inspector controls. Hover/press and disabled states use shared button
semantics. For contained visual review, use a seeded fixture DB through
`demo/reset_visual_graph_fixture.sh` and `--visual-review`; keep production DBs
out of test runs.

No app version bump or canonical source adoption is part of this Main Edit slice.
