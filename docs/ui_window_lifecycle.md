# Window and fullscreen lifecycle

The Main Edit window adapter adopts the shared SDL observation/mapping contract
and corrected Vulkan frame submission/recovery. Logical window and drawable
sizes are tracked independently. F11 toggles desktop fullscreen; native window
controls remain available. Native geometry/display/visibility/focus boundaries
cancel stale pane/control ownership and restore active splitter draft state.
Hidden/minimized windows defer submits; show/restore rebuild geometry and resume.

Accepted module versions: kit_ui 0.18.0, vk_renderer 1.6.0, vk_runtime 0.6.0,
kit_render 0.14.6 and kit_pane 0.5.0. The Vulkan context borrows its host window;
all structure consumers require a clean rebuild. Use exact shared source pins,
not version equality, to distinguish this contract from unrelated shared work.

The opt-in CODEWORK_WINDOW_LIFECYCLE_PROOF output directory drives bounded native
resize/fullscreen/hide/minimize/restore qualification and captures actual frames.
It is inactive in routine use. Main Edit comparison packaging is separate from
canonical source, stable Desktop apps and release/publication.

Echo observes actual drawable dimensions even when logical dimensions stay constant. Opt-in native proof uses a task-owned database/preferences root. Pending redraw survives suspension.
