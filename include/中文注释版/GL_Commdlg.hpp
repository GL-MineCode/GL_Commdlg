/*
  GL_Commdlg
  Copyright (C) 2025 Gao Li

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file GL_Commdlg.hpp
 *
 *  本头文件包装了一些Windows Commdlg还有Windows Shell中和用户对话框有关的API，使其易用。而且对其没有的功能进行了拓展。
 */


#ifndef __INC_GL_COMMDLG_
#define __INC_GL_COMMDLG_

#include <windows.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <Shlobj.h>

// 链接对话框库，若使用非MSCV编译器，请添加编译参数-lcomdlg32 -lshell32
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

namespace {
    /**
     * @brief 将UTF8字符串转换为宽字符串
     * @param utf8 输入的UTF8字符串
     * @return 转换后的宽字符串
     * @throw std::runtime_error 转换失败时抛出
     */
    std::wstring utf8ToWide(const std::string& utf8) {
        if (utf8.empty()) return L"";

        int wideSize = MultiByteToWideChar(
            CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), 
            nullptr, 0
        );
        if (wideSize == 0) {
            throw std::runtime_error("UTF8 to wide conversion failed: " + 
                std::to_string(GetLastError()));
        }

        std::wstring wide(wideSize, L'\0');
        if (MultiByteToWideChar(
            CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), 
            &wide[0], wideSize
        ) == 0) {
            throw std::runtime_error("UTF8 to wide conversion failed: " + 
                std::to_string(GetLastError()));
        }

        return wide;
    }

    /**
     * @brief 将宽字符串转换为UTF8字符串
     * @param wide 输入的宽字符串
     * @return 转换后的UTF8字符串
     * @throw std::runtime_error 转换失败时抛出
     */
    std::string wideToUtf8(const std::wstring& wide) {
        if (wide.empty()) return "";

        int utf8Size = WideCharToMultiByte(
            CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), 
            nullptr, 0, nullptr, nullptr
        );
        if (utf8Size == 0) {
            throw std::runtime_error("Wide to UTF8 conversion failed: " + 
                std::to_string(GetLastError()));
        }

        std::string utf8(utf8Size, '\0');
        if (WideCharToMultiByte(
            CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), 
            &utf8[0], utf8Size, nullptr, nullptr
        ) == 0) {
            throw std::runtime_error("Wide to UTF8 conversion failed: " + 
                std::to_string(GetLastError()));
        }

        return utf8;
    }

    /**
     * @brief 构建文件过滤器字符串（宽字符版）
     * @param filters 过滤器列表，每个元素格式为"描述|过滤模式"
     * @return 符合API要求的宽字符过滤器字符串
     * @throw std::invalid_argument 过滤器格式错误时抛出
     */
    std::wstring buildFilter(const std::vector<std::string>& filters) {
        std::wstring filterStr;

        for (const auto& filter : filters) {
            size_t pipePos = filter.find('|');
            if (pipePos == std::string::npos) {
                throw std::invalid_argument(
                    "Invalid filter format: '" + filter + 
                    "'. Use '描述|过滤模式' (e.g., 'Text Files(*.txt)|*.txt')"
                );
            }

            std::string desc = filter.substr(0, pipePos);
            std::string pattern = filter.substr(pipePos + 1);
            filterStr += utf8ToWide(desc);
            filterStr += L'\0';
            filterStr += utf8ToWide(pattern);
            filterStr += L'\0';
        }

        filterStr += L'\0';
        return filterStr;
    }

    std::wstring FindFontFileLocalMachine(const std::wstring& fontNameSubstring)
    {
        HKEY hKey;
        LONG result;
        DWORD index = 0;
        WCHAR valueName[256];
        DWORD valueNameSize;
        BYTE valueData[1024];
        DWORD valueDataSize;
        DWORD valueType;
        
        result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 
                            0, KEY_READ, &hKey);
        
        if (result != ERROR_SUCCESS) {
            return L"";
        }
        
        std::wstring fontPath;
        
        while (true) {
            valueNameSize = sizeof(valueName) / sizeof(WCHAR);
            valueDataSize = sizeof(valueData);
            
            result = RegEnumValueW(hKey, index, valueName, &valueNameSize, 
                                NULL, &valueType, valueData, &valueDataSize);
            
            if (result != ERROR_SUCCESS) {
                break;
            }
            
            index++;
            
            if (valueType == REG_SZ) {
                std::wstring currentFontName(valueName);
                std::wstring currentFontPath(reinterpret_cast<wchar_t*>(valueData));
                
                size_t pos = currentFontName.find(L" (");
                if (pos != std::wstring::npos) {
                    currentFontName = currentFontName.substr(0, pos);
                }
                
                std::wstring lowerFontName = currentFontName;
                std::wstring lowerSubstring = fontNameSubstring;
                
                for (auto& c : lowerFontName) c = towlower(c);
                for (auto& c : lowerSubstring) c = towlower(c);
                
                if (lowerFontName.find(lowerSubstring) != std::wstring::npos) {
                    
                    if (currentFontPath.find(L':') == std::wstring::npos) {
                        WCHAR windowsDir[MAX_PATH];
                        GetWindowsDirectoryW(windowsDir, MAX_PATH);
                        fontPath = std::wstring(windowsDir) + L"\\Fonts\\" + currentFontPath;
                    } else {
                        fontPath = currentFontPath;
                    }
                    break;
                }
            }
        }
        
        RegCloseKey(hKey);
        return fontPath;
    }

    std::wstring FindFontFileCurrentUser(const std::wstring& fontNameSubstring)
    {
        HKEY hKey;
        LONG result;
        DWORD index = 0;
        WCHAR valueName[256];
        DWORD valueNameSize;
        BYTE valueData[1024];
        DWORD valueDataSize;
        DWORD valueType;
        
        result = RegOpenKeyExW(HKEY_CURRENT_USER, 
                            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 
                            0, KEY_READ, &hKey);
        
        if (result != ERROR_SUCCESS) {
            return L"";
        }
        
        std::wstring fontPath;
        
        while (true) {
            valueNameSize = sizeof(valueName) / sizeof(WCHAR);
            valueDataSize = sizeof(valueData);
            
            result = RegEnumValueW(hKey, index, valueName, &valueNameSize, 
                                NULL, &valueType, valueData, &valueDataSize);
            
            if (result != ERROR_SUCCESS) {
                break;
            }
            
            index++;
            
            if (valueType == REG_SZ) {
                std::wstring currentFontName(valueName);
                std::wstring currentFontPath(reinterpret_cast<wchar_t*>(valueData));
                
                size_t pos = currentFontName.find(L" (");
                if (pos != std::wstring::npos) {
                    currentFontName = currentFontName.substr(0, pos);
                }
                
                std::wstring lowerFontName = currentFontName;
                std::wstring lowerSubstring = fontNameSubstring;
                
                for (auto& c : lowerFontName) c = towlower(c);
                for (auto& c : lowerSubstring) c = towlower(c);
                
                if (lowerFontName.find(lowerSubstring) != std::wstring::npos) {
                    
                    if (currentFontPath.find(L':') == std::wstring::npos) {
                        WCHAR windowsDir[MAX_PATH];
                        GetWindowsDirectoryW(windowsDir, MAX_PATH);
                        fontPath = std::wstring(windowsDir) + L"\\Fonts\\" + currentFontPath;
                    } else {
                        fontPath = currentFontPath;
                    }
                    break;
                }
            }
        }
        
        RegCloseKey(hKey);
        return fontPath;
    }

    std::wstring FindFontFile(const std::wstring& fontNameSubstring){
        std::wstring try_lm = FindFontFileLocalMachine(fontNameSubstring);
        if(try_lm.empty()){
            try_lm = FindFontFileCurrentUser(fontNameSubstring);
        }
        return try_lm;
    }

}

/**
 * @brief 显示文件打开对话框，让用户选择一个已存在的文件
 * @param filters 文件过滤器列表，每个元素必须遵循"描述|过滤模式"格式：
 *                - 描述：显示在过滤器下拉框中的文本（如"文本文件(*.txt)"）
 *                - 过滤模式：文件匹配规则（如"*.txt"，多模式用分号分隔"*.bmp;*.jpg"）
 *                示例：{"文本文件(*.txt)|*.txt", "所有文件(*.*)|*.*"}
 * @param title 文件打开对话框的窗口标题，若为空则使用默认的标题，即"打开"
 * @param initialDir 初始目录（UTF8编码），为空则使用当前工作目录
 * @param defaultFileName 默认显示的文件名（UTF8编码），为空则不设置
 * @param defaultExt 默认扩展名（无需带点，如"txt"），用户未输入扩展名时自动添加
 * @param parentHWND 文件打开对话框的父窗口句柄
 * @return 选中的文件路径（UTF8编码），用户取消时返回空字符串
 * @throw std::invalid_argument 过滤器格式错误时
 * @throw std::runtime_error 字符串转换失败或对话框调用出错时
 */
std::string getOpenFileName(const std::vector<std::string>& filters,
                           const std::string& title = "",
                           const std::string& initialDir = "",
                           const std::string& defaultFileName = "",
                           const std::string& defaultExt = "",HWND parentHWND = NULL) {
    std::wstring filter = buildFilter(filters);
    std::wstring wtitle = utf8ToWide(title);

    const int MAX_PATH_LEN = 4096;
    std::wstring filePath(MAX_PATH_LEN, L'\0');
    if (!defaultFileName.empty()) {
        std::wstring defaultWide = utf8ToWide(defaultFileName);
        if (defaultWide.size() >= MAX_PATH_LEN) {
            throw std::runtime_error("Default file name is too long");
        }
        wcscpy_s(&filePath[0], MAX_PATH_LEN, defaultWide.c_str());
    }

    std::wstring initialDirWide;
    LPCWSTR initialDirPtr = nullptr;
    if (!initialDir.empty()) {
        initialDirWide = utf8ToWide(initialDir);
        initialDirPtr = initialDirWide.c_str();
    }

    std::wstring defaultExtWide;
    LPCWSTR defaultExtPtr = nullptr;
    if (!defaultExt.empty()) {
        defaultExtWide = utf8ToWide(defaultExt);
        defaultExtPtr = defaultExtWide.c_str();
    }

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = parentHWND;
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = &filePath[0];
    ofn.nMaxFile = MAX_PATH_LEN;
    ofn.lpstrInitialDir = initialDirPtr;
    ofn.lpstrDefExt = defaultExtPtr;
    if(!title.empty()) ofn.lpstrTitle = wtitle.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST |
                OFN_NOCHANGEDIR |
                OFN_EXPLORER;

    if (!GetOpenFileNameW(&ofn)) {
        DWORD err = CommDlgExtendedError();
        if (err != 0) {
            throw std::runtime_error("Open file dialog failed: " + std::to_string(err));
        }
        return "";
    }

    filePath.resize(wcslen(filePath.c_str()));
    return wideToUtf8(filePath);
}

/**
 * @brief 显示文件保存对话框，让用户指定文件保存路径
 * @param filters 文件过滤器列表，每个元素必须遵循"描述|过滤模式"格式：
 *                - 描述：显示在过滤器下拉框中的文本（如"文本文件(*.txt)"）
 *                - 过滤模式：文件匹配规则（如"*.txt"，多模式用分号分隔"*.bmp;*.jpg"）
 *                示例：{"文本文件(*.txt)|*.txt", "所有文件(*.*)|*.*"}
 * @param title 文件打开对话框的窗口标题，若为空则使用默认的标题，即"另存为"
 * @param initialDir 初始目录（UTF8编码），为空则使用当前工作目录
 * @param defaultFileName 默认显示的文件名（UTF8编码），为空则不设置
 * @param defaultExt 默认扩展名（无需带点，如"txt"），用户未输入扩展名时自动添加
 * @param parentHWND 文件保存对话框的父窗口句柄
 * @return 选中的文件保存路径（UTF8编码），用户取消时返回空字符串
 * @throw std::invalid_argument 过滤器格式错误时
 * @throw std::runtime_error 字符串转换失败或对话框调用出错时
 */
std::string getSaveFileName(const std::vector<std::string>& filters,
                           const std::string& title = "",
                           const std::string& initialDir = "",
                           const std::string& defaultFileName = "",
                           const std::string& defaultExt = "",HWND parentHWND = NULL) {
    std::wstring filter = buildFilter(filters);
    std::wstring wtitle = utf8ToWide(title);

    const int MAX_PATH_LEN = 4096;
    std::wstring filePath(MAX_PATH_LEN, L'\0');
    if (!defaultFileName.empty()) {
        std::wstring defaultWide = utf8ToWide(defaultFileName);
        if (defaultWide.size() >= MAX_PATH_LEN) {
            throw std::runtime_error("Default file name is too long");
        }
        wcscpy_s(&filePath[0], MAX_PATH_LEN, defaultWide.c_str());
    }

    std::wstring initialDirWide;
    LPCWSTR initialDirPtr = nullptr;
    if (!initialDir.empty()) {
        initialDirWide = utf8ToWide(initialDir);
        initialDirPtr = initialDirWide.c_str();
    }

    std::wstring defaultExtWide;
    LPCWSTR defaultExtPtr = nullptr;
    if (!defaultExt.empty()) {
        defaultExtWide = utf8ToWide(defaultExt);
        defaultExtPtr = defaultExtWide.c_str();
    }

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = parentHWND;
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = &filePath[0];
    ofn.nMaxFile = MAX_PATH_LEN;
    ofn.lpstrInitialDir = initialDirPtr;
    ofn.lpstrDefExt = defaultExtPtr;
    if(!title.empty()) ofn.lpstrTitle = wtitle.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT |
                OFN_PATHMUSTEXIST |
                OFN_NOCHANGEDIR |
                OFN_EXPLORER;

    if (!GetSaveFileNameW(&ofn)) {
        DWORD err = CommDlgExtendedError();
        if (err != 0) {
            throw std::runtime_error("Save file dialog failed: " + std::to_string(err));
        }
        return "";
    }

    filePath.resize(wcslen(filePath.c_str()));
    return wideToUtf8(filePath);
}

/**
 * @brief 显示文件打开对话框，让用户选择多个已存在的文件
 * @param filters 文件过滤器列表，每个元素必须遵循"描述|过滤模式"格式：
 *                - 描述：显示在过滤器下拉框中的文本（如"文本文件(*.txt)"）
 *                - 过滤模式：文件匹配规则（如"*.txt"，多模式用分号分隔"*.bmp;*.jpg"）
 *                示例：{"文本文件(*.txt)|*.txt", "所有文件(*.*)|*.*"}
 * @param title 文件打开对话框的窗口标题，若为空则使用默认的标题，即"打开"
 * @param initialDir 初始目录（UTF8编码），为空则使用当前工作目录
 * @param defaultFileName 默认显示的文件名（UTF8编码），为空则不设置
 * @param defaultExt 默认扩展名（无需带点，如"txt"），用户未输入扩展名时自动添加
 * @param parentHWND 文件打开对话框的父窗口句柄
 * @return 选中的文件路径列表（UTF8编码），用户取消时返回空vector
 * @throw std::invalid_argument 过滤器格式错误时
 * @throw std::runtime_error 字符串转换失败或对话框调用出错时
 */
std::vector<std::string> getOpenMultipleFileNames(const std::vector<std::string>& filters,
                           const std::string& title = "",
                           const std::string& initialDir = "",
                           const std::string& defaultFileName = "",
                           const std::string& defaultExt = "",
                           HWND parentHWND = NULL) {
    std::wstring filter = buildFilter(filters);
    std::wstring wtitle = utf8ToWide(title);

    // 为多选文件分配更大的缓冲区（64KB）
    const int BUFFER_SIZE = 65536;
    std::vector<wchar_t> filePathBuffer(BUFFER_SIZE, L'\0');
    
    if (!defaultFileName.empty()) {
        std::wstring defaultWide = utf8ToWide(defaultFileName);
        if (defaultWide.size() * sizeof(wchar_t) >= BUFFER_SIZE) {
            throw std::runtime_error("Default file name is too long");
        }
        wcscpy_s(filePathBuffer.data(), BUFFER_SIZE / sizeof(wchar_t), defaultWide.c_str());
    }

    std::wstring initialDirWide;
    LPCWSTR initialDirPtr = nullptr;
    if (!initialDir.empty()) {
        initialDirWide = utf8ToWide(initialDir);
        initialDirPtr = initialDirWide.c_str();
    }

    std::wstring defaultExtWide;
    LPCWSTR defaultExtPtr = nullptr;
    if (!defaultExt.empty()) {
        defaultExtWide = utf8ToWide(defaultExt);
        defaultExtPtr = defaultExtWide.c_str();
    }

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = parentHWND;
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = filePathBuffer.data();
    ofn.nMaxFile = BUFFER_SIZE;
    ofn.lpstrInitialDir = initialDirPtr;
    ofn.lpstrDefExt = defaultExtPtr;
    if(!title.empty()) ofn.lpstrTitle = wtitle.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST |
                OFN_NOCHANGEDIR |
                OFN_EXPLORER |
                OFN_ALLOWMULTISELECT;

    if (!GetOpenFileNameW(&ofn)) {
        DWORD err = CommDlgExtendedError();
        if (err != 0) {
            throw std::runtime_error("Open file dialog failed: " + std::to_string(err));
        }
        return {};
    }

    std::vector<std::string> selectedFiles;
    
    const wchar_t* ptr = filePathBuffer.data();
    std::wstring directory = ptr;
    ptr += directory.length() + 1;
    
    if (*ptr == L'\0') {
        selectedFiles.push_back(wideToUtf8(directory));
    } else {
        while (*ptr != L'\0') {
            std::wstring filename = ptr;
            std::wstring fullPath = directory + L"\\" + filename;
            selectedFiles.push_back(wideToUtf8(fullPath));
            ptr += filename.length() + 1;
        }
    }

    return selectedFiles;
}

/**
 * @brief 显示目录选择对话框，让用户选择一个目录
 * @param title 目录选择对话框中显示的提示文字
 * @param initialDir 初始目录（UTF8编码），为空则使用当前工作目录
 * @param parentHWND 目录选择对话框的父窗口句柄
 * @return 选中的目录路径（UTF8编码），用户取消时返回空字符串
 * @throw std::runtime_error 字符串转换失败或对话框调用出错时
 * 
 * @note 由于SHBrowseForFolderW不支持自定义窗口标题，所以参数title指的不算是真正意义上的"标题"，应该算提示文字。另外SHBrowseForFolderW默认的窗口标题是"浏览文件夹"
 */
std::string getOpenDirectoryName(const std::string& title = "",
                                const std::string& initialDir = "",
                                HWND parentHWND = NULL) {
    std::wstring wtitle = utf8ToWide(title);
    
    std::wstring initialDirWide;
    if (!initialDir.empty()) {
        initialDirWide = utf8ToWide(initialDir);
    }

    BROWSEINFOW bi = {0};
    bi.hwndOwner = parentHWND;
    if(!title.empty()) bi.lpszTitle = wtitle.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    if (!initialDirWide.empty()) {
        bi.lParam = reinterpret_cast<LPARAM>(initialDirWide.c_str());
        bi.lpfn = [](HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) -> int {
            if (uMsg == BFFM_INITIALIZED) {
                SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, lpData);
            }
            return 0;
        };
    }

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl == nullptr) {
        return "";
    }

    std::wstring directoryPath(MAX_PATH, L'\0');
    if (!SHGetPathFromIDListW(pidl, &directoryPath[0])) {
        CoTaskMemFree(pidl);
        throw std::runtime_error("Failed to get path from ID list");
    }

    CoTaskMemFree(pidl);
    directoryPath.resize(wcslen(directoryPath.c_str()));
    return wideToUtf8(directoryPath);
}

#ifndef SDL_pixels_h_

struct SDL_Color{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

#endif

/**
 * @brief 显示颜色选择对话框，让用户选择一个颜色
 * 
 * @param selectedColor 用户所选的颜色
 * @param hwndParent 颜色选择对话框的父窗口句柄
 */
void chooseColor(SDL_Color& selectedColor, HWND hwndParent = NULL)
{
    static COLORREF customColors[16] = {0};
    
    CHOOSECOLORW cc = {0};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwndParent;
    cc.lpCustColors = customColors;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
    
    COLORREF initialColor = RGB(
        selectedColor.r,
        selectedColor.g,
        selectedColor.b
    );
    cc.rgbResult = initialColor;
	
    if (!ChooseColorW(&cc))
    {
        DWORD err = CommDlgExtendedError();
        if (err != 0) {
            throw std::runtime_error("Choose color dialog failed: " + std::to_string(err));
        }
    }

    selectedColor.r = GetRValue(cc.rgbResult);
    selectedColor.g = GetGValue(cc.rgbResult);
    selectedColor.b = GetBValue(cc.rgbResult);
    selectedColor.a = 255;
}

struct chooseFontInfo{
    std::string fontFaceName;
    std::string fontPath;
    int fontPointSize;
};

/**
 * @brief 显示字体选择对话框，让用户选择系统上所安装的字体
 * 
 * @param cfi 输出参数，注意，不保证一定可以找到选择的字体的路径，但大概率可以找到。若没找到，fontPath为空字符串。
 * @param hwndParent 颜色选择对话框的父窗口句柄
 */
void chooseFont(chooseFontInfo& cfi, HWND hwndParent = NULL){
    CHOOSEFONTW cf = {0};
    LOGFONTW lf = {0};
    cf.lStructSize = sizeof(CHOOSEFONTW);
    cf.hwndOwner = hwndParent;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_NOVERTFONTS | CF_TTONLY;
    if (!ChooseFontW(&cf)) {
        DWORD err = CommDlgExtendedError();
        if (err != 0) {
            throw std::runtime_error("Choose font dialog failed: " + std::to_string(err));
        }
    }
    cfi.fontFaceName = wideToUtf8(lf.lfFaceName);
    cfi.fontPointSize = cf.iPointSize / 10;
    cfi.fontPath = wideToUtf8(FindFontFile(lf.lfFaceName));
}

#pragma region 非Win32原生对话框
//使用原始的方法轻量级实现一些commdlg.h没有实现的对话框，比如prompt

#define __GCOMMMDLG_IDC_PROMPT  1001  // 提示文本
#define __GCOMMMDLG_IDC_INPUT   1002  // 输入框
#define __GCOMMMDLG_IDOK        1003  // 确定按钮
#define __GCOMMMDLG_IDCANCEL    1004  // 取消按钮

namespace{

    WCHAR* g_inputText = nullptr;
    std::wstring g_defalutContent;
    std::wstring g_message;
    bool g_did_confirm;

    LRESULT CALLBACK PromptDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        
        static HWND hStaticPrompt = NULL;
        static HWND hEditInput = NULL;
        static HWND hButtonOK = NULL;
        static HWND hButtonCancel = NULL;
        static HBRUSH hDefaultBrush = CreateSolidBrush(RGB(240, 240, 240)); 
        
        switch (msg) {
            case WM_CREATE: {
                
                HFONT hFont = CreateFontW(
                    24, 0, 0, 0, FW_NORMAL, 
                    FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"微软雅黑"
                );
                
                hStaticPrompt = CreateWindowExW(
                    0,
                    L"STATIC",
                    g_message.c_str(),
                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                    20, 20, 260, 25,
                    hDlg,
                    (HMENU)__GCOMMMDLG_IDC_PROMPT,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL
                );
                
                hEditInput = CreateWindowExW(
                    WS_EX_CLIENTEDGE,  // 使用3D边框
                    L"EDIT",
                    g_defalutContent.c_str(),
                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                    20, 50, 360, 30,
                    hDlg,
                    (HMENU)__GCOMMMDLG_IDC_INPUT,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL
                );
                
                hButtonOK = CreateWindowExW(
                    0,
                    L"BUTTON",
                    L"确定",
                    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                    120, 95, 80, 30,
                    hDlg,
                    (HMENU)__GCOMMMDLG_IDOK,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL
                );
                
                hButtonCancel = CreateWindowExW(
                    0,
                    L"BUTTON",
                    L"取消",
                    WS_CHILD | WS_VISIBLE,
                    220, 95, 80, 30,
                    hDlg,
                    (HMENU)__GCOMMMDLG_IDCANCEL,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL
                );
                
                if (hFont) {
                    SendMessage(hStaticPrompt, WM_SETFONT, (WPARAM)hFont, TRUE);
                    SendMessage(hEditInput, WM_SETFONT, (WPARAM)hFont, TRUE);
                    SendMessage(hButtonOK, WM_SETFONT, (WPARAM)hFont, TRUE);
                    SendMessage(hButtonCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
                }
                
                return 0;
            }

            case WM_SIZE: {
                int clientWidth = LOWORD(lParam);
                int clientHeight = HIWORD(lParam);
                
                if (hStaticPrompt) {
                    SetWindowPos(hStaticPrompt, NULL, 
                        20, 20, 
                        clientWidth - 40, 25, 
                        SWP_NOZORDER);
                }
                
                if (hEditInput) {
                    SetWindowPos(hEditInput, NULL, 
                        20, 55, 
                        clientWidth - 40, 30, 
                        SWP_NOZORDER);
                }
                
                if (hButtonOK && hButtonCancel) {
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

            case WM_COMMAND: {
                if (LOWORD(wParam) == __GCOMMMDLG_IDOK) {
                    WCHAR buffer[256] = {0};
                    GetDlgItemTextW(hDlg, __GCOMMMDLG_IDC_INPUT, buffer, 256);
                    wcscpy(g_inputText, buffer);
                    g_did_confirm = true;
                    DestroyWindow(hDlg);
                }
                else if (LOWORD(wParam) == __GCOMMMDLG_IDCANCEL) {
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

            case WM_CTLCOLOREDIT: {
                HDC hdc = (HDC)wParam;
                SetBkColor(hdc, RGB(240, 240, 240));
                SetTextColor(hdc,RGB(0, 0, 0));
                return (LRESULT)hDefaultBrush;
            }

            case WM_CTLCOLORSTATIC: {
                HDC hdc = (HDC)wParam;
                SetBkColor(hdc, RGB(240, 240, 240));
                SetTextColor(hdc,RGB(0, 0, 0));
                return (LRESULT)hDefaultBrush;
            }

            case WM_CTLCOLORBTN: {
                HDC hdc = (HDC)wParam;
                SetBkColor(hdc, RGB(240, 240, 240));
                SetTextColor(hdc,RGB(0, 0, 0));
                return (LRESULT)hDefaultBrush;
            }

            case WM_DESTROY: {
                PostQuitMessage(0);
                return 0;
            }

            case WM_ERASEBKGND: {
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
 * @brief 显示输入对话框，用于让用户输入一段字符串
 * 
 * @param title 输入对话框的标题
 * @param message 输入对话框内显示的提示文本
 * @param output 输出用户输入的内容
 * @param defaultContent 输入栏内的默认内容
 * @param hParent 输入对话框的父窗口句柄
 * @return 用户是否确认了输入
 */
bool promptDialog(std::string title,std::string message,std::string& output,std::string defaultContent = "",HWND hParent = NULL) {
    
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = PromptDialogProc;
    wc.hInstance     = GetModuleHandleW(NULL);
    wc.lpszClassName = L"PromptDialogClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style         = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    int targetWidth = GetSystemMetrics(SM_CXSCREEN);
    int targetHeight = GetSystemMetrics(SM_CYSCREEN);

    int x = targetWidth / 2 - 400 / 2,y = targetHeight / 2 - 180 / 2;

    g_inputText = new wchar_t[256];
    g_message = utf8ToWide(message);
    g_defalutContent = utf8ToWide(defaultContent);

    HWND hDlg = CreateWindowExW(
        0,
        L"PromptDialogClass",
        utf8ToWide(title).c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME/* | WS_SIZEBOX*/,
        x, y, 400, 180,
        hParent,
        NULL,
        GetModuleHandleW(NULL),
        NULL
    );

    if (hDlg) {
        ShowWindow(hDlg, SW_SHOW);
        UpdateWindow(hDlg);

        MSG msg;
        
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    UnregisterClassW(L"PromptDialogClass", GetModuleHandleW(NULL));

    if(!g_did_confirm){
        output = "";
        return false;
    }

    output = wideToUtf8(g_inputText);
    delete[] g_inputText;
    return true;
}

#define __GCOMMMDLG_BTN_START 2000  // 选项按钮起始ID

#define __GCOMMDLG_MSGBOX_BTN_WIDTH 100

namespace {

    std::vector<std::pair<int, std::wstring>> g_options;
    std::wstring g_msgContent;
    std::wstring g_boxTitle;
    int g_selectedId = 0;

    LRESULT CALLBACK MessageBoxDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        
        static HWND hStaticMsg = NULL;
        static std::vector<HWND> hButtons;
        static HBRUSH hDefaultBrush = CreateSolidBrush(RGB(240, 240, 240));
        static HFONT hFont = NULL;

        switch (msg) {
            case WM_CREATE: {
                
                hFont = CreateFontW(
                    24, 0, 0, 0, FW_NORMAL, 
                    FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"微软雅黑"
                );

                hStaticMsg = CreateWindowExW(
                    0,
                    L"STATIC",
                    g_msgContent.c_str(),
                    WS_CHILD | WS_VISIBLE | SS_LEFT | SS_WORDELLIPSIS,
                    20, 20, 360, 26,
                    hDlg,
                    (HMENU)1001,
                    ((LPCREATESTRUCTW)lParam)->hInstance,
                    NULL
                );

                hButtons.reserve(g_options.size());
                for (size_t i = 0; i < g_options.size(); ++i) {
                    HWND hBtn = CreateWindowExW(
                        0,
                        L"BUTTON",
                        g_options[i].second.c_str(),
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        20, 120, __GCOMMDLG_MSGBOX_BTN_WIDTH, 30,
                        hDlg,
                        (HMENU)(__GCOMMMDLG_BTN_START + i),
                        ((LPCREATESTRUCTW)lParam)->hInstance,
                        NULL
                    );
                    hButtons.push_back(hBtn);
                }

                if (hFont) {
                    SendMessage(hStaticMsg, WM_SETFONT, (WPARAM)hFont, TRUE);
                    for (HWND hBtn : hButtons) {
                        SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
                    }
                }

                return 0;
            }

            case WM_SIZE: {
                int clientWidth = LOWORD(lParam);
                int clientHeight = HIWORD(lParam);

                if (hStaticMsg) {
                    SetWindowPos(hStaticMsg, NULL,
                        20, 20,
                        clientWidth - 40, 26,
                        SWP_NOZORDER);
                }

                if (!hButtons.empty()) {
                    const int btnWidth = __GCOMMDLG_MSGBOX_BTN_WIDTH;
                    const int btnHeight = 30;
                    const int spacing = 20;
                    int startX = 20;
                    int startY = clientHeight - btnHeight - 20;

                    for (size_t i = 0; i < hButtons.size(); ++i) {
                        SetWindowPos(hButtons[i], NULL,
                            startX + (i % 3) * (btnWidth + spacing), startY - (i / 3) * 40,
                            btnWidth, btnHeight,
                            SWP_NOZORDER);
                    }
                }
                return 0;
            }

            case WM_COMMAND: {
                int btnId = LOWORD(wParam);
                if (btnId >= __GCOMMMDLG_BTN_START && btnId < __GCOMMMDLG_BTN_START + (int)g_options.size()) {
                    size_t index = btnId - __GCOMMMDLG_BTN_START;
                    g_selectedId = g_options[index].first;
                    DestroyWindow(hDlg);
                }
                return 0;
            }

            case WM_CLOSE: {
                g_selectedId = 0;
                DestroyWindow(hDlg);
                return 0;
            }

            case WM_DESTROY: {
                
                if (hFont) {
                    DeleteObject(hFont);
                    hFont = NULL;
                }
                hButtons.clear();
                PostQuitMessage(0);
                return 0;
            }

            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLORBTN: {
                HDC hdc = (HDC)wParam;
                SetBkColor(hdc, RGB(240, 240, 240));
                SetTextColor(hdc, RGB(0, 0, 0));
                return (LRESULT)hDefaultBrush;
            }

            case WM_ERASEBKGND: {
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
 * @brief 显示自定义消息对话框，支持多个选项按钮
 * 
 * @param title 对话框标题
 * @param message 对话框内的提示文本
 * @param options 选项集合（键为返回值，值为按钮文本）
 * @param hParent 父窗口句柄
 * @return 选中的选项ID（关闭窗口返回0，要是你传入的options没有元素则返回-1以告知失败）
 */
int messageBox(std::string title, std::string message, const std::vector<std::pair<int, std::string>>& options, HWND hParent = NULL) {

    if(options.empty()) return -1;
    
    g_options.clear();
    for (const auto& opt : options) {
        g_options.emplace_back(opt.first, utf8ToWide(opt.second));
    }
    g_msgContent = utf8ToWide(message);
    g_boxTitle = utf8ToWide(title);
    g_selectedId = 0;

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MessageBoxDialogProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"CustomMessageBoxClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassExW(&wc)) {
        return 0;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = (__GCOMMDLG_MSGBOX_BTN_WIDTH + 20) * 3 + 20;
    int windowHeight = 140 + 40 * ((options.size() - 1) / 3);
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    HWND hDlg = CreateWindowExW(
        0,
        L"CustomMessageBoxClass",
        g_boxTitle.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
        x, y, windowWidth, windowHeight,
        hParent,
        NULL,
        GetModuleHandleW(NULL),
        NULL
    );

    if (hDlg) {
        ShowWindow(hDlg, SW_SHOW);
        UpdateWindow(hDlg);

        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    UnregisterClassW(L"CustomMessageBoxClass", GetModuleHandleW(NULL));
    g_options.clear();

    return g_selectedId;
}


#endif
