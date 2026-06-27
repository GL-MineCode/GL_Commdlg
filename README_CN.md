选择语言 | Select a language

[**English**](README.md)  
[**简体中文**](README_CN.md)  

# GL_Commdlg

一个 C++ 头文件库，封装了 Windows Common Dialogs 和 Shell 对话框，提供易用的 UTF-8 API 并扩展了额外功能。

## 特性

- **单头文件** — 只需 `#include "GL_Commdlg.hpp"`，无额外依赖
- **UTF-8 API** — 所有字符串参数和返回值均使用 UTF-8 编码，不用再为Win32 API的编码问题烦恼！
- **文件对话框** — 打开、保存、多选文件对话框，支持自定义过滤器
- **目录选择器** — 目录选择对话框，自动采用合适系统版本的API
- **字体选择器** — 封装 `ChooseFontW` 的字体选择对话框，自动通过注册表查找字体文件路径
- **输入对话框** — 自定义文本输入的对话框
- **自定义消息框** — 支持用户自定义按钮和两种显示样式的消息框
- **非阻塞动态对话框** — 进度条和滑动条在独立线程中运行，可从调用线程实时控制
    - **滑条对话框** — 非阻塞的滑条对话框，允许用户通过一个带有滑条的对话框来选择数值
    - **进度对话框** — 非阻塞的进度对话框，允许用户通过一个带有进度条的对话框来查看某项任务进行的进度
    - **颜色选择器** — 非阻塞的颜色选择对话框，比Windows自带的阻塞式对话框更强！支持HSL/HEX颜色输入、屏幕取色、Alpha值等功能，比肩QT的颜色选择对话框（正在开发中，敬请期待）

## 快速开始

```cpp
#include "GL_Commdlg.hpp"
#include <iostream>

int main() {
    // 打开文件对话框
    std::string file = GLDLG::getOpenFileName(
        {"文本文件 (*.txt)|*.txt", "所有文件 (*.*)|*.*"},
        "选择一个文件"
    );
    if (!file.empty())
        std::cout << "已选择: " << file << std::endl;

    // 自定义按钮的消息框
    int choice = GLDLG::messageBox(
        "问题",
        "是否继续？",
        {{1, "是"}, {2, "否"}}
    );
    std::cout << "你选择了: " << choice << std::endl;

    return 0;
}
```

## 编译

本库为头文件库，只需包含头文件即可：

```bash
# MinGW-w64（需要额外链接库）
g++ -std=c++20 -Iinclude your_program.cpp -o your_program -lcomdlg32 -lshell32 -lgdi32 -lole32 -luuid -ldwmapi

# MSVC（通过 #pragma comment 自动链接）
cl /std:c++20 /Iinclude your_program.cpp
```

或使用提供的 Makefile 构建测试程序：

```bash
mingw32-make test    # 编译 test/test.cpp
./build/test.exe      # 运行测试
```

## API 参考

### 文件与目录对话框

| 函数 | 说明 |
|------|------|
| `getOpenFileName` | 打开文件对话框，选择一个已存在的文件 |
| `getSaveFileName` | 保存文件对话框，指定保存路径 |
| `getOpenMultipleFileNames` | 打开文件对话框，选择多个文件 |
| `getOpenDirectoryName` | 目录浏览对话框，选择一个目录 |

过滤器使用 `"描述|匹配模式"` 格式，多模式用 `;` 分隔：

```cpp
auto file = GLDLG::getOpenFileName({
    "文本文件 (*.txt)|*.txt",
    "图片 (*.png;*.jpg)|*.png;*.jpg",
    "所有文件 (*.*)|*.*"
});
```

所有函数均支持可选参数：`title`（标题）、`initialDir`（初始目录）、`defaultFileName`（默认文件名）、`defaultExt`（默认扩展名）和 `parentHWND`（父窗口句柄）。

| 对话框 | 预览 |
|--------|------|
| 打开文件 | ![](demo/select_file.png) |
| 保存文件 | ![](demo/save_file.png) |
| 浏览文件夹 | ![](demo/select_directory.png) |

### 字体选择器

```cpp
GLDLG::chooseFontInfo cfi;
GLDLG::chooseFont(cfi);
// cfi.fontFaceName  — 例如 "Arial"
// cfi.fontPointSize — 例如 12
// cfi.fontPath      — 例如 "C:\\Windows\\Fonts\\arial.ttf"
//                    （未找到时可能为空字符串）
```

![](demo/pick_font.png)

### 输入对话框

```cpp
std::string input;
bool confirmed = GLDLG::promptDialog("输入", "请输入你的名字：", input, "默认名字");
if (confirmed) {
    // input 包含用户输入的内容
}
```

![](demo/prompt.png)

### 自定义消息框

```cpp
int result = GLDLG::messageBox(
    "标题",
    "消息内容。",
    {{10, "确定"}, {20, "取消"}, {30, "帮助"}},
    NULL,       // 父窗口句柄
    0           // 样式：0 = GDI 渲染（自动折行），1 = EDIT 控件
);
// 返回点击按钮的键值，关闭返回 0，options 为空返回 -1
```

样式 0 使用 GDI 的 `DrawTextW` 配合 `DT_WORDBREAK` 绘制文本，对话框会根据文本内容自动调整大小。

![](demo/message_box_style_0.png)

样式 1 使用支持自动滚动的多行 EDIT 控件，适合显示长文本。

![](demo/message_box_style_1.png)

## 动态（非阻塞）对话框

动态对话框在**独立线程**中运行，不会阻塞调用代码。通过可移动、不可复制的 RAII 接口控制，析构时自动清理资源。

### 动态进度条

```cpp
auto bar = GLDLG::CreateDynamicProgressBar("进度", "工作中...");

for (int i = 0; i <= 100; i += 10) {
    bar.SetValue(i, 100, std::to_string(i) + "%");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

bar.Close();  // 或让析构函数自动处理
```

| 方法 | 说明 |
|------|------|
| `SetValue(current, max, message)` | 更新进度值和提示文本 |
| `GetProgressInfo(current, max, message, percent)` | 读取当前状态 |
| `Show()` / `Close()` | 显示或关闭对话框 |
| `IsFinished()` | 检查对话框是否已关闭 |

![](demo/progress_bar.png)

### 动态滑动条

```cpp
auto slider = GLDLG::CreateDynamicSlider(
    "音量", "调节音量：", 0, 100, 50,
    [](GLDLG::DynamicSliderCallbackMessageType type, int value) -> int {
        return value;  // 可选地修改或限制值
    }
);

while (!slider.IsFinished()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int cur, min, max;
std::string msg;
slider.GetSliderInfo(cur, min, max, msg);
```

| 方法 | 说明 |
|------|------|
| `SetValue(value)` | 设置当前值 |
| `SetRange(min, max)` | 设置滑动条范围 |
| `SetCallback(cb)` | 设置值变化回调 |
| `GetSliderInfo(current, min, max, message)` | 读取当前状态 |
| `Show()` / `Close()` | 显示或关闭对话框 |
| `IsFinished()` | 检查对话框是否已关闭 |
| `IsDragging()` | 检查用户是否正在拖动滑块 |

回调接收 `DynamicSliderCallbackMessageType`（`Dragging` 或 `Released`）和当前值，可返回修改后的值。

![](demo/slider.png)

### 动态颜色选择器

```cpp
auto picker = GLDLG::CreateDynamicColorPicker("选择颜色", {255, 0, 0, 255});

while (!picker.IsFinished()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

GLDLG::ColorRGBA color = picker.GetColor();
```

| 方法 | 说明 |
|------|------|
| `GetColor()` | 获取当前颜色（返回 `ColorRGBA`） |
| `SetCallback(cb)` | 设置颜色改变时的回调函数 |
| `Show()` / `Close()` | 显示或关闭对话框 |
| `IsFinished()` | 检查对话框是否已关闭 |

> 点击取色按钮（`Pick`）可从屏幕任意位置拾取颜色。

![](demo/pick_color2.png)
![](demo/pick_color3.png)

## 主题定制

扩展对话框的UI全部使用 `GLDLG::Theme` 定义的统一暗色配色方案

```cpp
// 获取当前主题
const auto &t = GLDLG::GetTheme();

// 设置自定义主题（ColorRGBA 值）
GLDLG::SetTheme({
    {230, 230, 239},   // Text           — #E6E6EF
    {56,  56,  66},    // ControlFrame   — #383842
    {26,  26,  30},    // PrimaryBackground  — #1A1A1E
    {39,  39,  46},    // SecondaryBackground — #27272E
    {66,  73,  73},    // PrimaryForeground   — #424949
    {72,  84,  102}    // SecondaryForeground — #485466
});
```

| 颜色 | 默认值 | 用途 |
|------|--------|------|
| `Text` | `#E6E6EF` | 标签、控件文字 |
| `ControlFrame` | `#383842` | 所有控件的边框 |
| `PrimaryBackground` | `#1A1A1E` | 对话框背景 |
| `SecondaryBackground` | `#27272E` | 控件内部背景 |
| `PrimaryForeground` | `#424949` | 按钮悬停状态 |
| `SecondaryForeground` | `#485466` | 禁用状态 |

## 项目结构

```
GL_Commdlg/
├── demo/
│   ├── select_file.png      # 各对话框的演示截图
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
│   ├── GL_Commdlg.hpp            # 主头文件 — 整个库
│   ├── GL_Commdlg_Native.hpp     # 原生 Win32 通用对话框封装
│   ├── GL_Commdlg_Extended.hpp   # 扩展自定义对话框与控件
│   └── UTF8toWide.hpp            # UTF-8 / 宽字符串转换辅助函数
├── test/
│   └── test.cpp             # 测试程序（覆盖所有 API）
├── Makefile                 # 构建脚本（MinGW-w64）
├── CONTROLS.md              # 内部控件参考文档（EN）
├── CONTROLS_cn.md           # 内部控件参考文档（CN）
├── LICENSE                  # 许可证文件
├── README.md                # 英文文档
└── README_cn.md             # 中文文档
```

## 复用控件

若想复用我们内部使用的控件（`Button`、`Edit`、`Tooltip`），详细文档见 [`CONTROLS.md`](CONTROLS.md)。

## 许可证

本软件按"原样"提供，不附带任何明示或暗示的担保。
详见 [LICENSE](LICENSE) 文件。

版权所有 (c) 2026 Gao Li

