# GLDLG::Controls — 自定义 Win32 控件

`GLDLG::Controls` 命名空间提供暗色主题的自定义 Win32 控件，可用于你自己的对话框窗口。
所有控件使用统一的配色方案（参见 `GLDLG::Theme` 结构体）。

## 速览

| 控件 | 说明 |
|------|------|
| `Button` | 子类化标准 `BUTTON`，绘制暗色圆角按钮 |
| `Edit` | 子类化标准 `EDIT`，绘制暗色扁平边框编辑框，支持自动隐藏滚动条 |
| `Tooltip` | 创建自绘 Tooltip 弹出窗口，圆角暗色主题 |

---

## 主题

六种主题色可以随时查询或覆盖：

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

`ColorRGBA` 是一个简单的结构体：`{uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255}`。

| 颜色 | 默认值 | 用途 |
|------|--------|------|
| `Text` | `#E6E6EF` | 标签、控件文字 |
| `ControlFrame` | `#383842` | 所有控件的边框 |
| `PrimaryBackground` | `#1A1A1E` | 对话框背景 |
| `SecondaryBackground` | `#27272E` | 控件内部（按钮表面、编辑框） |
| `PrimaryForeground` | `#424949` | 按钮悬停状态 |
| `SecondaryForeground` | `#485466` | 禁用状态 |

---

## Controls::Button

子类化标准 Win32 `BUTTON`，绘制自定义暗色圆角按钮。
处理所有视觉状态（正常、悬停、按下、禁用）。

### 用法

```cpp
#include <windows.h>
#include "GL_Commdlg.hpp"

// 1. 正常创建按钮
HWND hBtn = CreateWindowW(L"BUTTON", L"Click Me",
    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
    x, y, 80, 30, hParent, (HMENU)1001, hInst, nullptr);

// 2. 应用子类化
GLDLG::Controls::Button::Subclass(hBtn);

// (可选) 设置字体
SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
```

### API

| 函数 | 说明 |
|------|------|
| `Button::Subclass(hWnd)` | 替换窗口过程为自定义过程；设置 `BS_OWNERDRAW` 并保存原始过程 |
| `Button::Unsubclass(hWnd)` | 恢复原始窗口过程并释放 `WinData` |

### 视觉状态

| 状态 | 背景 | 边框 |
|------|------|------|
| 正常 | `SecondaryBackground` | `ControlFrame`，5px 圆角 |
| 悬停 | `PrimaryForeground` | `ControlFrame` |
| 按下 | `PrimaryForeground` 半亮度 | `ControlFrame` |
| 禁用 | `SecondaryForeground` | `ControlFrame`，文字半亮度 |

---

## Controls::Edit

子类化标准 `EDIT` 以匹配暗色主题。
移除默认的 `WS_EX_CLIENTEDGE` 3D 边框，通过自定义 `WM_PAINT` 绘制扁平 1px 边框。
当文本完全可见时自动隐藏滚动条。

### 用法

```cpp
// 1. 创建 EDIT 控件（不带 WS_EX_CLIENTEDGE）
HWND hEdit = CreateWindowW(L"EDIT", L"some text",
    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
    x, y, 200, 24, hParent, (HMENU)1002, hInst, nullptr);

// 2. 子类化（WS_EX_CLIENTEDGE 会自动移除）
GLDLG::Controls::Edit::Subclass(hEdit);

// 3. 设置字体（子类化之后再设置，会触发 AutoHideScrollbar）
SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
```

### 父窗口要求

父对话框必须处理 `WM_CTLCOLOREDIT`（以及只读编辑框的 `WM_CTLCOLORSTATIC`）来设置文字颜色：

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

| 函数 | 说明 |
|------|------|
| `Edit::Subclass(hWnd)` | 替换窗口过程；移除 `WS_EX_CLIENTEDGE`；强制重绘边框 |
| `Edit::Unsubclass(hWnd)` | 恢复原始窗口过程 |
| `Edit::AutoHideScrollbar(hWnd)` | 检查文本是否超过客户区高度；相应显示/隐藏 `WS_VSCROLL`（在 `WM_SETFONT` 和 `WM_SIZE` 中自动调用） |

### 行为

- 背景在 `WM_ERASEBKGND` 中用 `SecondaryBackground` 填充
- 扁平 1px 边框在 `WM_PAINT` 中用 `ControlFrame` 色绘制（在默认文本/选区渲染之后）
- 垂直滚动条在内容可完全显示时自动隐藏
- 边框**不使用**圆角（与 `Controls::Button` 风格一致）

---

## Controls::Tooltip

创建轻量级自绘 Tooltip 弹出窗口，圆角暗色主题。
**不使用**原生的 `TOOLTIPS_CLASS`，因为它就是一坨屎山（**哪个二流程序员设计的这垃圾东西** >:( ）。

### 用法

```cpp
using namespace GLDLG::Controls;

// 创建
HWND tip = Tooltip::Create(hParent, L"Hello", hFont);
Tooltip::SetPosition(tip, screenX, screenY - 14);  // 向上偏移 14px
ShowWindow(tip, SW_SHOW);

// 更新文字（自动重算尺寸）
Tooltip::SetText(tip, L"New text", hFont);

// 重新定位
Tooltip::SetPosition(tip, newX, newY - 14, width, height);

// 销毁
Tooltip::Destroy(tip);
```

### API

| 函数 | 说明 |
|------|------|
| `Tooltip::Create(hParent, text, hFont)` | 创建一个弹出窗口；返回 `HWND`，失败返回 `nullptr` |
| `Tooltip::SetText(hTooltip, text, hFont)` | 更改显示文本；重新计算窗口尺寸；将字体句柄存入 `GWLP_USERDATA` |
| `Tooltip::SetPosition(hTooltip, x, y, w, h)` | 移动到屏幕坐标；`w,h` 传 `0,0` 保持当前尺寸 |
| `Tooltip::Destroy(hTooltip)` | 销毁 Tooltip 窗口（传入 `nullptr` 或无效句柄也是安全的） |
| `Tooltip::MeasureText(hdc, text, hFont, outW, outH)` | 静态测量辅助：返回包含 18px 水平 / 10px 垂直内边距的像素尺寸 |

### 绘制方式

Tooltip 完全在 `WM_PAINT` 中自绘：
1. 用 `SecondaryBackground` 填充整个客户区矩形
2. 将文本绘制裁剪到 `RoundRect` 区域（8px 半径）
3. 用 `ControlFrame` 色通过 `RoundRect` 绘制边框（8px 半径）

不使用 `SetWindowRgn`——背景填充延伸到四个角落，圆角边框绘制在其上，呈现干净的圆角外观。
