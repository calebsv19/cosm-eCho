# Pane composition in mem_console

Echo navigation/detail/graph use a shared composition snapshot after existing CorePane layout solve. Nested pane clips also constrain button registration. A caller-owned pane pointer owner prevents a press in one section from becoming a release gesture in another and cancels on modal/authoring/splitter takeover. Existing product headers stay inside content, with no extra title row.

The shared source pin is `86037d7cb82c8acb99bd30c9ddec458c7bda8223` (`kit_pane 0.4.0`, `kit_ui 0.17.0`).
CorePane still owns topology/constraints/drag math; module dispatch, workspace
transactions/persistence and purpose remain product-owned. Mixed control focus
order is available in the kit but has not replaced product traversal. Native
caret anchoring is adopted in existing editable fields using measured text
geometry and window/render mapping; session eligibility remains host-owned.

Verification: clean build, production-linked `make ui-text-presentation-self-test
vulkan-rollout-contract`, existing product/headless/native lifecycle and isolated
Main Edit package gates. Shared pane tests include exact 1x/2x SDL clip pixels;
Orchestra also has captured native Vulkan pane overflow proof.

This is a Development adoption in retained Main Edit. Canonical source, app
VERSION and stable package remain separate. Next build the reusable pane-host
lifecycle/dispatch and transactional splitter handoff; docking/module reassignment,
native IME human acceptance and wider rollout remain separate boundaries.
