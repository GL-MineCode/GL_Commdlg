Select a language | 选择语言

[**简体中文**](README_cn.md)  
[**English**](README.md)  

# GL_Commdlg

A C++ header-only library that wraps Windows Common Dialogs and Shell dialogs, providing an easy-to-use UTF-8 API with extended functionality.

## Features

- **Single header** — Just `#include "GL_Commdlg.hpp"`, no extra dependencies
- **UTF-8 API** — All string parameters and return values use UTF-8; conversion to/from wide strings is handled internally
- **File dialogs** — Open, save, and multi-select file dialogs with custom filters
- **Directory picker** — Folder browser dialog wrapping `SHBrowseForFolderW`
- **Color picker** — Color selection dialog wrapping `ChooseColorW`
- **Font picker** — Font selection dialog wrapping `ChooseFontW`, with automatic font file path lookup via the registry
- **Prompt dialog** — Custom text input dialog (not available in standard `commdlg`)
- **Custom message box** — Message box with user-defined buttons and two display styles
- **Non-blocking dynamic dialogs** — Progress bar and slider that run in a separate thread, controllable from the calling thread

## Quick Start

```cpp
#include "GL_Commdlg.hpp"
#include <iostream>

int main() {
    // Open a file dialog
    std::string file = getOpenFileName(
        {"Text Files (*.txt)|*.txt", "All Files (*.*)|*.*"},
        "Select a file"
    );
    if (!file.empty())
        std::cout << "Selected: " << file << std::endl;

    // Message box with custom buttons
    int choice = messageBox(
        "Question",
        "Do you want to continue?",
        {{1, "Yes"}, {2, "No"}}
    );
    std::cout << "You chose: " << choice << std::endl;

    return 0;
}
```

## Building

This is a header-only library — just include the header:

```bash
# MinGW-w64 (requires extra linker flags)
g++ -std=c++20 -Iinclude your_program.cpp -o your_program -lcomdlg32 -lshell32 -lgdi32 -lole32

# MSVC (auto-links via #pragma comment)
cl /std:c++20 /Iinclude your_program.cpp
```

Or use the provided Makefile to build the test:

```bash
mingw32-make test    # Build test/test.cpp
./build/test.exe      # Run tests
```

## API Reference

### File & Directory Dialogs

| Function | Description |
|----------|-------------|
| `getOpenFileName` | Open file dialog, select one existing file |
| `getSaveFileName` | Save file dialog, specify a save path |
| `getOpenMultipleFileNames` | Open file dialog, select multiple files |
| `getOpenDirectoryName` | Folder browser dialog, select a directory |

Filters use `"Description|Pattern"` format. Multiple patterns are separated by `;`:

```cpp
auto file = getOpenFileName({
    "Text Files (*.txt)|*.txt",
    "Images (*.png;*.jpg)|*.png;*.jpg",
    "All Files (*.*)|*.*"
});
```

All functions accept optional parameters: `title`, `initialDir`, `defaultFileName`, `defaultExt`, and `parentHWND`.

| Dialog | Preview |
|--------|---------|
| Open File | ![](demo/select_file.png) |
| Save File | ![](demo/save_file.png) |
| Browse Folder | ![](demo/select_directory.png) |

### Color Picker

```cpp
SDL_Color color = {255, 0, 0, 255};  // initial red
chooseColor(color);
// color now holds the user's selection
```

The library defines a fallback `SDL_Color` only if the SDL header is not already included.

![](demo/pick_color.png)

### Font Picker

```cpp
chooseFontInfo cfi;
chooseFont(cfi);
// cfi.fontFaceName  — e.g. "Arial"
// cfi.fontPointSize — e.g. 12
// cfi.fontPath      — e.g. "C:\\Windows\\Fonts\\arial.ttf"
//                    (may be empty if the font file is not found)
```

![](demo/pick_font.png)

### Prompt Dialog

```cpp
std::string input;
bool confirmed = promptDialog("Input", "Enter your name:", input, "Default Name");
if (confirmed) {
    // input contains the entered text
}
```

![](demo/prompt.png)

### Custom Message Box

```cpp
int result = messageBox(
    "Title",
    "Message text.",
    {{10, "OK"}, {20, "Cancel"}, {30, "Help"}},
    NULL,       // parent HWND
    0           // style: 0 = GDI rendered (word wrapping), 1 = EDIT control
);
// Returns the clicked button's key, 0 if closed, -1 if options is empty
```

Style 0 uses GDI `DrawTextW` with `DT_WORDBREAK` — the dialog auto-sizes to fit the text.

![](demo/message_box_style_0.png)

Style 1 uses a multi-line EDIT control with auto-scroll — suitable for long messages.

![](demo/message_box_style_1.png)

## Dynamic (Non-blocking) Dialogs

Dynamic dialogs run in a **separate thread**, so they never block the calling code. They are controlled through a movable, non-copyable RAII interface that automatically cleans up on destruction.

### DynamicProgressBar

```cpp
auto bar = CreateDynamicProgressBar("Progress", "Working...");

for (int i = 0; i <= 100; i += 10) {
    bar.SetValue(i, 100, std::to_string(i) + "%");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

bar.Close();  // or let the destructor handle it
```

| Method | Description |
|--------|-------------|
| `SetValue(current, max, message)` | Update progress value and text |
| `GetProgressInfo(current, max, message, percent)` | Read current state |
| `Show()` / `Close()` | Show or close the dialog |
| `IsFinished()` | Check if the dialog has been closed |

![](demo/progress_bar.png)

### DynamicSlider

```cpp
auto slider = CreateDynamicSlider(
    "Volume", "Adjust the volume:", 0, 100, 50,
    [](DynamicSliderCallbackMessageType type, int value) -> int {
        return value;  // optionally modify or clamp
    }
);

while (!slider.IsFinished()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int cur, min, max;
std::string msg;
slider.GetSliderInfo(cur, min, max, msg);
```

| Method | Description |
|--------|-------------|
| `SetValue(value)` | Set current value |
| `SetRange(min, max)` | Set slider range |
| `SetCallback(cb)` | Set value-change callback |
| `GetSliderInfo(current, min, max, message)` | Read current state |
| `Show()` / `Close()` | Show or close the dialog |
| `IsFinished()` | Check if the dialog has been closed |
| `IsDragging()` | Check if the user is dragging the thumb |

The callback receives a `DynamicSliderCallbackMessageType` (`Dragging` or `Released`) and the current value; it can return a modified value.

![](demo/slider.png)

## Project Structure

```
GL_Commdlg/
├── demo/
│   ├── select_file.png      # Screenshots demonstrating each dialog
│   ├── save_file.png
│   ├── select_directory.png
│   ├── pick_color.png
│   ├── pick_font.png
│   ├── prompt.png
│   ├── message_box_style_0.png
│   ├── message_box_style_1.png
│   ├── progress_bar.png
│   └── slider.png
├── include/
│   ├── GL_Commdlg.hpp      # Main header — the whole library
│   └── UTF8toWide.hpp       # UTF-8 / wide string conversion helpers
├── test/
│   └── test.cpp             # Test program (exercises all APIs)
├── Makefile                 # Build script (MinGW-w64)
├── LICENSE                  # License file
├── README.md                # English documentation
└── README_cn.md             # Chinese documentation
```

## License

This software is provided 'as-is', without any express or implied warranty.
See the [LICENSE](LICENSE) file for full details.

Copyright (c) 2026 Gao Li

