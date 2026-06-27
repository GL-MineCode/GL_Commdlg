#ifndef __INC_GL_COMMDLG_EXTENT_
#define __INC_GL_COMMDLG_EXTENT_

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <Shlobj.h>
#include <cstdint>
#include <thread>
#include <memory>
#include <functional>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <cmath>
#include <uxtheme.h>
#include <wingdi.h>
#include <dwmapi.h>
#include "UTF8toWide.hpp"

namespace GLDLG{

#pragma region 非Win32原生对话框

    struct ColorRGBA{
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;

        constexpr ColorRGBA(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
            : r(r_), g(g_), b(b_), a(a_) {}

        COLORREF ToCOLORREF() const
        {
            return RGB(r, g, b);
        }
    };

    struct Theme{
        ColorRGBA Text;
        ColorRGBA ControlFrame;
        ColorRGBA PrimaryBackground;
        ColorRGBA SecondaryBackground;
        ColorRGBA PrimaryForeground;
        ColorRGBA SecondaryForeground;
    };

    inline Theme theme = {
        // Text: #E6E6EF 
        ColorRGBA(230, 230, 239),
        // ControlFrame: #383842 
        ColorRGBA(56, 56, 66),
        // PrimaryBackground: #1A1A1E 
        ColorRGBA(26, 26, 30),
        // SecondaryBackground: #27272E 
        ColorRGBA(39, 39, 46),
        // PrimaryForeground: #424949 
        ColorRGBA(66, 73, 73),
        // SecondaryForeground: #485466 
        ColorRGBA(72, 84, 102)};

    inline const Theme& GetTheme() { return theme; }
    inline void SetTheme(const Theme &t) { theme = t; }

    namespace Controls{
        void InitWindowColor(HWND hWnd){
            COLORREF captionBgr = theme.PrimaryBackground.ToCOLORREF();
            DwmSetWindowAttribute(
                hWnd,
                DWMWA_CAPTION_COLOR,
                &captionBgr,
                sizeof(COLORREF));

            COLORREF captionText = theme.Text.ToCOLORREF();
            DwmSetWindowAttribute(
                hWnd,
                DWMWA_TEXT_COLOR,
                &captionText,
                sizeof(COLORREF));
            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(
                hWnd,
                DWMWA_USE_IMMERSIVE_DARK_MODE,
                &darkMode,
                sizeof(BOOL));

            SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }

        enum class CtrlState
            {
                Normal,
                Hover,
                Pressed,
                Disabled
            };
        namespace CtrlDraw
        {

            void DrawRoundFrame(HDC hdc, const RECT &rc, COLORREF penColor, COLORREF bgColor, int radius = 4)
            {
                HBRUSH hBrush = CreateSolidBrush(bgColor);
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
                HPEN hPen = CreatePen(PS_SOLID, 1, penColor);
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);
                SelectObject(hdc, hOldBrush);
                DeleteObject(hBrush);
            }

            void DrawFrame(HDC hdc, const RECT &rc, COLORREF penColor, COLORREF bgColor)
            {
                HBRUSH hBrush = CreateSolidBrush(bgColor);
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
                HPEN hPen = CreatePen(PS_SOLID, 1, penColor);
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);
                SelectObject(hdc, hOldBrush);
                DeleteObject(hBrush);
            }

        }
        namespace Button{
            struct WinData
            {
                WNDPROC origProc = nullptr;
                CtrlState state = CtrlState::Normal;
                HWND hWnd = nullptr;
                HFONT font = nullptr;
            };

            namespace{
                COLORREF GetBgColor(CtrlState state)
                {
                    switch (state)
                    {
                    case CtrlState::Hover:
                        return theme.PrimaryForeground.ToCOLORREF();
                    case CtrlState::Pressed:
                    {
                        auto c = theme.PrimaryForeground;
                        return RGB(c.r / 2, c.g / 2, c.b / 2);
                    }
                    case CtrlState::Disabled:
                        return theme.SecondaryForeground.ToCOLORREF();
                    default:
                        return theme.SecondaryBackground.ToCOLORREF();
                    }
                }

                COLORREF GetTextColor(CtrlState state)
                {
                    if (state == CtrlState::Disabled)
                    {
                        auto c = theme.Text;
                        return RGB(c.r / 2, c.g / 2, c.b / 2);
                    }
                    return theme.Text.ToCOLORREF();
                }
            }

            void Unsubclass(HWND hBtn);

            LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
            {
                WinData *pData = (WinData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                if (!pData || !pData->origProc)
                    return DefWindowProc(hWnd, msg, wParam, lParam);

                switch (msg)
                {
                case WM_SETFONT:
                {
                    pData->font = (HFONT)wParam;
                    break;
                } 
                case WM_MOUSEMOVE:
                {
                    if (pData->state != CtrlState::Disabled)
                    {
                        if (pData->state != CtrlState::Pressed){
                            pData->state = CtrlState::Hover;
                            InvalidateRect(hWnd, nullptr, TRUE);
                        }
                        TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hWnd, 0};
                        TrackMouseEvent(&tme);
                    }
                    break;
                }
                case WM_MOUSELEAVE:
                    pData->state = CtrlState::Normal;
                    InvalidateRect(hWnd, nullptr, TRUE);
                    break;
                case WM_LBUTTONDOWN:
                    if (pData->state != CtrlState::Disabled)
                    {
                        pData->state = CtrlState::Pressed;
                        InvalidateRect(hWnd, nullptr, TRUE);
                    }
                    break;
                case WM_LBUTTONUP:
                    if (pData->state == CtrlState::Pressed)
                    {
                        pData->state = CtrlState::Hover;
                        InvalidateRect(hWnd, nullptr, TRUE);
                    }
                    break;
                case WM_ENABLE:
                    pData->state = (wParam) ? CtrlState::Normal : CtrlState::Disabled;
                    InvalidateRect(hWnd, nullptr, TRUE);
                    break;
                case WM_ERASEBKGND:
                    return 1;
                case WM_PAINT:
                {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hWnd, &ps);
                    RECT rcClient;
                    GetClientRect(hWnd, &rcClient);

                    HDC hMemDC = CreateCompatibleDC(hdc);
                    HBITMAP hBmp = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
                    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);

                    BitBlt(hMemDC, 0, 0, rcClient.right, rcClient.bottom, hdc, 0, 0, SRCCOPY);

                    CtrlDraw::DrawRoundFrame(hMemDC, rcClient, theme.ControlFrame.ToCOLORREF(), GetBgColor(pData->state), 5);

                    wchar_t szText[256] = {0};
                    GetWindowTextW(hWnd, szText, 256);
                    SetTextColor(hMemDC, GetTextColor(pData->state));
                    SetBkMode(hMemDC, TRANSPARENT);
                    HFONT hOldFont = (HFONT)SelectObject(hMemDC, pData->font);

                    RECT rcText = rcClient;
                    DrawTextW(hMemDC, szText, lstrlenW(szText), &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    SelectObject(hMemDC, hOldFont);

                    BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hMemDC, 0, 0, SRCCOPY);

                    SelectObject(hMemDC, hOldBmp);
                    DeleteObject(hBmp);
                    DeleteDC(hMemDC);

                    EndPaint(hWnd, &ps);
                    return 0;

                }
                case WM_DESTROY:
                {
                    LRESULT res = CallWindowProc(pData->origProc, hWnd, msg, wParam, lParam);
                    Unsubclass(hWnd);
                    return res;
                }
                case WM_DRAWITEM:
                    return 0;
                }

                return CallWindowProc(pData->origProc, hWnd, msg, wParam, lParam);
            }

            void Subclass(HWND hBtn)
            {
                WinData *pData = new WinData();
                pData->hWnd = hBtn;
                pData->origProc = (WNDPROC)GetWindowLongPtr(hBtn, GWLP_WNDPROC);
                SetWindowLongPtr(hBtn, GWLP_USERDATA, (LONG_PTR)pData);
                SetWindowLongPtr(hBtn, GWLP_WNDPROC, (LONG_PTR)WinProc);

                LONG_PTR style = GetWindowLongPtr(hBtn, GWL_STYLE);
                style &= ~BS_PUSHBUTTON;
                style &= ~BS_DEFPUSHBUTTON;
                style |= BS_OWNERDRAW;
                SetWindowLongPtr(hBtn, GWL_STYLE, style);
            }

            void Unsubclass(HWND hBtn)
            {
                WinData *pData = (WinData *)GetWindowLongPtr(hBtn, GWLP_USERDATA);
                if (pData)
                {
                    SetWindowLongPtr(hBtn, GWLP_WNDPROC, (LONG_PTR)pData->origProc);
                    SetWindowLongPtr(hBtn, GWLP_USERDATA, 0);
                    delete pData;
                }
            }
        }
        namespace Edit{
            struct WinData
            {
                WNDPROC origProc = nullptr;
                CtrlState state = CtrlState::Normal;
                HWND hWnd = nullptr;
                HFONT font = nullptr;
            };

            namespace{
                COLORREF GetBgColor(CtrlState state)
                {
                    switch (state)
                    {
                    case CtrlState::Disabled:
                        return theme.SecondaryForeground.ToCOLORREF();
                    default:
                        return theme.SecondaryBackground.ToCOLORREF();
                    }
                }
            }

            void Unsubclass(HWND hEdit);
            void AutoHideScrollbar(HWND hEdit);

            LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
            {
                WinData *pData = (WinData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                if (!pData || !pData->origProc)
                    return DefWindowProc(hWnd, msg, wParam, lParam);

                switch (msg)
                {
                case WM_SETFONT:
                {
                    pData->font = (HFONT)wParam;
                    LRESULT res = CallWindowProc(pData->origProc, hWnd, msg, wParam, lParam);
                    AutoHideScrollbar(hWnd);
                    return res;
                }
                case WM_SETFOCUS:
                {
                    pData->state = CtrlState::Normal;
                    InvalidateRect(hWnd, nullptr, TRUE);
                    break;
                }
                case WM_KILLFOCUS:
                {
                    InvalidateRect(hWnd, nullptr, TRUE);
                    break;
                }
                case WM_ENABLE:
                    pData->state = (wParam) ? CtrlState::Normal : CtrlState::Disabled;
                    InvalidateRect(hWnd, nullptr, TRUE);
                    break;
                case WM_ERASEBKGND:
                {
                    HDC hdc = (HDC)wParam;
                    RECT rc;
                    GetClientRect(hWnd, &rc);
                    HBRUSH hBrush = CreateSolidBrush(GetBgColor(pData->state));
                    FillRect(hdc, &rc, hBrush);
                    DeleteObject(hBrush);
                    return 1;
                }
                case WM_NCPAINT:
                {
                    // 边框改由 WM_PAINT 中绘制，避免被默认 EDIT 绘制覆盖
                    return CallWindowProc(pData->origProc, hWnd, msg, wParam, lParam);
                }
                case WM_PAINT:
                {
                    // 先让默认 EDIT 控件绘制文本/光标/选区
                    CallWindowProc(pData->origProc, hWnd, msg, wParam, lParam);

                    // 再在其上方绘制无圆角边框（不填充内部，避免覆盖文本）
                    HDC hdc = GetDC(hWnd);
                    RECT rc;
                    GetClientRect(hWnd, &rc);
                    HPEN hPen = CreatePen(PS_INSIDEFRAME, 1, theme.ControlFrame.ToCOLORREF());
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
                    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
                    SelectObject(hdc, hOldPen);
                    SelectObject(hdc, hOldBrush);
                    DeleteObject(hPen);
                    ReleaseDC(hWnd, hdc);
                    return 0;
                }
                case WM_SIZE:
                {
                    LRESULT res = CallWindowProc(pData->origProc, hWnd, msg, wParam, lParam);
                    AutoHideScrollbar(hWnd);
                    return res;
                }
                case WM_DESTROY:
                {
                    LRESULT res = CallWindowProc(pData->origProc, hWnd, msg, wParam, lParam);
                    Unsubclass(hWnd);
                    return res;
                }
                }

                return CallWindowProc(pData->origProc, hWnd, msg, wParam, lParam);
            }

            void Subclass(HWND hEdit)
            {
                WinData *pData = new WinData();
                pData->hWnd = hEdit;
                pData->origProc = (WNDPROC)GetWindowLongPtr(hEdit, GWLP_WNDPROC);
                SetWindowLongPtr(hEdit, GWLP_USERDATA, (LONG_PTR)pData);
                SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)WinProc);

                LONG_PTR exStyle = GetWindowLongPtr(hEdit, GWL_EXSTYLE);
                exStyle &= ~WS_EX_CLIENTEDGE;
                SetWindowLongPtr(hEdit, GWL_EXSTYLE, exStyle);
                SetWindowPos(hEdit, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
            }

            void Unsubclass(HWND hEdit)
            {
                WinData *pData = (WinData *)GetWindowLongPtr(hEdit, GWLP_USERDATA);
                if (pData)
                {
                    SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)pData->origProc);
                    SetWindowLongPtr(hEdit, GWLP_USERDATA, 0);
                    delete pData;
                }
            }

            void AutoHideScrollbar(HWND hEdit)
            {
                LONG_PTR style = GetWindowLongPtr(hEdit, GWL_STYLE);
                if (!(style & WS_VSCROLL))
                    return;

                // 临时移除滚动条，检测文本是否装得下
                style &= ~WS_VSCROLL;
                SetWindowLongPtr(hEdit, GWL_STYLE, style);
                SetWindowPos(hEdit, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

                HDC hdc = GetDC(hEdit);
                if (hdc)
                {
                    HFONT hFont = (HFONT)SendMessage(hEdit, WM_GETFONT, 0, 0);
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont ? hFont : GetStockObject(SYSTEM_FONT));

                    RECT rc;
                    GetClientRect(hEdit, &rc);

                    int textLen = GetWindowTextLengthW(hEdit);
                    if (textLen > 0)
                    {
                        wchar_t *buf = new wchar_t[textLen + 1];
                        GetWindowTextW(hEdit, buf, textLen + 1);

                        RECT rcText = {0, 0, rc.right, 0};
                        DrawTextW(hdc, buf, -1, &rcText, DT_CALCRECT | DT_WORDBREAK | DT_LEFT | DT_TOP);

                        delete[] buf;

                        if (rcText.bottom > rc.bottom)
                        {
                            // 文本高度超过可见区域，恢复滚动条
                            style |= WS_VSCROLL;
                            SetWindowLongPtr(hEdit, GWL_STYLE, style);
                            SetWindowPos(hEdit, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                        }
                    }

                    SelectObject(hdc, hOldFont);
                    ReleaseDC(hEdit, hdc);
                }
            }
        }
        namespace Tooltip{

            LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

            namespace{
                const wchar_t CLASS_NAME[] = L"GL_Commdlg.TooltipClass";

                bool EnsureRegistered()
                {
                    static std::once_flag flag;
                    static bool registered = false;
                    std::call_once(flag, []()
                                   {
                    WNDCLASSEXW wc = {sizeof(wc)};
                    wc.style = CS_HREDRAW | CS_VREDRAW;
                    wc.lpfnWndProc = WinProc;
                    wc.hInstance = GetModuleHandleW(nullptr);
                    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
                    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                    wc.lpszClassName = CLASS_NAME;
                    registered = RegisterClassExW(&wc) != 0; });
                    return registered;
                }
            }

            void MeasureText(HDC hdc, const std::wstring &text, HFONT hFont, int &outW, int &outH)
            {
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                SIZE sz = {};
                GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.length()), &sz);
                SelectObject(hdc, hOldFont);
                outW = sz.cx + 18;
                outH = sz.cy + 10;
            }

            HWND Create(HWND hParent, const std::wstring &text, HFONT hFont)
            {
                if (!EnsureRegistered())
                    return nullptr;

                HDC hdc = GetDC(hParent);
                int tw = 40, th = 20;
                if (!text.empty())
                    MeasureText(hdc, text, hFont, tw, th);
                ReleaseDC(hParent, hdc);

                HWND hwnd = CreateWindowExW(
                    WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                    CLASS_NAME, text.c_str(), WS_POPUP,
                    0, 0, tw, th,
                    hParent, nullptr,
                    GetModuleHandleW(nullptr), nullptr);

                if (hwnd)
                    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)hFont);
                return hwnd;
            }

            void SetText(HWND hTooltip, const std::wstring &text, HFONT hFont)
            {
                if (!hTooltip || !IsWindow(hTooltip))
                    return;
                SetWindowTextW(hTooltip, text.c_str());
                SetWindowLongPtrW(hTooltip, GWLP_USERDATA, (LONG_PTR)hFont);

                HWND hParent = GetWindow(hTooltip, GW_OWNER);
                if (!hParent)
                    hParent = GetDesktopWindow();

                HDC hdc = GetDC(hParent);
                int tw, th;
                MeasureText(hdc, text, hFont, tw, th);
                ReleaseDC(hParent, hdc);

                SetWindowPos(hTooltip, nullptr, 0, 0, tw, th, SWP_NOMOVE | SWP_NOZORDER);
                InvalidateRect(hTooltip, nullptr, TRUE);
            }

            void SetPosition(HWND hTooltip, int screenX, int screenY, int width = 0, int height = 0)
            {
                if (!hTooltip || !IsWindow(hTooltip))
                    return;
                if (width == 0 || height == 0)
                {
                    RECT rc;
                    GetWindowRect(hTooltip, &rc);
                    width = rc.right - rc.left;
                    height = rc.bottom - rc.top;
                }
                SetWindowPos(hTooltip, nullptr, screenX, screenY, width, height, SWP_NOZORDER);
            }

            void Destroy(HWND hTooltip)
            {
                if (hTooltip && IsWindow(hTooltip))
                    DestroyWindow(hTooltip);
            }

            LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
            {
                switch (msg)
                {
                case WM_NCPAINT:
                case WM_NCACTIVATE:
                    return 0;
                case WM_PAINT:
                {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);
                    RECT rc;
                    GetClientRect(hwnd, &rc);

                    HBRUSH hBrush = CreateSolidBrush(theme.SecondaryBackground.ToCOLORREF());
                    FillRect(hdc, &rc, hBrush);
                    DeleteObject(hBrush);

                    HRGN hClipRgn = CreateRoundRectRgn(0, 0, rc.right, rc.bottom, 8, 8);
                    SelectClipRgn(hdc, hClipRgn);
                    DeleteObject(hClipRgn);

                    HFONT hFont = (HFONT)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    if (hFont)
                    {
                        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                        SetTextColor(hdc, theme.Text.ToCOLORREF());
                        SetBkMode(hdc, TRANSPARENT);
                        wchar_t szText[256] = {0};
                        GetWindowTextW(hwnd, szText, 256);
                        DrawTextW(hdc, szText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                        SelectObject(hdc, hOldFont);
                    }

                    SelectClipRgn(hdc, NULL);
                    HPEN hPen = CreatePen(PS_SOLID, 1, theme.ControlFrame.ToCOLORREF());
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
                    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);
                    SelectObject(hdc, hOldPen);
                    SelectObject(hdc, hOldBrush);
                    DeleteObject(hPen);

                    EndPaint(hwnd, &ps);
                    return 0;
                }
                case WM_ERASEBKGND:
                    return 1;
                }
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            }
        }

    };

#define __GCOMMMDLG_IDC_PROMPT 1001 // 提示文本
#define __GCOMMMDLG_IDC_INPUT 1002  // 输入框
#define __GCOMMMDLG_IDOK 1003       // 确定按钮
#define __GCOMMMDLG_IDCANCEL 1004   // 取消按钮

    namespace
    {

        WCHAR *g_inputText = nullptr;
        std::wstring g_defalutContent;
        std::wstring g_message;
        bool g_did_confirm;

        LRESULT CALLBACK PromptDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
        {

            static HWND hStaticPrompt = NULL;
            static HWND hEditInput = NULL;
            static HWND hButtonOK = NULL;
            static HWND hButtonCancel = NULL;
            static HBRUSH hDefaultBrush = CreateSolidBrush(theme.PrimaryBackground.ToCOLORREF());

            switch (msg)
            {
            case WM_CREATE:
            {

                Controls::InitWindowColor(hDlg);

                HFONT hFont = CreateFontW(
                    24, 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"Segoe UI");

                hStaticPrompt = CreateWindowExW(
                    0,
                    L"STATIC",
                    g_message.c_str(),
                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                    20, 20, 260, 25,
                    hDlg,
                    (HMENU)__GCOMMMDLG_IDC_PROMPT,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL);

                hEditInput = CreateWindowExW(
                    0,
                    L"EDIT",
                    g_defalutContent.c_str(),
                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                    20, 50, 360, 30,
                    hDlg,
                    (HMENU)__GCOMMMDLG_IDC_INPUT,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL);

                Controls::Edit::Subclass(hEditInput);

                hButtonOK = CreateWindowExW(
                    0,
                    L"BUTTON",
                    L"确定",
                    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                    120, 95, 80, 30,
                    hDlg,
                    (HMENU)__GCOMMMDLG_IDOK,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL);

                Controls::Button::Subclass(hButtonOK);

                hButtonCancel = CreateWindowExW(
                    0,
                    L"BUTTON",
                    L"取消",
                    WS_CHILD | WS_VISIBLE,
                    220, 95, 80, 30,
                    hDlg,
                    (HMENU)__GCOMMMDLG_IDCANCEL,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL);

                Controls::Button::Subclass(hButtonCancel);

                if (hFont)
                {
                    SendMessage(hStaticPrompt, WM_SETFONT, (WPARAM)hFont, TRUE);
                    SendMessage(hEditInput, WM_SETFONT, (WPARAM)hFont, TRUE);
                    SendMessage(hButtonOK, WM_SETFONT, (WPARAM)hFont, TRUE);
                    SendMessage(hButtonCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
                }

                return 0;
            }

            case WM_SIZE:
            {
                int clientWidth = LOWORD(lParam);
                int clientHeight = HIWORD(lParam);

                if (hStaticPrompt)
                {
                    SetWindowPos(hStaticPrompt, NULL,
                                 20, 20,
                                 clientWidth - 40, 25,
                                 SWP_NOZORDER);
                }

                if (hEditInput)
                {
                    SetWindowPos(hEditInput, NULL,
                                 20, 55,
                                 clientWidth - 40, 30,
                                 SWP_NOZORDER);
                }

                if (hButtonOK && hButtonCancel)
                {
                    int buttonWidth = 80;
                    int buttonHeight = 30;
                    int buttonY = clientHeight - buttonHeight - 15;
                    int totalButtonWidth = buttonWidth * 2 + 20;
                    int startX = (clientWidth - totalButtonWidth) / 2;

                    SetWindowPos(hButtonOK, NULL,
                                 startX, buttonY,
                                 buttonWidth, buttonHeight,
                                 SWP_NOZORDER);

                    SetWindowPos(hButtonCancel, NULL,
                                 startX + buttonWidth + 20, buttonY,
                                 buttonWidth, buttonHeight,
                                 SWP_NOZORDER);
                }
                return 0;
            }

            case WM_COMMAND:
            {
                if (LOWORD(wParam) == __GCOMMMDLG_IDOK)
                {
                    WCHAR buffer[256] = {0};
                    GetDlgItemTextW(hDlg, __GCOMMMDLG_IDC_INPUT, buffer, 256);
                    wcscpy(g_inputText, buffer);
                    g_did_confirm = true;
                    DestroyWindow(hDlg);
                }
                else if (LOWORD(wParam) == __GCOMMMDLG_IDCANCEL)
                {
                    wcscpy(g_inputText, L"");
                    g_did_confirm = false;
                    DestroyWindow(hDlg);
                }
                return 0;
            }

            case WM_CLOSE:
                wcscpy(g_inputText, L"");
                g_did_confirm = false;
                DestroyWindow(hDlg);
                return 0;

            case WM_CTLCOLOREDIT:
            {
                HDC hdc = (HDC)wParam;
                SetBkColor(hdc, theme.SecondaryBackground.ToCOLORREF());
                SetTextColor(hdc, theme.Text.ToCOLORREF());
                static HBRUSH hEditBrush = CreateSolidBrush(theme.SecondaryBackground.ToCOLORREF());
                return (LRESULT)hEditBrush;
            }

            case WM_CTLCOLORSTATIC:
            {
                HDC hdc = (HDC)wParam;
                SetBkColor(hdc, theme.PrimaryBackground.ToCOLORREF());
                SetTextColor(hdc, theme.Text.ToCOLORREF());
                return (LRESULT)hDefaultBrush;
            }

            case WM_CTLCOLORBTN:
            {
                HDC hdc = (HDC)wParam;
                SetBkColor(hdc, theme.PrimaryBackground.ToCOLORREF());
                SetTextColor(hdc, theme.Text.ToCOLORREF());
                return (LRESULT)hDefaultBrush;
            }

            case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            }

            case WM_ERASEBKGND:
            {
                HDC hdc = (HDC)wParam;
                RECT rect;
                GetClientRect(hDlg, &rect);
                FillRect(hdc, &rect, hDefaultBrush);
                return TRUE;
            }

            default:
                return DefWindowProcW(hDlg, msg, wParam, lParam);
            }
        }
    }

    /**
     * @brief Show an input dialog for the user to enter a string
     * @brief 显示输入对话框，用于让用户输入一段字符串
     *
     * @param title Title of the input dialog
     * @param title 输入对话框的标题
     * @param message Prompt text displayed inside the input dialog
     * @param message 输入对话框内显示的提示文本
     * @param output Output string receiving the user's input
     * @param output 输出用户输入的内容
     * @param defaultContent Default content pre-filled in the input field
     * @param defaultContent 输入栏内的默认内容
     * @param hParent Parent window handle of the input dialog
     * @param hParent 输入对话框的父窗口句柄
     * @return Whether the user confirmed the input
     * @return 用户是否确认了输入
     */
    bool promptDialog(std::string title, std::string message, std::string &output, std::string defaultContent = "", HWND hParent = NULL)
    {

        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = PromptDialogProc;
        wc.hInstance = GetModuleHandleW(NULL);
        wc.lpszClassName = L"GL_Commdlg.PromptDialogClass";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.style = CS_HREDRAW | CS_VREDRAW;

        RegisterClassExW(&wc);

        int targetWidth = GetSystemMetrics(SM_CXSCREEN);
        int targetHeight = GetSystemMetrics(SM_CYSCREEN);

        int x = targetWidth / 2 - 400 / 2, y = targetHeight / 2 - 180 / 2;

        g_inputText = new wchar_t[256];
        g_message = utf8ToWide(message);
        g_defalutContent = utf8ToWide(defaultContent);

        HWND hDlg = CreateWindowExW(
            0,
            L"GL_Commdlg.PromptDialogClass",
            utf8ToWide(title).c_str(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME /* | WS_SIZEBOX*/,
            x, y, 400, 180,
            hParent,
            NULL,
            GetModuleHandleW(NULL),
            NULL);

        if (hDlg)
        {
            ShowWindow(hDlg, SW_SHOW);
            UpdateWindow(hDlg);

            MSG msg;

            while (GetMessageW(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        UnregisterClassW(L"GL_Commdlg.PromptDialogClass", GetModuleHandleW(NULL));

        if (!g_did_confirm)
        {
            output = "";
            return false;
        }

        output = wideToUtf8(g_inputText);
        delete[] g_inputText;
        return true;
    }

#define __GCOMMMDLG_BTN_START 2000 // 选项按钮起始ID

    namespace
    {

        std::vector<std::pair<int, std::wstring>> g_options;
        std::wstring g_msgContent;
        std::wstring g_boxTitle;
        int g_selectedId = 0;

        LRESULT CALLBACK MessageBoxDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
        {

            static std::vector<HWND> hButtons;
            static HBRUSH hDefaultBrush = CreateSolidBrush(theme.PrimaryBackground.ToCOLORREF());
            static HFONT hFont = NULL;
            static int calculatedTextHeight = 0;

            switch (msg)
            {
            case WM_CREATE:
            {

                Controls::InitWindowColor(hDlg);

                const int TEXT_MARGIN_TOP = 20;
                const int BTN_GAP_ABOVE = 20;
                const int BTN_HEIGHT = 30;
                const int BTN_SPACING = 20;
                const int BTN_PADDING = 20; // horizontal text padding inside button
                const int BTN_BOTTOM_MARGIN = 24;
                const int MIN_BTN_WIDTH = 80;

                hFont = CreateFontW(
                    24, 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"Segoe UI");

                // Measure button label widths & message text height
                int maxBtnWidth = MIN_BTN_WIDTH;
                HDC hdc = GetDC(hDlg);
                if (hdc && hFont)
                {
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

                    // Measure each button label
                    for (const auto &opt : g_options)
                    {
                        SIZE sz = {};
                        GetTextExtentPoint32W(hdc, opt.second.c_str(),
                                              static_cast<int>(opt.second.length()), &sz);
                        int w = sz.cx + BTN_PADDING;
                        if (w > maxBtnWidth)
                            maxBtnWidth = w;
                    }

                    SelectObject(hdc, hOldFont);
                    ReleaseDC(hDlg, hdc);
                }

                // Calculate dialog width based on button grid (max 3 per row)
                size_t numBtns = g_options.size();
                int btnCols = (std::min)(static_cast<int>(numBtns), 3);
                int btnRows = static_cast<int>((numBtns + 2) / 3);
                int clientWidth = 20 + btnCols * maxBtnWidth + (btnCols - 1) * BTN_SPACING + 20;
                clientWidth = (std::max)(clientWidth, 400);
                int textWidth = clientWidth - 40;

                // Calculate text height with the actual text width
                hdc = GetDC(hDlg);
                if (hdc && hFont)
                {
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                    RECT rcText = {0, 0, textWidth, 0};
                    DrawTextW(hdc, g_msgContent.c_str(), -1, &rcText,
                              DT_CALCRECT | DT_WORDBREAK | DT_LEFT | DT_TOP);
                    calculatedTextHeight = rcText.bottom;
                    SelectObject(hdc, hOldFont);
                    ReleaseDC(hDlg, hdc);
                }
                else
                {
                    calculatedTextHeight = 26;
                }

                // Calculate desired client area size
                int btnAreaH = btnRows * BTN_HEIGHT + (btnRows - 1) * 10;
                int clientHeight = TEXT_MARGIN_TOP + calculatedTextHeight +
                                   BTN_GAP_ABOVE + btnAreaH + BTN_BOTTOM_MARGIN;
                clientHeight = (std::max)(clientHeight, 120);

                // Convert client area → window size (account for title bar & border)
                RECT rcWin = {0, 0, clientWidth, clientHeight};
                AdjustWindowRectEx(&rcWin,
                                   WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
                                   FALSE, 0);
                int winWidth = rcWin.right - rcWin.left;
                int winHeight = rcWin.bottom - rcWin.top;

                // Center and resize the dialog
                int scrW = GetSystemMetrics(SM_CXSCREEN);
                int scrH = GetSystemMetrics(SM_CYSCREEN);
                int cx = (scrW - winWidth) / 2;
                int cy = (scrH - winHeight) / 2;
                SetWindowPos(hDlg, NULL, cx, cy, winWidth, winHeight,
                             SWP_NOZORDER | SWP_NOACTIVATE);

                // Create buttons at correct positions with dynamic width
                int startY = TEXT_MARGIN_TOP + calculatedTextHeight + BTN_GAP_ABOVE;
                hButtons.reserve(numBtns);
                for (size_t i = 0; i < numBtns; ++i)
                {
                    int col = static_cast<int>(i % 3);
                    int row = static_cast<int>(i / 3);
                    int btnX = 20 + col * (maxBtnWidth + BTN_SPACING);
                    int btnY = startY + row * (BTN_HEIGHT + 10);

                    HWND hBtn = CreateWindowExW(
                        0, L"BUTTON", g_options[i].second.c_str(),
                        WS_CHILD | WS_VISIBLE,
                        btnX, btnY, maxBtnWidth, BTN_HEIGHT,
                        hDlg, (HMENU)(INT_PTR)(__GCOMMMDLG_BTN_START + i),
                        ((LPCREATESTRUCTW)lParam)->hInstance, NULL);
                    Controls::Button::Subclass(hBtn);
                    hButtons.push_back(hBtn);
                    if (hFont)
                    {
                        SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
                    }
                }
                return 0;
            }

            // Draw message text using GDI (supports word wrapping)
            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hDlg, &ps);
                if (hFont && calculatedTextHeight > 0)
                {
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                    RECT textRect = {20, 20, 380, 20 + calculatedTextHeight};
                    SetTextColor(hdc, theme.Text.ToCOLORREF());
                    SetBkMode(hdc, TRANSPARENT);
                    DrawTextW(hdc, g_msgContent.c_str(), -1, &textRect,
                              DT_WORDBREAK | DT_LEFT | DT_TOP);
                    SelectObject(hdc, hOldFont);
                }
                EndPaint(hDlg, &ps);
                return 0;
            }

            case WM_COMMAND:
            {
                int btnId = LOWORD(wParam);
                if (btnId >= __GCOMMMDLG_BTN_START && btnId < __GCOMMMDLG_BTN_START + (int)g_options.size())
                {
                    size_t index = btnId - __GCOMMMDLG_BTN_START;
                    g_selectedId = g_options[index].first;
                    DestroyWindow(hDlg);
                }
                return 0;
            }

            case WM_CLOSE:
            {
                g_selectedId = 0;
                DestroyWindow(hDlg);
                return 0;
            }

            case WM_DESTROY:
            {
                if (hFont)
                {
                    DeleteObject(hFont);
                    hFont = NULL;
                }
                calculatedTextHeight = 0;
                // for(auto i : hButtons){
                    
                // }
                hButtons.clear();
                PostQuitMessage(0);
                return 0;
            }

            // case WM_CTLCOLORBTN:
            // {
            //     HDC hdc = (HDC)wParam;
            //     SetBkColor(hdc, RGB(240, 240, 240));
            //     SetTextColor(hdc, RGB(0, 0, 0));
            //     return (LRESULT)hDefaultBrush;
            // }

            case WM_ERASEBKGND:
            {
                HDC hdc = (HDC)wParam;
                RECT rect;
                GetClientRect(hDlg, &rect);
                FillRect(hdc, &rect, hDefaultBrush);
                return TRUE;
            }

            default:
                return DefWindowProcW(hDlg, msg, wParam, lParam);
            }
        }

        LRESULT CALLBACK MessageBoxDialogProc2(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            static HWND hEditMsg = NULL;
            static std::vector<HWND> hButtons;
            static HBRUSH hDefaultBrush = CreateSolidBrush(theme.PrimaryBackground.ToCOLORREF());
            static HFONT hFont = NULL;
            static int calculatedEditHeight = 0;

            switch (msg)
            {
            case WM_CREATE:
            {

                Controls::InitWindowColor(hDlg);

                hFont = CreateFontW(
                    24, 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"Segoe UI");

                hEditMsg = CreateWindowExW(
                    0,
                    L"EDIT",
                    g_msgContent.c_str(),
                    WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL |
                        ES_WANTRETURN | WS_VSCROLL | ES_LEFT,
                    20, 20, 360, 100,
                    hDlg,
                    (HMENU)1001,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL);

                Controls::Edit::Subclass(hEditMsg);

                if (hFont)
                {
                    SendMessage(hEditMsg, WM_SETFONT, (WPARAM)hFont, TRUE);
                }

                HDC hdc = GetDC(hEditMsg);
                if (hdc && hFont)
                {
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

                    RECT rcText = {0, 0, 360 - 10, 0};
                    const wchar_t *text = g_msgContent.c_str();

                    DrawTextW(hdc, text, -1, &rcText, DT_CALCRECT | DT_WORDBREAK | DT_LEFT | DT_TOP | DT_EXPANDTABS);

                    calculatedEditHeight = std::min<int>(std::max<int>(rcText.bottom + 20, 60), 400);

                    SelectObject(hdc, hOldFont);
                    ReleaseDC(hEditMsg, hdc);

                    SetWindowPos(hEditMsg, NULL, 20, 20, 360, calculatedEditHeight, SWP_NOZORDER);
                }

                // Measure each button label to determine dynamic width
                int maxBtnWidth = 80; // minimum
                if (hFont)
                {
                    HDC hdc2 = GetDC(hDlg);
                    if (hdc2)
                    {
                        HFONT hOldFont = (HFONT)SelectObject(hdc2, hFont);
                        const int btnPadding = 20;
                        for (const auto &opt : g_options)
                        {
                            SIZE sz = {};
                            GetTextExtentPoint32W(hdc2, opt.second.c_str(),
                                                  static_cast<int>(opt.second.length()), &sz);
                            int w = sz.cx + btnPadding;
                            if (w > maxBtnWidth)
                                maxBtnWidth = w;
                        }
                        SelectObject(hdc2, hOldFont);
                        ReleaseDC(hDlg, hdc2);
                    }
                }

                hButtons.reserve(g_options.size());
                const int btnHeight = 32;
                const int btnSpacing = 15;

                // Calculate dialog width to fit all buttons in a single row
                int btnCols = static_cast<int>(g_options.size());
                int clientWidth = 20 + btnCols * maxBtnWidth + (btnCols - 1) * btnSpacing + 20;
                clientWidth = (std::max)(clientWidth, 400);
                int totalBtnWidth = btnCols * maxBtnWidth + (btnCols - 1) * btnSpacing;
                int startX = (clientWidth - totalBtnWidth) / 2;

                for (size_t i = 0; i < g_options.size(); ++i)
                {
                    HWND hBtn = CreateWindowExW(
                        0,
                        L"BUTTON",
                        g_options[i].second.c_str(),
                        WS_CHILD | WS_VISIBLE,
                        startX + (int)i * (maxBtnWidth + btnSpacing),
                        20 + calculatedEditHeight + 20,
                        maxBtnWidth, btnHeight,
                        hDlg,
                        (HMENU)(__GCOMMMDLG_BTN_START + i),
                        ((LPCREATESTRUCTW)lParam)->hInstance,
                        NULL);

                    Controls::Button::Subclass(hBtn);

                    hButtons.push_back(hBtn);

                    if (hFont)
                    {
                        SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
                    }
                }

                int totalHeight = 20 + calculatedEditHeight + 20 + btnHeight + 20;
                // Also resize width to fit buttons
                RECT rcWin2 = {0, 0, clientWidth, totalHeight};
                AdjustWindowRectEx(&rcWin2,
                                   WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
                                   FALSE, 0);
                int winWidth = rcWin2.right - rcWin2.left;
                int winHeight = rcWin2.bottom - rcWin2.top;
                int scrW = GetSystemMetrics(SM_CXSCREEN);
                int scrH = GetSystemMetrics(SM_CYSCREEN);
                int cx = (scrW - winWidth) / 2;
                int cy = (scrH - winHeight) / 2;
                SetWindowPos(hDlg, NULL, cx, cy, winWidth, winHeight,
                             SWP_NOZORDER | SWP_NOACTIVATE);

                return 0;
            }

            case WM_SIZE:
            {
                int clientWidth = LOWORD(lParam);

                if (hEditMsg)
                {
                    SetWindowPos(hEditMsg, NULL,
                                 20, 20,
                                 clientWidth - 40, calculatedEditHeight,
                                 SWP_NOZORDER);
                }

                if (!hButtons.empty())
                {
                    // Re-measure button widths from actual button text
                    // Since hFont might have been deleted, use saved hFont
                    int btnWidth = 80;
                    // Read back actual button widths by getting text
                    // For simplicity, measure the first button's width
                    // Since all buttons share the same max width
                    // We stored the button handles, we can just get the window text
                    WCHAR buf[256];
                    int maxW = 0;
                    for (HWND hBtn : hButtons)
                    {
                        GetWindowTextW(hBtn, buf, 256);
                        // Use a rough estimate: count characters * some factor
                        int len = (int)wcslen(buf);
                        // Estimate 14px per CJK char at 20pt, 8px per Latin at 18pt
                        int estW = len * 8 + 20;
                        if (estW > maxW)
                            maxW = estW;
                    }
                    btnWidth = (std::max)(maxW, 80);

                    const int btnHeight = 32;
                    const int btnSpacing = 15;
                    int totalBtnWidth = (btnWidth + btnSpacing) * (int)hButtons.size() - btnSpacing;
                    int startX = (clientWidth - totalBtnWidth) / 2;
                    int startY = 20 + calculatedEditHeight + 20;

                    for (size_t i = 0; i < hButtons.size(); ++i)
                    {
                        SetWindowPos(hButtons[i], NULL,
                                     startX + (int)i * (btnWidth + btnSpacing),
                                     startY,
                                     btnWidth, btnHeight,
                                     SWP_NOZORDER);
                    }
                }
                return 0;
            }

            case WM_COMMAND:
            {
                int btnId = LOWORD(wParam);
                if (btnId >= __GCOMMMDLG_BTN_START && btnId < __GCOMMMDLG_BTN_START + (int)g_options.size())
                {
                    size_t index = btnId - __GCOMMMDLG_BTN_START;
                    g_selectedId = g_options[index].first;
                    DestroyWindow(hDlg);
                }
                return 0;
            }

            case WM_CLOSE:
            {
                g_selectedId = 0;
                DestroyWindow(hDlg);
                return 0;
            }

            case WM_DESTROY:
            {

                if (hFont)
                {
                    DeleteObject(hFont);
                    hFont = NULL;
                }
                hButtons.clear();
                PostQuitMessage(0);
                return 0;
            }

            case WM_CTLCOLOREDIT:
            case WM_CTLCOLORSTATIC:
            {
                HDC hdc = (HDC)wParam;
                SetBkColor(hdc, theme.SecondaryBackground.ToCOLORREF());
                SetTextColor(hdc, theme.Text.ToCOLORREF());
                static HBRUSH hEditBrush = CreateSolidBrush(theme.SecondaryBackground.ToCOLORREF());
                return (LRESULT)hEditBrush;
            }

            case WM_CTLCOLORBTN:
            {
                HDC hdc = (HDC)wParam;
                SetBkColor(hdc, theme.PrimaryBackground.ToCOLORREF());
                SetTextColor(hdc, theme.Text.ToCOLORREF());
                return (LRESULT)hDefaultBrush;
            }

            case WM_ERASEBKGND:
            {
                HDC hdc = (HDC)wParam;
                RECT rect;
                GetClientRect(hDlg, &rect);
                FillRect(hdc, &rect, hDefaultBrush);
                return TRUE;
            }

            default:
                return DefWindowProcW(hDlg, msg, wParam, lParam);
            }
        }
    }

    /**
     * @brief Display a custom message box with multiple option buttons
     * @brief 显示自定义消息对话框，支持多个选项按钮
     *
     * @param title Dialog title
     * @param title 对话框标题
     * @param message Prompt text inside the dialog
     * @param message 对话框内的提示文本
     * @param options Option set (key = return value, value = button text)
     * @param options 选项集合（键为返回值，值为按钮文本）
     * @param hParent Parent window handle
     * @param hParent 父窗口句柄
     * @param style Dialog style: 0 = GDI rendered text (supports word wrapping), 1 = EDIT control
     * @param style 对话框的样式，若为0，则使用GDI绘制文本。若为1，则使用EDIT控件来显示提示文本。
     * @return Selected option ID (returns 0 if closed, returns -1 if options is empty)
     * @return 选中的选项ID（关闭窗口返回0，要是你传入的options没有元素则返回-1以告知失败）
     */
    int messageBox(std::string title, std::string message, const std::vector<std::pair<int, std::string>> &options, HWND hParent = NULL, int style = 0)
    {

        if (options.empty())
            return -1;

        g_options.clear();
        for (const auto &opt : options)
        {
            g_options.emplace_back(opt.first, utf8ToWide(opt.second));
        }
        g_msgContent = utf8ToWide(message);
        g_boxTitle = utf8ToWide(title);
        g_selectedId = 0;

        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);

        if (style == 1)
        {
            wc.lpfnWndProc = MessageBoxDialogProc2;
        }
        else
        {
            wc.lpfnWndProc = MessageBoxDialogProc;
        }

        wc.hInstance = GetModuleHandleW(NULL);
        wc.lpszClassName = L"GL_Commdlg.MessageBoxClass";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.style = CS_HREDRAW | CS_VREDRAW;

        if (!RegisterClassExW(&wc))
        {
            return 0;
        }

        // Initial dimensions; style 0 will be resized in WM_CREATE
        int windowWidth = 400;
        int windowHeight = 200;
        int x = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;

        HWND hDlg = CreateWindowExW(
            0,
            L"GL_Commdlg.MessageBoxClass",
            g_boxTitle.c_str(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
            x, y, windowWidth, windowHeight,
            hParent,
            NULL,
            GetModuleHandleW(NULL),
            NULL);

        if (hDlg)
        {
            ShowWindow(hDlg, SW_SHOW);
            UpdateWindow(hDlg);

            MSG msg;
            while (GetMessageW(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        UnregisterClassW(L"GL_Commdlg.MessageBoxClass", GetModuleHandleW(NULL));
        g_options.clear();

        return g_selectedId;
    }

    /*
        我接下来会引入一个新的概念：动态对话框
        动态对话框是在传统静态阻塞型对话框的基础上进行创新的新型动态非阻塞型对话框
        调用动态对话框函数会返回一个Interface用与控制创建的动态对话框
        动态对话框的生命周期与返回的Interface一致
        动态对话框由一个单独的线程来维护，线程的维护则由Interface来完成
        也就是说Interface的析构函数必须完成动态对话框的所有资源释放
        Interface类是不可复制的，仅能进行移动操作
    */

    class DynamicDialogInterface
    {
    public:
        virtual ~DynamicDialogInterface() = default;

        // 禁止复制，允许移动
        DynamicDialogInterface(const DynamicDialogInterface &) = delete;
        DynamicDialogInterface &operator=(const DynamicDialogInterface &) = delete;
        DynamicDialogInterface(DynamicDialogInterface &&) = default;
        DynamicDialogInterface &operator=(DynamicDialogInterface &&) = default;

        // 显示对话框
        virtual void Show() = 0;

        // 关闭对话框
        virtual void Close() = 0;

    protected:
        DynamicDialogInterface() = default;
    };

    namespace
    {
        // 进度条消息类型
        enum class ProgressBarMessageType
        {
            SetValue,
            Close
        };

        // 进度条消息结构
        struct ProgressBarMessage
        {
            ProgressBarMessageType type;
            uint64_t currentValue;
            uint64_t maxValue;
            std::string message;

            ProgressBarMessage(ProgressBarMessageType t, uint64_t c = 0, uint64_t m = 0, const std::string &msg = "")
                : type(t), currentValue(c), maxValue(m), message(msg) {}
        };
    }

    // 动态进度条接口类
    class DynamicProgressBar : public DynamicDialogInterface
    {
    private:
        // 对话框数据
        struct DialogData
        {
            HWND hwnd = nullptr;
            HWND hwndParent = nullptr;
            std::wstring title;
            std::atomic<uint64_t> currentValue{0};
            std::atomic<uint64_t> maxValue{100};
            std::atomic<bool> isFinished{false};
            std::string currentMessage;
            std::mutex dataMutex;

            // UI资源
            HFONT hFont = nullptr;
            HBRUSH hBackgroundBrush = nullptr;
            HPEN hBorderPen = nullptr;
            HPEN hProgressPen = nullptr;

            void UpdateMessage(const std::string &msg)
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                currentMessage = msg;
            }

            std::string GetMessage()
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                return currentMessage;
            }
        };

        // 线程控制
        std::unique_ptr<std::thread> dialogThread;
        std::atomic<bool> threadRunning{false};
        std::condition_variable messageCV;
        std::mutex messageMutex;
        std::queue<ProgressBarMessage> messageQueue;

        // 对话框数据
        std::shared_ptr<DialogData> dialogData;

        // 窗口类注册状态
        static bool IsWindowClassRegistered()
        {
            static std::once_flag registerFlag;
            static bool registered = false;

            std::call_once(registerFlag, []()
                           {
            WNDCLASSEXW wc = {0};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = L"GL_Commdlg.DynamicProgressBarClass";
            
            registered = RegisterClassExW(&wc) != 0; });

            return registered;
        }

        // 窗口过程（静态，通过用户数据获取实例）
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            if (msg == WM_NCCREATE)
            {
                CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
                DialogData *pData = reinterpret_cast<DialogData *>(pCreate->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pData));
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            }

            DialogData *pData = reinterpret_cast<DialogData *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

            switch (msg)
            {
            case WM_CREATE:
            {

                Controls::InitWindowColor(hwnd);

                if (!pData)
                    return -1;

                pData->hwnd = hwnd;

                // 创建字体和画刷
                pData->hFont = CreateFontW(
                    24, 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"Segoe UI");

                pData->hBackgroundBrush = CreateSolidBrush(theme.SecondaryBackground.ToCOLORREF());
                pData->hBorderPen = CreatePen(PS_SOLID, 1, theme.ControlFrame.ToCOLORREF());
                pData->hProgressPen = CreatePen(PS_SOLID, 1, RGB(0, 160, 200));

                return 0;
            }

            case WM_PAINT:
            {
                if (!pData)
                    break;

                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);

                uint64_t current = pData->currentValue.load();
                uint64_t max = pData->maxValue.load();
                std::string message = pData->GetMessage();
                double progress = (max > 0) ? static_cast<double>(current) / static_cast<double>(max) * 100.0 : 0.0;

                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                int width = clientRect.right - clientRect.left;
                int height = clientRect.bottom - clientRect.top;

                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
                HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

                FillRect(memDC, &clientRect, pData->hBackgroundBrush);

                HFONT oldFont = NULL;
                if (pData->hFont)
                {
                    oldFont = (HFONT)SelectObject(memDC, pData->hFont);
                }

                SetTextColor(memDC, theme.Text.ToCOLORREF());
                SetBkMode(memDC, TRANSPARENT);

                if (!message.empty())
                {
                    std::wstring wmessage = utf8ToWide(message);
                    RECT textRect = {20, 20, clientRect.right - 20, 50};
                    DrawTextW(memDC, wmessage.c_str(), -1, &textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
                }

                std::wstring progressText = L"Progress: " +
                                            std::to_wstring(static_cast<int>(progress)) + L"% (" +
                                            std::to_wstring(current) + L" / " + std::to_wstring(max) + L")";

                RECT percentRect = {20, 60, clientRect.right - 20, 90};
                DrawTextW(memDC, progressText.c_str(), -1, &percentRect, DT_LEFT | DT_TOP);

                RECT progressRect = {20, 100, clientRect.right - 20, 130};

                if (progress > 0)
                {
                    int fillWidth = static_cast<int>(
                        (progressRect.right - progressRect.left) * progress / 100.0);

                    RECT fillRect = progressRect;
                    fillRect.right = progressRect.left + fillWidth;

                    HBRUSH hProgressBrush = CreateSolidBrush(RGB(0, 180, 120));
                    FillRect(memDC, &fillRect, hProgressBrush);
                    DeleteObject(hProgressBrush);
                }

                HPEN oldPen = (HPEN)SelectObject(memDC, pData->hBorderPen);
                HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(HOLLOW_BRUSH));
                Rectangle(memDC, progressRect.left, progressRect.top,
                          progressRect.right, progressRect.bottom);

                if (pData->hFont)
                {
                    SelectObject(memDC, oldFont);
                }
                SelectObject(memDC, oldPen);
                SelectObject(memDC, oldBrush);

                BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

                SelectObject(memDC, oldBitmap);
                DeleteObject(memBitmap);
                DeleteDC(memDC);

                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_CLOSE:
                // 防止用户关闭对话框
                return 0;

            case WM_DESTROY:
                if (pData)
                {
                    pData->isFinished.store(true);

                    // 清理资源
                    if (pData->hFont)
                        DeleteObject(pData->hFont);
                    if (pData->hBackgroundBrush)
                        DeleteObject(pData->hBackgroundBrush);
                    if (pData->hBorderPen)
                        DeleteObject(pData->hBorderPen);
                    if (pData->hProgressPen)
                        DeleteObject(pData->hProgressPen);

                    pData->hwnd = nullptr;
                }

                PostQuitMessage(0);
                return 0;

            case WM_ERASEBKGND:
                return 1;

            default:
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            }

            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        void DialogThreadProc()
        {
            if (!IsWindowClassRegistered())
            {
                threadRunning.store(false);
                return;
            }

            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            int windowWidth = 600;
            int windowHeight = 190;
            int x = (screenWidth - windowWidth) / 2;
            int y = (screenHeight - windowHeight) / 2;

            HWND hwnd = CreateWindowExW(
                WS_EX_DLGMODALFRAME,
                L"GL_Commdlg.DynamicProgressBarClass",
                dialogData->title.c_str(),
                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                x, y, windowWidth, windowHeight,
                dialogData->hwndParent,
                nullptr,
                GetModuleHandleW(nullptr),
                dialogData.get());

            if (!hwnd)
            {
                threadRunning.store(false);
                return;
            }

            dialogData->hwnd = hwnd;

            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);

            MSG msg;
            while (threadRunning.load())
            {

                {
                    std::unique_lock<std::mutex> lock(messageMutex);
                    messageCV.wait_for(lock, std::chrono::milliseconds(10),
                                       [this]()
                                       { return !messageQueue.empty(); });

                    while (!messageQueue.empty())
                    {
                        ProgressBarMessage message = messageQueue.front();
                        messageQueue.pop();
                        lock.unlock();

                        ProcessMessage(message);
                        lock.lock();
                    }
                }

                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                    {
                        threadRunning.store(false);
                        break;
                    }

                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }

                if (!IsWindow(hwnd) || dialogData->isFinished.load())
                {
                    threadRunning.store(false);
                }
            }

            if (IsWindow(hwnd))
            {
                DestroyWindow(hwnd);
            }
        }

        void ProcessMessage(const ProgressBarMessage &msg)
        {
            switch (msg.type)
            {
            case ProgressBarMessageType::SetValue:
                dialogData->currentValue.store(msg.currentValue);
                dialogData->maxValue.store(msg.maxValue);
                dialogData->UpdateMessage(msg.message);

                if (dialogData->hwnd && IsWindow(dialogData->hwnd))
                {
                    InvalidateRect(dialogData->hwnd, nullptr, TRUE);
                    UpdateWindow(dialogData->hwnd);
                }
                break;

            case ProgressBarMessageType::Close:
                dialogData->isFinished.store(true);
                threadRunning.store(false);
                if (dialogData->hwnd && IsWindow(dialogData->hwnd))
                {
                    DestroyWindow(dialogData->hwnd);
                }
                break;
            }
        }

        void PostMessageToThread(ProgressBarMessageType type, uint64_t current = 0,
                                 uint64_t max = 0, const std::string &message = "")
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            messageQueue.emplace(type, current, max, message);
            messageCV.notify_one();
        }

    public:
        DynamicProgressBar(const std::string &title, const std::string &initialMessage, HWND hParent = nullptr)
            : dialogData(std::make_shared<DialogData>())
        {

            dialogData->hwndParent = hParent;
            dialogData->title = utf8ToWide(title);
            dialogData->UpdateMessage(initialMessage);

            threadRunning.store(true);
            dialogThread = std::make_unique<std::thread>([this]()
                                                         { DialogThreadProc(); });
            // dialogThread->detach();
        }

        ~DynamicProgressBar() override
        {
            Close();

            if (dialogThread && dialogThread->joinable())
            {
                dialogThread->join();
            }
        }

        DynamicProgressBar(DynamicProgressBar &&other) noexcept
            : dialogThread(std::move(other.dialogThread)), threadRunning(other.threadRunning.load()), dialogData(std::move(other.dialogData))
        {
        }

        DynamicProgressBar &operator=(DynamicProgressBar &&other) noexcept
        {
            if (this != &other)
            {
                Close();

                if (dialogThread && dialogThread->joinable())
                {
                    dialogThread->join();
                }

                dialogThread = std::move(other.dialogThread);
                threadRunning = other.threadRunning.load();
                dialogData = std::move(other.dialogData);
            }
            return *this;
        }

        void SetValue(uint64_t currentValue, uint64_t maxValue, const std::string &message)
        {
            if (!threadRunning.load() || dialogData->isFinished.load())
            {
                return;
            }

            PostMessageToThread(ProgressBarMessageType::SetValue, currentValue, maxValue, message);
        }

        void GetProgressInfo(uint64_t &current, uint64_t &max, std::string &message, double &progress)
        {
            if (!dialogData)
                return;

            current = dialogData->currentValue.load();
            max = dialogData->maxValue.load();
            message = dialogData->GetMessage();
            progress = (max > 0) ? static_cast<double>(current) / static_cast<double>(max) * 100.0 : 0.0;
        }

        void Show() override
        {
            if (dialogData)
            {
                ShowWindow(dialogData->hwnd, SW_SHOW);
            }
        }

        void Close() override
        {
            if (threadRunning.load() && !dialogData->isFinished.load())
            {
                PostMessageToThread(ProgressBarMessageType::Close);
            }
        }

        bool IsFinished() const
        {
            return dialogData ? dialogData->isFinished.load() : true;
        }

        HWND GetWindowHandle() const
        {
            return dialogData ? dialogData->hwnd : nullptr;
        }
    };

    /**
     * @brief Create a dynamic progress bar dialog instance
     * @brief 创建进度条动态对话框实例
     *
     * @param title Dialog title
     * @param title 对话框标题
     * @param initialMessage Initial prompt text inside the dialog
     * @param initialMessage 初始时对话框内的提示文本
     * @param hParent Parent window handle
     * @param hParent 父窗口句柄
     * @return The created dynamic progress bar instance
     * @return 创建的进度条动态对话框实例
     */
    DynamicProgressBar CreateDynamicProgressBar(const std::string &title,
                                                const std::string &initialMessage,
                                                HWND hParent = nullptr)
    {
        return DynamicProgressBar(title, initialMessage, hParent);
    }

    namespace
    {
        // 滑动条消息类型
        enum class SliderMessageType
        {
            SetValue,
            Close,
            SetRange,
            SetCallback
        };

        // 滑动条回调消息类型
        enum class DynamicSliderCallbackMessageType
        {
            Dragging,
            Released
        };

        // 滑动条消息结构
        struct SliderMessage
        {
            SliderMessageType type;
            int value;
            int minValue;
            int maxValue;
            std::function<int(DynamicSliderCallbackMessageType, int)> callback;

            SliderMessage(SliderMessageType t, int v = 0, int min = 0, int max = 100,
                          std::function<int(DynamicSliderCallbackMessageType, int)> cb = nullptr)
                : type(t), value(v), minValue(min), maxValue(max), callback(cb) {}
        };
    }

    // 动态滑动条接口类
    class DynamicSlider : public DynamicDialogInterface
    {
    private:
        // 对话框数据
        struct DialogData
        {
            DynamicSlider *parentObject;

            HWND hwnd = nullptr;
            HWND hwndParent = nullptr;
            std::wstring title;
            std::atomic<int> currentValue{0};
            std::atomic<int> minValue{0};
            std::atomic<int> maxValue{100};
            std::atomic<bool> isFinished{false};
            std::atomic<bool> isDragging{false};
            std::string currentMessage;
            std::mutex dataMutex;

            // 回调函数
            std::function<int(DynamicSliderCallbackMessageType, int)> callback;
            std::mutex callbackMutex;

            // UI资源
            HFONT hFont = nullptr;
            HBRUSH hBackgroundBrush = nullptr;
            HBRUSH hSliderBgBrush = nullptr;
            HBRUSH hSliderThumbBrush = nullptr;
            HPEN hBorderPen = nullptr;
            HPEN hThumbPen = nullptr;

            // 滑条区域
            RECT sliderRect = {0, 0, 0, 0};
            int thumbSize = 20;
            HWND hTooltip = nullptr;

            void UpdateMessage(const std::string &msg)
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                currentMessage = msg;
            }

            std::string GetMessage()
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                return currentMessage;
            }

            void SetCallback(std::function<int(DynamicSliderCallbackMessageType, int)> cb)
            {
                std::lock_guard<std::mutex> lock(callbackMutex);
                callback = cb;
            }

            int CallCallback(DynamicSliderCallbackMessageType type, int value)
            {
                std::lock_guard<std::mutex> lock(callbackMutex);
                if (callback)
                {
                    return callback(type, value);
                }
                else
                {
                    return value;
                }
            }
        };

        // 线程控制
        std::unique_ptr<std::thread> dialogThread;
        std::atomic<bool> threadRunning{false};
        std::condition_variable messageCV;
        std::mutex messageMutex;
        std::queue<SliderMessage> messageQueue;

        // 对话框数据
        std::shared_ptr<DialogData> dialogData;

        // Tooltip 工具函数（使用 Controls::Tooltip 公共 API）
        static void ShowTooltip(HWND hwndSlider, int value)
        {
            DialogData *pData = reinterpret_cast<DialogData *>(GetWindowLongPtrW(hwndSlider, GWLP_USERDATA));
            if (!pData)
                return;

            Controls::Tooltip::Destroy(pData->hTooltip);
            pData->hTooltip = nullptr;

            std::wstring text = std::to_wstring(value);
            pData->hTooltip = Controls::Tooltip::Create(hwndSlider, text, pData->hFont);
            if (!pData->hTooltip)
                return;

            int thumbX = GetThumbPosition(hwndSlider);
            POINT pt = {thumbX + pData->thumbSize / 2, pData->sliderRect.top};
            ClientToScreen(hwndSlider, &pt);

            RECT rc;
            GetWindowRect(pData->hTooltip, &rc);
            int tw = rc.right - rc.left;
            int th = rc.bottom - rc.top;

            Controls::Tooltip::SetPosition(pData->hTooltip, pt.x - tw / 2, pt.y - th - 14, tw, th);
            ShowWindow(pData->hTooltip, SW_SHOW);
        }

        static void UpdateTooltip(HWND hwndSlider, int value)
        {
            DialogData *pData = reinterpret_cast<DialogData *>(GetWindowLongPtrW(hwndSlider, GWLP_USERDATA));
            if (!pData || !pData->hTooltip || !IsWindow(pData->hTooltip))
                return;

            std::wstring text = std::to_wstring(value);
            Controls::Tooltip::SetText(pData->hTooltip, text, pData->hFont);

            int thumbX = GetThumbPosition(hwndSlider);
            POINT pt = {thumbX + pData->thumbSize / 2, pData->sliderRect.top};
            ClientToScreen(hwndSlider, &pt);

            RECT rc;
            GetWindowRect(pData->hTooltip, &rc);
            int tw = rc.right - rc.left;
            int th = rc.bottom - rc.top;

            Controls::Tooltip::SetPosition(pData->hTooltip, pt.x - tw / 2, pt.y - th - 14, tw, th);
        }

        static void HideTooltip(HWND hwndSlider)
        {
            DialogData *pData = reinterpret_cast<DialogData *>(GetWindowLongPtrW(hwndSlider, GWLP_USERDATA));
            if (!pData)
                return;
            Controls::Tooltip::Destroy(pData->hTooltip);
            pData->hTooltip = nullptr;
        }

        // 窗口类注册状态
        static bool IsWindowClassRegistered()
        {
            static std::once_flag registerFlag;
            static bool registered = false;

            std::call_once(registerFlag, []()
                           {
            WNDCLASSEXW wc = {0};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = L"GL_Commdlg.DynamicSliderClass";
            
            registered = RegisterClassExW(&wc) != 0; });

            return registered;
        }

        // 窗口过程（静态，通过用户数据获取实例）
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            if (msg == WM_NCCREATE)
            {
                CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
                DialogData *pData = reinterpret_cast<DialogData *>(pCreate->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pData));
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            }

            DialogData *pData = reinterpret_cast<DialogData *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

            switch (msg)
            {
            case WM_CREATE:
            {

                Controls::InitWindowColor(hwnd);

                if (!pData)
                    return -1;

                pData->hwnd = hwnd;

                // 创建字体和画刷
                pData->hFont = CreateFontW(
                    24, 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"Segoe UI");

                pData->hBackgroundBrush = CreateSolidBrush(theme.SecondaryBackground.ToCOLORREF());
                pData->hSliderBgBrush = CreateSolidBrush(theme.ControlFrame.ToCOLORREF());
                pData->hSliderThumbBrush = CreateSolidBrush(theme.PrimaryForeground.ToCOLORREF());
                pData->hBorderPen = CreatePen(PS_SOLID, 1, theme.ControlFrame.ToCOLORREF());
                pData->hThumbPen = CreatePen(PS_SOLID, 1, theme.ControlFrame.ToCOLORREF());

                return 0;
            }

            case WM_PAINT:
            {
                if (!pData)
                    break;

                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);

                int minValue = pData->minValue.load();
                int maxValue = pData->maxValue.load();
                std::string message = pData->GetMessage();

                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                int width = clientRect.right - clientRect.left;
                int height = clientRect.bottom - clientRect.top;

                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
                HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

                FillRect(memDC, &clientRect, pData->hBackgroundBrush);

                HFONT oldFont = NULL;
                if (pData->hFont)
                {
                    oldFont = (HFONT)SelectObject(memDC, pData->hFont);
                }

                SetTextColor(memDC, theme.Text.ToCOLORREF());
                SetBkMode(memDC, TRANSPARENT);

                if (!message.empty())
                {
                    std::wstring wmessage = utf8ToWide(message);
                    RECT textRect = {20, 20, clientRect.right - 20, 60};
                    DrawTextW(memDC, wmessage.c_str(), -1, &textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
                }

                pData->sliderRect.left = 20;
                pData->sliderRect.right = clientRect.right - 20;
                pData->sliderRect.top = 70;
                pData->sliderRect.bottom = 100;

                RECT sliderBgRect = pData->sliderRect;
                FillRect(memDC, &sliderBgRect, pData->hSliderBgBrush);

                HPEN oldPen = (HPEN)SelectObject(memDC, pData->hBorderPen);
                HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(HOLLOW_BRUSH));
                Rectangle(memDC, sliderBgRect.left, sliderBgRect.top,
                          sliderBgRect.right, sliderBgRect.bottom);

                int numTicks = 5;
                for (int i = 0; i <= numTicks; i++)
                {
                    float ratio = static_cast<float>(i) / static_cast<float>(numTicks);
                    int tickX = pData->sliderRect.left +
                                static_cast<int>(ratio * (pData->sliderRect.right - pData->sliderRect.left));

                    MoveToEx(memDC, tickX, pData->sliderRect.bottom + 2, NULL);
                    LineTo(memDC, tickX, pData->sliderRect.bottom + 8);

                    int tickValue = minValue + static_cast<int>(ratio * (maxValue - minValue));
                    std::wstring tickText = std::to_wstring(tickValue);
                    RECT tickRect = {tickX - 40, pData->sliderRect.bottom + 10, tickX + 40, pData->sliderRect.bottom + 30};
                    DrawTextW(memDC, tickText.c_str(), -1, &tickRect, DT_CENTER | DT_TOP);
                }

                int thumbX = GetThumbPosition(hwnd);
                int thumbY = (pData->sliderRect.top + pData->sliderRect.bottom) / 2;
                int thumbRadius = pData->thumbSize / 2;

                RECT thumbRect = {
                    thumbX,
                    thumbY - thumbRadius - 10,
                    thumbX + pData->thumbSize,
                    thumbY + thumbRadius + 10};

                FillRect(memDC, &thumbRect, pData->hSliderThumbBrush);

                // SelectObject(memDC, pData->hSliderThumbBrush);
                // SelectObject(memDC, pData->hThumbPen);
                // Ellipse(memDC,thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.bottom);

                Rectangle(memDC, thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.bottom);

                if (oldFont)
                {
                    SelectObject(memDC, oldFont);
                }
                SelectObject(memDC, oldPen);
                SelectObject(memDC, oldBrush);

                BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

                SelectObject(memDC, oldBitmap);
                DeleteObject(memBitmap);
                DeleteDC(memDC);

                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_LBUTTONDOWN:
            {
                if (!pData)
                    break;

                int mouseX = GET_X_LPARAM(lParam);
                int mouseY = GET_Y_LPARAM(lParam);

                // 检查是否点击在滑块上
                int thumbX = GetThumbPosition(hwnd);
                int thumbY = (pData->sliderRect.top + pData->sliderRect.bottom) / 2;
                int thumbRadius = pData->thumbSize / 2;

                RECT thumbRect = {
                    thumbX,
                    thumbY - thumbRadius,
                    thumbX + pData->thumbSize,
                    thumbY + thumbRadius};

                // 检查是否点击在滑条区域内
                POINT pt = {mouseX, mouseY};
                if (PtInRect(&thumbRect, pt) ||
                    (mouseY >= pData->sliderRect.top && mouseY <= pData->sliderRect.bottom &&
                     mouseX >= pData->sliderRect.left && mouseX <= pData->sliderRect.right))
                {

                    pData->isDragging.store(true);
                    SetCapture(hwnd);
                    UpdateValueFromMouse(hwnd, mouseX);
                    int newVal = pData->currentValue.load();
                    ShowTooltip(hwnd, newVal);
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return 0;
                }
                break;
            }
            case WM_MOUSEMOVE:
            {
                if (!pData)
                    break;

                if (pData->isDragging.load())
                {
                    int mouseX = GET_X_LPARAM(lParam);
                    UpdateValueFromMouse(hwnd, mouseX);
                    int curVal = pData->currentValue.load();
                    UpdateTooltip(hwnd, curVal);
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return 0;
                }
                break;
            }

            case WM_LBUTTONUP:
            {
                if (!pData)
                    break;

                if (pData->isDragging.load())
                {
                    HideTooltip(hwnd);
                    pData->isDragging.store(false);
                    ReleaseCapture();

                    // 触发释放回调
                    pData->currentValue.store(pData->CallCallback(DynamicSliderCallbackMessageType::Released,
                                                                  pData->currentValue.load()));

                    return 0;
                }
                break;
            }

            case WM_CLOSE:
                if (pData)
                {
                    pData->parentObject->PostMessageToThread(SliderMessageType::Close);
                }
                return 0;

            case WM_DESTROY:
                if (pData)
                {
                    pData->isFinished.store(true);

                    // 清理资源
                    Controls::Tooltip::Destroy(pData->hTooltip);
                    pData->hTooltip = nullptr;
                    if (pData->hFont)
                        DeleteObject(pData->hFont);
                    if (pData->hBackgroundBrush)
                        DeleteObject(pData->hBackgroundBrush);
                    if (pData->hSliderBgBrush)
                        DeleteObject(pData->hSliderBgBrush);
                    if (pData->hSliderThumbBrush)
                        DeleteObject(pData->hSliderThumbBrush);
                    if (pData->hBorderPen)
                        DeleteObject(pData->hBorderPen);
                    if (pData->hThumbPen)
                        DeleteObject(pData->hThumbPen);

                    pData->hwnd = nullptr;
                }

                PostQuitMessage(0);
                return 0;

            case WM_ERASEBKGND:
                return 1;

            default:
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            }

            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        // 辅助函数：获取滑块位置
        static int GetThumbPosition(HWND hwnd)
        {
            DialogData *pData = reinterpret_cast<DialogData *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!pData)
                return 0;

            int value = pData->currentValue.load();
            int min = pData->minValue.load();
            int max = pData->maxValue.load();

            if (max <= min)
                return pData->sliderRect.left;

            float ratio = static_cast<float>(value - min) / static_cast<float>(max - min);
            int range = pData->sliderRect.right - pData->sliderRect.left - pData->thumbSize;
            return pData->sliderRect.left + static_cast<int>(ratio * range);
        }

        // 辅助函数：根据鼠标位置更新值
        static void UpdateValueFromMouse(HWND hwnd, int mouseX)
        {

            DialogData *pData = reinterpret_cast<DialogData *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!pData)
                return;

            RECT sliderRect = pData->sliderRect;
            int thumbSize = pData->thumbSize;

            // 确保鼠标在滑条区域内
            mouseX = std::max<LONG>(sliderRect.left, std::min<LONG>(sliderRect.right - thumbSize, mouseX));

            int range = sliderRect.right - sliderRect.left - thumbSize;
            if (range <= 0)
                return;

            float ratio = static_cast<float>(mouseX - sliderRect.left) / static_cast<float>(range);

            int minValue = pData->minValue.load();
            int maxValue = pData->maxValue.load();
            int newValue = minValue + static_cast<int>(ratio * (maxValue - minValue));

            // 确保值在范围内
            newValue = std::max(minValue, std::min(maxValue, newValue));

            // 更新值
            newValue = pData->CallCallback(DynamicSliderCallbackMessageType::Dragging, newValue);
            pData->currentValue.store(newValue);
        }

        void DialogThreadProc()
        {
            if (!IsWindowClassRegistered())
            {
                threadRunning.store(false);
                return;
            }

            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            int windowWidth = 600;
            int windowHeight = 190;
            int x = (screenWidth - windowWidth) / 2;
            int y = (screenHeight - windowHeight) / 2;

            HWND hwnd = CreateWindowExW(
                WS_EX_DLGMODALFRAME,
                L"GL_Commdlg.DynamicSliderClass",
                dialogData->title.c_str(),
                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                x, y, windowWidth, windowHeight,
                dialogData->hwndParent,
                nullptr,
                GetModuleHandleW(nullptr),
                dialogData.get());

            if (!hwnd)
            {
                threadRunning.store(false);
                return;
            }

            dialogData->hwnd = hwnd;

            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);

            MSG msg;
            while (threadRunning.load())
            {

                {
                    std::unique_lock<std::mutex> lock(messageMutex);
                    messageCV.wait_for(lock, std::chrono::milliseconds(10),
                                       [this]()
                                       { return !messageQueue.empty(); });

                    while (!messageQueue.empty())
                    {
                        SliderMessage message = messageQueue.front();
                        messageQueue.pop();
                        lock.unlock();

                        ProcessMessage(message);
                        lock.lock();
                    }
                }

                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                    {
                        threadRunning.store(false);
                        break;
                    }

                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }

                if (!IsWindow(hwnd) || dialogData->isFinished.load())
                {
                    threadRunning.store(false);
                }
            }

            if (IsWindow(hwnd))
            {
                DestroyWindow(hwnd);
            }
        }

        void ProcessMessage(const SliderMessage &msg)
        {
            switch (msg.type)
            {
            case SliderMessageType::SetValue:
                dialogData->currentValue.store(msg.value);

                if (dialogData->hwnd && IsWindow(dialogData->hwnd))
                {
                    InvalidateRect(dialogData->hwnd, nullptr, TRUE);
                    UpdateWindow(dialogData->hwnd);
                }
                break;

            case SliderMessageType::SetRange:
            {
                dialogData->minValue.store(msg.minValue);
                dialogData->maxValue.store(msg.maxValue);

                // 确保当前值在范围内
                int current = dialogData->currentValue.load();
                if (current < msg.minValue)
                    dialogData->currentValue.store(msg.minValue);
                if (current > msg.maxValue)
                    dialogData->currentValue.store(msg.maxValue);

                if (dialogData->hwnd && IsWindow(dialogData->hwnd))
                {
                    InvalidateRect(dialogData->hwnd, nullptr, TRUE);
                    UpdateWindow(dialogData->hwnd);
                }
                break;
            }
            case SliderMessageType::SetCallback:
                dialogData->SetCallback(msg.callback);
                break;

            case SliderMessageType::Close:
                dialogData->isFinished.store(true);
                threadRunning.store(false);
                if (dialogData->hwnd && IsWindow(dialogData->hwnd))
                {
                    DestroyWindow(dialogData->hwnd);
                }
                break;
            }
        }

        void PostMessageToThread(SliderMessageType type, int value = 0,
                                 int minValue = 0, int maxValue = 100,
                                 std::function<int(DynamicSliderCallbackMessageType, int)> callback = nullptr)
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            messageQueue.emplace(type, value, minValue, maxValue, callback);
            messageCV.notify_one();
        }

    public:
        DynamicSlider(const std::string &title, const std::string &initialMessage,
                      int minValue, int maxValue, int initialValue,
                      std::function<int(DynamicSliderCallbackMessageType, int)> callback,
                      HWND hParent = nullptr)
            : dialogData(std::make_shared<DialogData>())
        {

            dialogData->parentObject = this;
            dialogData->hwndParent = hParent;
            dialogData->title = utf8ToWide(title);
            dialogData->UpdateMessage(initialMessage);
            dialogData->minValue.store(minValue);
            dialogData->maxValue.store(maxValue);
            dialogData->currentValue.store(initialValue);
            dialogData->SetCallback(callback);

            threadRunning.store(true);
            dialogThread = std::make_unique<std::thread>([this]()
                                                         { DialogThreadProc(); });
        }

        ~DynamicSlider() override
        {
            Close();

            if (dialogThread && dialogThread->joinable())
            {
                dialogThread->join();
            }
        }

        DynamicSlider(DynamicSlider &&other) noexcept
            : dialogThread(std::move(other.dialogThread)), threadRunning(other.threadRunning.load()), dialogData(std::move(other.dialogData))
        {
        }

        DynamicSlider &operator=(DynamicSlider &&other) noexcept
        {
            if (this != &other)
            {
                Close();

                if (dialogThread && dialogThread->joinable())
                {
                    dialogThread->join();
                }

                dialogThread = std::move(other.dialogThread);
                threadRunning = other.threadRunning.load();
                dialogData = std::move(other.dialogData);
            }
            return *this;
        }

        void SetValue(int value)
        {
            if (!threadRunning.load() || dialogData->isFinished.load())
            {
                return;
            }

            PostMessageToThread(SliderMessageType::SetValue, value);
        }

        void SetRange(int minValue, int maxValue)
        {
            if (!threadRunning.load() || dialogData->isFinished.load())
            {
                return;
            }

            PostMessageToThread(SliderMessageType::SetRange, 0, minValue, maxValue);
        }

        void SetCallback(std::function<int(DynamicSliderCallbackMessageType, int)> callback)
        {
            if (!threadRunning.load() || dialogData->isFinished.load())
            {
                return;
            }

            PostMessageToThread(SliderMessageType::SetCallback, 0, 0, 0, std::move(callback));
        }

        void GetSliderInfo(int &current, int &min, int &max, std::string &message)
        {
            if (!dialogData)
                return;

            current = dialogData->currentValue.load();
            min = dialogData->minValue.load();
            max = dialogData->maxValue.load();
            message = dialogData->GetMessage();
        }

        void Show() override
        {
            if (dialogData)
            {
                ShowWindow(dialogData->hwnd, SW_SHOW);
            }
        }

        void Close() override
        {
            if (threadRunning.load() && !dialogData->isFinished.load())
            {
                PostMessageToThread(SliderMessageType::Close);
            }
        }

        bool IsFinished() const
        {
            return dialogData ? dialogData->isFinished.load() : true;
        }

        HWND GetWindowHandle() const
        {
            return dialogData ? dialogData->hwnd : nullptr;
        }

        bool IsDragging() const
        {
            return dialogData ? dialogData->isDragging.load() : false;
        }
    };

    /**
     * @brief Create a dynamic slider dialog instance
     * @brief 创建滑动条动态对话框实例
     *
     * @param title Dialog title
     * @param title 对话框标题
     * @param initialMessage Initial prompt text inside the dialog
     * @param initialMessage 初始时对话框内的提示文本
     * @param minValue Minimum slider value
     * @param minValue 滑动条最小值
     * @param maxValue Maximum slider value
     * @param maxValue 滑动条最大值
     * @param initialValue Initial slider value
     * @param callbackOnValueChange Callback function for value changes; parameters are event type and current value
     * @param callbackOnValueChange 值改变时的回调函数，参数为事件类型和当前值
     * @param hParent Parent window handle
     * @param hParent 父窗口句柄
     * @return The created dynamic slider instance
     * @return 创建的滑动条动态对话框实例
     */
    DynamicSlider CreateDynamicSlider(const std::string &title,
                                      const std::string &initialMessage,
                                      int minValue,
                                      int maxValue,
                                      int initialValue,
                                      std::function<int(DynamicSliderCallbackMessageType, int)> callbackOnValueChange,
                                      HWND hParent = nullptr)
    {
        return DynamicSlider(title, initialMessage, minValue, maxValue, initialValue, callbackOnValueChange, hParent);
    }
}

#endif