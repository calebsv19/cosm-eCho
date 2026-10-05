# Shared button interaction in eCho

The retained Main Edit lane adopts `kit_ui 0.14.1` and
`kit_workspace_authoring 0.6.1`. The accepted shared source snapshot is
`b1c67d7e1b78b826c81269a6531706fc1f29ea86`. `kit_render 0.14.6`, `vk_renderer 1.5.0` and
`vk_runtime 0.6.0` retain the proven transform/textured-quad rendering contract.

## Adopted surfaces

Left root/DB/load/refresh controls, browse filters, project chips and item rows; graph labels/edge limit/hops/kinds/view/sort/flags and action HUD; relationship Add/navigation/kind/delete; graph legend filters; DB-modal rows/actions; common Font/Theme and top authoring controls. DB-modal and authoring scopes exclude background buttons. Text fields explicitly take keyboard ownership while pointer controls remain available.

## Input, drawing and ownership

A press owns its enabled control until matching release or cancellation. Release
inside that same visible control activates its existing host action. Release-only,
cross-control and outside releases cannot activate. Disabled/removed controls lose
focus and capture; outstanding owned releases remain consumed across scope changes.
Tab/Shift-Tab navigate the active visible controls in registration order;
Enter/Space activate on matching key-up with repeat suppressed. A blue underline
identifies focus. Ctrl/Alt/GUI shortcuts and unfocused Enter retain host behavior.
Focus loss, hide/minimize, resize and quit cancel interaction ownership without
replacing the app lifecycle owner.

The UI sibling `src/ui/mem_console_ui_surface.c` publishes clipped controls from the real frame drawing paths. Events queue activations in order; the host claims one per frame and requests further frames while pending. A competing keyboard action defers the queued button instead of dropping it. `src/ui/mem_console_ui_text_frame.c` copies transient captions into a bounded 256 KiB UI-frame arena so every queued label survives submission. Database operations, previews and accepted-only persistence retain their original host owners. Native output captures exercise the ordinary graph HUD at both scales.

The shared snapshot retains bounded semantic keys, opaque handles, geometry and
press/queue state. It owns no product action, widget tree, label storage, pane
layout, database or persistence. Text/caret/clipboard/IME engines and canvas/pane
or scrollbar gestures remain distinct host contracts.

## Verification and adoption

Run sequentially in this Main Edit checkout:

```sh
make clean
make ui-interaction-self-test vulkan-rollout-contract
make test run-headless-smoke
make vulkan-rollout-self-test
make package-desktop-main-edit-self-test
```

The source check compares the adopted module bytes with the immutable accepted
commit. `tools/verify-vulkan-rollout.py --require-current-canonical` additionally
checks the mutable upstream checkout when intentionally assessing a new adoption.
Independent upstream development cannot silently enter this pinned program.

Production-linked event/frame replays cover representative real actions, modal
exclusion, disabled controls, matching releases and keyboard ownership. DataLab
also runs real picker catalog/root/file effects at 1x/2x and checks SDL marker
pixels. Native Vulkan output, lifecycle/package checks and human interactive
review remain separately identified evidence. Broad regressions do not constitute
an exhaustive visual review of every button.

Local proof and delivery readback are retained in
`_private_workspace_artifacts/ui_unification/surfaces_20261004/`. The comparison
Main Edit app uses isolated identity/state namespaces. Canonical adoption, app
VERSION changes, public releases and native Linux qualification remain later work.

The shared [interaction contract](../third_party/codework_shared/docs/UI_INTERACTION_CONTRACT.md)
defines the optional engine and snapshot semantics. The next architecture boundary
is a separate text-edit/IME and modal-focus contract, followed by pane composition
and wider program adoption after the trio's human workflow review.
