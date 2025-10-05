# GL_Commdlg

一个轻量级的C++头文件库，封装了Windows常用对话框API，提供简单易用的文件、颜色、字体选择等功能。

## 功能特性

### 🗂️ 文件对话框
- **文件打开对话框** - 选择单个文件
- **文件保存对话框** - 指定保存路径
- **多文件选择** - 同时选择多个文件
- **目录选择** - 浏览和选择文件夹

### 🎨 系统对话框
- **颜色选择** - 系统颜色选择器
- **字体选择** - 系统已安装字体选择
- **输入对话框** - 自定义文本输入
- **消息框** - 支持多个选项的自定义对话框

### ✨ 核心优势
- **UTF-8支持** - 完整的中文和Unicode支持
- **跨编译器兼容** - 支持MSVC和其他编译器
- **异常安全** - 完善的错误处理机制
- **轻量级** - 单头文件，易于集成

## 快速开始

### 基本用法

```cpp
#include "GL_Commdlg.hpp"

// 打开文件对话框
auto filePath = getOpenFileName(
    {"Text Files(*.txt)|*.txt", "All Files(*.*)|*.*"},
    "选择文件",
    "C:\\",
    "default.txt",
    "txt"
);

// 颜色选择
SDL_Color color = {255, 0, 0, 255};
chooseColor(color);

// 字体选择
chooseFontInfo cfi;
chooseFont(cfi);
```

### 完整示例

```cpp
#include "GL_Commdlg.hpp"
#include <iostream>

int main() {
    try {
        // 1. 打开单个文件
        std::string file = getOpenFileName(
            {"Images|*.jpg;*.png;*.bmp", "All Files|*.*"},
            "选择图片文件"
        );
        
        if (!file.empty()) {
            std::cout << "选择的文件: " << file << std::endl;
        }

        // 2. 选择多个文件
        auto files = getOpenMultipleFileNames(
            {"Text Files|*.txt", "All Files|*.*"}
        );
        
        for (const auto& f : files) {
            std::cout << "多选文件: " << f << std::endl;
        }

        // 3. 选择目录
        std::string dir = getOpenDirectoryName("选择工作目录");
        if (!dir.empty()) {
            std::cout << "选择的目录: " << dir << std::endl;
        }

        // 4. 颜色选择
        SDL_Color color = {128, 128, 128, 255};
        chooseColor(color);
        std::cout << "选择的颜色: R=" << (int)color.r 
                  << " G=" << (int)color.g 
                  << " B=" << (int)color.b << std::endl;

        // 5. 字体选择
        chooseFontInfo fontInfo;
        chooseFont(fontInfo);
        std::cout << "选择的字体: " << fontInfo.fontFaceName 
                  << " 大小: " << fontInfo.fontPointSize << std::endl;

        // 6. 输入对话框
        std::string input;
        if (promptDialog("输入", "请输入您的姓名:", input, "默认名称")) {
            std::cout << "用户输入: " << input << std::endl;
        }

        // 7. 自定义消息框
        int result = messageBox("确认", "请选择操作:", {
            {1, "确定"},
            {2, "取消"},
            {3, "帮助"}
        });
        std::cout << "用户选择: " << result << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
    }
    
    return 0;
}
```

## API 参考

### 文件操作

```cpp
// 打开单个文件
std::string getOpenFileName(
    const std::vector<std::string>& filters,
    const std::string& title = "",
    const std::string& initialDir = "",
    const std::string& defaultFileName = "",
    const std::string& defaultExt = "",
    HWND parentHWND = NULL
);

// 保存文件
std::string getSaveFileName(...);  // 参数同上

// 多文件选择
std::vector<std::string> getOpenMultipleFileNames(...);  // 参数同上

// 目录选择
std::string getOpenDirectoryName(
    const std::string& title = "",
    const std::string& initialDir = "",
    HWND parentHWND = NULL
);
```

### 系统对话框

```cpp
// 颜色选择
void chooseColor(SDL_Color& selectedColor, HWND hwndParent = NULL);

// 字体选择
struct chooseFontInfo {
    std::string fontFaceName;
    std::string fontPath;      // 可能为空
    int fontPointSize;
};
void chooseFont(chooseFontInfo& cfi, HWND hwndParent = NULL);

// 输入对话框
bool promptDialog(
    std::string title,
    std::string message, 
    std::string& output,
    std::string defaultContent = "",
    HWND hParent = NULL
);

// 自定义消息框
int messageBox(
    std::string title,
    std::string message,
    const std::vector<std::pair<int, std::string>>& options,
    HWND hParent = NULL
);
```

## 编译说明

### MSVC编译器
自动链接所需库，无需额外配置。

### 其他编译器（GCC、Clang等）
添加编译参数：
```bash
-lcomdlg32 -lshell32
```

### 依赖项
- Windows SDK
- 标准C++库

## 许可证

本项目采用zlib许可证，允许自由使用于个人和商业项目。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 版本历史

- v1.0.0 (2025-01-01): 初始版本发布
  - 基础文件对话框功能
  - 系统颜色和字体选择
  - 自定义输入和消息对话框

## 技术支持

如有问题请提交GitHub Issue或联系维护者。