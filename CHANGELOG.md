# Changelog

## [Unreleased]

## [v2.0 Release - 2026/6/28]

### Added
- **`Controls::Edit`** — EDIT control customization with dark theme matching `Controls::Button`:
  - Custom `WinProc` with `WM_ERASEBKGND` (dark background), `WM_PAINT` (flat border), `WM_NCPAINT` passthrough
  - `Subclass` / `Unsubclass` API, removes `WS_EX_CLIENTEDGE`
  - `AutoHideScrollbar` — hides vertical scrollbar when text fits the client area
  - Applied to input fields in `PromptDialogProc` and `MessageBoxDialogProc2`
- **`Controls::Tooltip`** — Public reusable tooltip popup control:
  - `Create`, `SetText`, `SetPosition`, `Destroy` API
  - Dark-themed round-cornered window (`RoundRect` border + clip region for text)

- **`CtrlDraw::DrawFrame`** — Utility to draw a flat (non-rounded) rectangle frame with 1px border
- **`DynamicColorPicker`** — New non-blocking dynamic color picker dialog:
  - HSL colour wheel (2D canvas: saturation × lightness)
  - Hue strip + alpha strip sliders
  - RGBA / HSL / HEX numeric input fields
  - Old vs new color preview
  - Screen color picker (eyedropper tool)
  - Runs in a separate thread; uses `Controls::Button::Subclass` and `Controls::Edit::Subclass`
  - Factory function: `CreateDynamicColorPicker(title, r, g, b, a, hParent)`

### Changed
- **Dark theme consistency** — All hardcoded `RGB()` colors in `PromptDialogProc`,
  `MessageBoxDialogProc2`, `DynamicProgressBar`, and `DynamicSlider` replaced with
  `theme.*` colors
- **`PromptDialogProc`** — Now calls `Controls::InitWindowColor`, `Controls::Button::Subclass`
  on OK/Cancel buttons, and `Controls::Edit::Subclass` on the input field. All color
  handlers (`WM_CTLCOLOREDIT`, `WM_CTLCOLORSTATIC`, `WM_CTLCOLORBTN`) use theme colors.
- **`MessageBoxDialogProc2`** — Added `WM_CTLCOLORSTATIC` handler (for `ES_READONLY` EDIT),
  all color handlers use theme colors. `Controls::Edit::Subclass` applied.
- **`DynamicSlider`** — Removed static `Value:` label; current value shown as a tooltip
  thumb when dragging. Slider track moved up to fill the space.
- **Build flags** — Added `-ldwmapi` linker flag (for `DwmSetWindowAttribute`)
