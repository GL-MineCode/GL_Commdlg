# GLDLG::Controls — Custom Win32 Controls

The `GLDLG::Controls` namespace provides dark-themed custom Win32 controls for use
in your own dialog windows. All controls follow a unified color scheme defined by
the `GLDLG::Theme` struct.

## Quick Reference

| Control   | Header | Description |
|-----------|--------|-------------|
| `Button`  | Subclass a standard `BUTTON` | Dark round-rect button |
| `Edit`    | Subclass a standard `EDIT`   | Dark flat-bordered edit field with auto-hide scrollbar |
| `Tooltip` | Create a tooltip popup       | Lightweight self-painted tooltip with rounded corners |

---

## Theme

The six theme colors can be queried or overridden at any time:

```cpp
// Get current theme
const auto &t = GLDLG::GetTheme();

// Set a custom theme (ColorRGBA values)
GLDLG::SetTheme({
    {230, 230, 239},   // Text           — #E6E6EF
    {56,  56,  66},    // ControlFrame   — #383842
    {26,  26,  30},    // PrimaryBackground  — #1A1A1E
    {39,  39,  46},    // SecondaryBackground — #27272E
    {66,  73,  73},    // PrimaryForeground   — #424949
    {72,  84,  102}    // SecondaryForeground — #485466
});
```

`ColorRGBA` is a simple struct: `{uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255}`.

| Token | Default | Used For |
|-------|---------|----------|
| `Text` | `#E6E6EF` | Labels, control text |
| `ControlFrame` | `#383842` | Borders of all controls |
| `PrimaryBackground` | `#1A1A1E` | Dialog window background |
| `SecondaryBackground` | `#27272E` | Control interior (button face, edit field) |
| `PrimaryForeground` | `#424949` | Button hover state |
| `SecondaryForeground` | `#485466` | Disabled state |

---

## Controls::Button

Subclasses a standard Win32 `BUTTON` to draw a custom dark round-rect button.
Handles all visual states (normal, hover, pressed, disabled).

### Usage

```cpp
#include <windows.h>
#include "GL_Commdlg.hpp"

// 1. Create a regular button
HWND hBtn = CreateWindowW(L"BUTTON", L"Click Me",
    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
    x, y, 80, 30, hParent, (HMENU)1001, hInst, nullptr);

// 2. Subclass it
GLDLG::Controls::Button::Subclass(hBtn);

// (Optional) set a font
SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
```

### API

| Function | Description |
|----------|-------------|
| `Button::Subclass(hWnd)` | Replace the window proc with the custom one; sets `BS_OWNERDRAW` and stores the original proc |
| `Button::Unsubclass(hWnd)` | Restore the original window proc and free the `WinData` |

### Visual States

| State    | Background | Border |
|----------|-----------|--------|
| Normal   | `SecondaryBackground` | `ControlFrame`, 5px round corners |
| Hover    | `PrimaryForeground` | `ControlFrame` |
| Pressed  | `PrimaryForeground` at 50% brightness | `ControlFrame` |
| Disabled | `SecondaryForeground` | `ControlFrame`, text at 50% brightness |

---

## Controls::Edit

Subclasses a standard `EDIT` to match the dark theme.
Removes the default `WS_EX_CLIENTEDGE` 3D border and draws a flat 1px border
via custom `WM_PAINT`. The scrollbar is automatically hidden when all text fits.

### Usage

```cpp
// 1. Create an EDIT control (without WS_EX_CLIENTEDGE)
HWND hEdit = CreateWindowW(L"EDIT", L"some text",
    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
    x, y, 200, 24, hParent, (HMENU)1002, hInst, nullptr);

// 2. Subclass it (WS_EX_CLIENTEDGE is removed automatically)
GLDLG::Controls::Edit::Subclass(hEdit);

// 3. Set a font (after subclassing — this triggers AutoHideScrollbar)
SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
```

### Parent Window Requirements

The parent dialog must handle `WM_CTLCOLOREDIT` (and `WM_CTLCOLORSTATIC` for
read-only edits) to set the text color:

```cpp
case WM_CTLCOLOREDIT:
{
    HDC hdc = (HDC)wParam;
    SetBkColor(hdc, GLDLG::GetTheme().SecondaryBackground.ToCOLORREF());
    SetTextColor(hdc, GLDLG::GetTheme().Text.ToCOLORREF());
    static HBRUSH hBrush = CreateSolidBrush(
        GLDLG::GetTheme().SecondaryBackground.ToCOLORREF());
    return (LRESULT)hBrush;
}
```

### API

| Function | Description |
|----------|-------------|
| `Edit::Subclass(hWnd)` | Replace the window proc; remove `WS_EX_CLIENTEDGE`; force frame redraw |
| `Edit::Unsubclass(hWnd)` | Restore the original window proc |
| `Edit::AutoHideScrollbar(hWnd)` | Check whether text exceeds client height; show/hide `WS_VSCROLL` accordingly (called automatically on `WM_SETFONT` and `WM_SIZE`) |

### Behavior

- Background filled with `SecondaryBackground` in `WM_ERASEBKGND`
- Flat 1px border drawn with `ControlFrame` color in `WM_PAINT` (after the default text/selection rendering)
- Vertical scrollbar auto-hidden when content fits the client area
- Border does **not** use rounded corners (matching the style of `Controls::Button`)

---

## Controls::Tooltip

Creates a lightweight, self-painted tooltip popup with rounded corners and
dark theme. Does **not** use the native `TOOLTIPS_CLASS`,because it's just a shit( I'm **EXTREMELY** angry about it >:( ).

### Usage

```cpp
using namespace GLDLG::Controls;

// Create
HWND tip = Tooltip::Create(hParent, L"Hello", hFont);
Tooltip::SetPosition(tip, screenX, screenY - 14);  // 14px above
ShowWindow(tip, SW_SHOW);

// Update text (recalculates size)
Tooltip::SetText(tip, L"New text", hFont);

// Reposition
Tooltip::SetPosition(tip, newX, newY - 14, width, height);

// Destroy
Tooltip::Destroy(tip);
```

### API

| Function | Description |
|----------|-------------|
| `Tooltip::Create(hParent, text, hFont)` | Create a popup window; returns `HWND` or `nullptr` on failure |
| `Tooltip::SetText(hTooltip, text, hFont)` | Change the displayed text; recalculates window size; stores font in `GWLP_USERDATA` |
| `Tooltip::SetPosition(hTooltip, x, y, w, h)` | Move to screen coordinates; pass `0,0` for `w,h` to keep current size |
| `Tooltip::Destroy(hTooltip)` | Destroy the tooltip window (safe to call on `nullptr` or invalid handle) |
| `Tooltip::MeasureText(hdc, text, hFont, outW, outH)` | Static measurement helper: returns the pixel size including 18px horizontal / 10px vertical padding |
