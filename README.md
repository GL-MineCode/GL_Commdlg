# GL_Commdlg

A lightweight C++ header-only library that encapsulates Windows common dialog APIs, providing easy-to-use functions for file, color, font selection, and more.

## Features

### 🗂️ File Dialogs
- **File Open Dialog** - Select a single file
- **File Save Dialog** - Specify save path
- **Multiple File Selection** - Select multiple files simultaneously
- **Directory Selection** - Browse and select folders

### 🎨 System Dialogs
- **Color Picker** - System color selector
- **Font Picker** - Select from system-installed fonts
- **Input Dialog** - Custom text input
- **Message Box** - Custom dialog with multiple options

### ✨ Core Advantages
- **UTF-8 Support** - Full Chinese and Unicode support
- **Cross-compiler Compatibility** - Supports MSVC and other compilers
- **Exception Safety** - Comprehensive error handling mechanism
- **Lightweight** - Single header file, easy to integrate

## Quick Start

### Basic Usage

```cpp
#include "GL_Commdlg.hpp"

// Open file dialog
auto filePath = getOpenFileName(
    {"Text Files(*.txt)|*.txt", "All Files(*.*)|*.*"},
    "Select File",
    "C:\\",
    "default.txt",
    "txt"
);

// Color selection
SDL_Color color = {255, 0, 0, 255};
chooseColor(color);

// Font selection
chooseFontInfo cfi;
chooseFont(cfi);
```

### Complete Example

```cpp
#include "GL_Commdlg.hpp"
#include <iostream>

int main() {
    try {
        // 1. Open a single file
        std::string file = getOpenFileName(
            {"Images|*.jpg;*.png;*.bmp", "All Files|*.*"},
            "Select Image File"
        );
        
        if (!file.empty()) {
            std::cout << "Selected file: " << file << std::endl;
        }

        // 2. Select multiple files
        auto files = getOpenMultipleFileNames(
            {"Text Files|*.txt", "All Files|*.*"}
        );
        
        for (const auto& f : files) {
            std::cout << "Multi-selected file: " << f << std::endl;
        }

        // 3. Select directory
        std::string dir = getOpenDirectoryName("Select Working Directory");
        if (!dir.empty()) {
            std::cout << "Selected directory: " << dir << std::endl;
        }

        // 4. Color selection
        SDL_Color color = {128, 128, 128, 255};
        chooseColor(color);
        std::cout << "Selected color: R=" << (int)color.r 
                  << " G=" << (int)color.g 
                  << " B=" << (int)color.b << std::endl;

        // 5. Font selection
        chooseFontInfo fontInfo;
        chooseFont(fontInfo);
        std::cout << "Selected font: " << fontInfo.fontFaceName 
                  << " Size: " << fontInfo.fontPointSize << std::endl;

        // 6. Input dialog
        std::string input;
        if (promptDialog("Input", "Please enter your name:", input, "Default name")) {
            std::cout << "User input: " << input << std::endl;
        }

        // 7. Custom message box
        int result = messageBox("Confirmation", "Please select an operation:", {
            {1, "OK"},
            {2, "Cancel"},
            {3, "Help"}
        });
        std::cout << "User selection: " << result << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
```

## API Reference

### File Operations

```cpp
// Open a single file
std::string getOpenFileName(
    const std::vector<std::string>& filters,
    const std::string& title = "",
    const std::string& initialDir = "",
    const std::string& defaultFileName = "",
    const std::string& defaultExt = "",
    HWND parentHWND = NULL
);

// Save file
std::string getSaveFileName(...);  // Same parameters as above

// Multiple file selection
std::vector<std::string> getOpenMultipleFileNames(...);  // Same parameters as above

// Directory selection
std::string getOpenDirectoryName(
    const std::string& title = "",
    const std::string& initialDir = "",
    HWND parentHWND = NULL
);
```

### System Dialogs

```cpp
// Color selection
void chooseColor(SDL_Color& selectedColor, HWND hwndParent = NULL);

// Font selection
struct chooseFontInfo {
    std::string fontFaceName;
    std::string fontPath;      // May be empty
    int fontPointSize;
};
void chooseFont(chooseFontInfo& cfi, HWND hwndParent = NULL);

// Input dialog
bool promptDialog(
    std::string title,
    std::string message, 
    std::string& output,
    std::string defaultContent = "",
    HWND hParent = NULL
);

// Custom message box
int messageBox(
    std::string title,
    std::string message,
    const std::vector<std::pair<int, std::string>>& options,
    HWND hParent = NULL
);
```

## Compilation Instructions

### MSVC Compiler
Automatically links required libraries, no additional configuration needed.

### Other Compilers (GCC, Clang, etc.)
Add compilation parameters:
```bash
-lcomdlg32 -lshell32
```

### Dependencies
- Windows SDK
- Standard C++ Library

## License

This project uses the zlib license, allowing free use in both personal and commercial projects.

## Contributions

Issues and Pull Requests are welcome to improve this project.

## Version History

- v1.0.0 (2025-10-05): Initial release
  - Basic file dialog functionality
  - System color and font selection
  - Custom input and message dialogs

## Technical Support

For issues, please submit a GitHub Issue or contact the maintainer.