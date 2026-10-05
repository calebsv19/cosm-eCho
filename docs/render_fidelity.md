# Native Render Command Fidelity

Main Edit imports shared `f80f91d`: `kit_render 0.14.6`, `vk_renderer 1.5.0`,
`vk_runtime 0.6.0`, `kit_ui 0.12.0` and `kit_workspace_authoring 0.5.2`.
Vulkan commands now honor translation, signed independent scale, UV cropping
and reversal, and RGBA tint. The authoring HUD also receives the shared 0.5.2
appearance follow-up already used by orChestra and DataLab. DB actions, graph
semantics, hit geometry, input routing and frame state ownership retain their
existing boundaries.

`make render-fidelity-self-test` verifies the selected module bytes against
that committed snapshot, then links the common 1x/2x image harness against the
target-specific copied kit/renderer/runtime archives used by the application.
It defaults to ignored `build/targets/<triple>/render-fidelity/`; override with
`RENDER_FIDELITY_OUTPUT_DIR=<path>` for retained proof. A logged-in macOS
graphics session and Khronos validation layer are required. This gate remains
separate from `make test`, `make run-headless-smoke`, the normal application
resize/restart/capture gate `make vulkan-rollout-self-test`, and the Main Edit
package self-test.

The host rebuilds and copies shared archives for the selected target; after an
import, use `make clean && make` to also rebuild application objects. Later
canonical shared commits are accepted only while the selected module bytes
still match this pinned snapshot. The full command/lifetime contract is in
`third_party/codework_shared/docs/RENDER_COMMAND_FIDELITY.md`.

Main Edit installation is a development checkpoint. Interactive workflow
review, canonical adoption and stable publication remain separate steps.

The 2026-10-04 Main Edit checkpoint passed the clean build, full host regression
suite and headless smoke, plus validation-clean native startup/resize/restart
captures. The common fidelity harness checked 301,545 pixels and 2,106 text
samples at 1x, and 465,019 pixels and 8,424 text samples at 2x, with zero Vulkan
validation warnings or errors. The host's DB/path/mutation/state/graph/package
and visual-artifact regression checks remain part of its separate product proof.

## Font/Theme interaction follow-up

Main Edit now adopts optional shared button focus/capture and rounded Font/Theme control state.
See [Font/Theme interaction](ui_interaction.md) for scope, shortcuts and proof gates.
The composite source pin advances to the interaction snapshot; Vulkan transforms, UVs and tint remain unchanged.
