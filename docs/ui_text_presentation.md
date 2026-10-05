# Echo shared text presentation

Main Edit Development reference; canonical source and release VERSION remain unchanged.

Search, title, body, DB/input-root path, graph edge limit and relationship target adopt the shared presentation through `src/ui/mem_console_ui_text_presentation.c`. Six distinct UI-owned slots prevent queued text aliasing. Body editing uses measured scalar hard wrapping and explicit newlines; read-only bodies retain their existing layout. DB editing uses the full draft with horizontal caret-follow instead of an ellipsis string and duplicate byte-selection offsets. Numeric validation and database/session actions retain their owners.

`kit_ui 0.16.0`, accepted source `e469445`, adds an optional measured presentation companion to the bounded editing/focus contract. Caller-owned presentation storage retains display and row text through submission. The host supplies its actual font measurement in the viewport coordinate space; the kit computes scalar-safe rows, caret/selection geometry, transient replacement preedit with underline, horizontal caret-follow scrolling and multiline hard wrapping with explicit newlines. Painted rows and click placement use the same geometry. Field clicks collapse selection and cancel preedit through the shared editor, including when the caret byte position is unchanged. Body wheel scrolling retains its measured viewport and visible scrollbar. Queued rendering restores the enclosing clip and rolls recording back on failure; the optional SDL adapter restores clip, draw color and blend state.

Core owns domain state and persistence. The kit owns reusable edit/presentation mechanics; hosts own field eligibility, font/style and viewport selection, text-input sessions, button/field ownership, save/cancel actions and durable data. Generic kit code adds no SDL dependency. Row origins are top-left; the queued adapter converts to kit_render's centered text origin. DataLab's font painter uses TTF when available and its existing bitmap fallback otherwise; it measures the selected path. Font zoom and output DPI are applied once in the host's existing coordinate path.

This bounded presentation is not a general document editor. Unicode scalar safety does not claim grapheme navigation, bidi, shaping or multilingual font coverage. Native IME candidate-window placement and OS clipboard/IME workflow acceptance, mixed field/button traversal, text undo, nested modal stacks, broader panes and native Linux/Windows acceptance remain open. The kit exposes capacity failure rather than silently truncating. Read-only prose layout remains host-owned.

## Verification

`make ui-text-presentation-self-test` runs the actual production-linked field adapters and retained button/text/modal replay. Unicode replacement preedit, selection, measured hit and queued storage lifetime are checked; Echo covers all six slots and explicit multiline rows. DataLab's real SDL painter is exercised at 1x/2x, including caret pixels and clip/blend restoration. Native Vulkan captures use actual Orchestra/Echo host frames at 1x/2x with validation enabled; this is separate from OS-native IME acceptance and human workflow review. Run product/headless, Vulkan lifecycle and isolated Main Edit packaging gates before refreshing the separate comparison bundle.

See the imported shared `docs/UI_TEXT_PRESENTATION_CONTRACT.md` and [Text editing and modal focus](ui_text_focus.md). Next qualify mixed field/button focus traversal, then native text-input sessions/candidate placement/clipboard across the trio. Pane composition follows; wider programs adopt one surface at a time with an immutable shared pin and their own output/product gates.

## 2026-10-05 pane composition follow-on

Echo navigation/detail/graph use a shared composition snapshot after existing CorePane layout solve. Nested pane clips also constrain button registration. A caller-owned pane pointer owner prevents a press in one section from becoming a release gesture in another and cancels on modal/authoring/splitter takeover. Existing product headers stay inside content, with no extra title row.

See [pane composition adoption](ui_pane_composition.md). Pane composition now has
a shared foundation; the next boundary is pane-host lifecycle/dispatch and
transactional splitter/focus takeover. Mixed traversal is kit-only preparation;
native caret anchoring is integrated, with OS IME acceptance still pending.
