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
 *  This header file wraps some Windows Commdlg and Windows Shell APIs related to user dialogs, making them easy to use. It also extends functionality not available in the original APIs.
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

// Link dialog libraries. If using a non-MSVC compiler, add compile parameters: -lcomdlg32 -lshell32
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

namespace {
    /**
     * @brief Converts a UTF8 string to a wide string
     * @param utf8 Input UTF8 string
     * @return Converted wide string
     * @throw std::runtime_error Thrown when conversion fails
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
     * @brief Converts a wide string to a UTF8 string
     * @param wide Input wide string
     * @return Converted UTF8 string
     * @throw std::runtime_error Thrown when conversion fails
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
     * @brief Builds file filter string (wide character version)
     * @param filters Filter list, each element in "description|filter pattern" format
     * @return Wide character filter string conforming to API requirements
     * @throw std::invalid_argument Thrown when filter format is incorrect
     */
    std::wstring buildFilter(const std::vector<std::string>& filters) {
        std::wstring filterStr;

        for (const auto& filter : filters) {
            size_t pipePos = filter.find('|');
            if (pipePos == std::string::npos) {
                throw std::invalid_argument(
                    "Invalid filter format: '" + filter + 
                    "'. Use 'description|filter pattern' (e.g., 'Text Files(*.txt)|*.txt')"
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
 * @brief Shows a file open dialog for selecting an existing file
 * @param filters File filter list, each element must follow "description|filter pattern" format:
 *                - Description: Text displayed in the filter dropdown (e.g., "Text Files(*.txt)")
 *                - Filter pattern: File matching rules (e.g., "*.txt", multiple patterns separated by semicolons "*.bmp;*.jpg")
 *                Example: {"Text Files(*.txt)|*.txt", "All Files(*.*)|*.*"}
 * @param title File open dialog window title, if empty uses default title "Open"
 * @param initialDir Initial directory (UTF8 encoded), if empty uses current working directory
 * @param defaultFileName Default displayed filename (UTF8 encoded), if empty not set
 * @param defaultExt Default extension (without dot, e.g., "txt"), automatically added when user doesn't input extension
 * @param parentHWND Parent window handle for the file open dialog
 * @return Selected file path (UTF8 encoded), returns empty string if user cancels
 * @throw std::invalid_argument Thrown when filter format is incorrect
 * @throw std::runtime_error Thrown when string conversion fails or dialog call fails
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
 * @brief Shows a file save dialog for specifying a file save path
 * @param filters File filter list, each element must follow "description|filter pattern" format:
 *                - Description: Text displayed in the filter dropdown (e.g., "Text Files(*.txt)")
 *                - Filter pattern: File matching rules (e.g., "*.txt", multiple patterns separated by semicolons "*.bmp;*.jpg")
 *                Example: {"Text Files(*.txt)|*.txt", "All Files(*.*)|*.*"}
 * @param title File save dialog window title, if empty uses default title "Save As"
 * @param initialDir Initial directory (UTF8 encoded), if empty uses current working directory
 * @param defaultFileName Default displayed filename (UTF8 encoded), if empty not set
 * @param defaultExt Default extension (without dot, e.g., "txt"), automatically added when user doesn't input extension
 * @param parentHWND Parent window handle for the file save dialog
 * @return Selected file save path (UTF8 encoded), returns empty string if user cancels
 * @throw std::invalid_argument Thrown when filter format is incorrect
 * @throw std::runtime_error Thrown when string conversion fails or dialog call fails
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
 * @brief Shows a file open dialog for selecting multiple existing files
 * @param filters File filter list, each element must follow "description|filter pattern" format:
 *                - Description: Text displayed in the filter dropdown (e.g., "Text Files(*.txt)")
 *                - Filter pattern: File matching rules (e.g., "*.txt", multiple patterns separated by semicolons "*.bmp;*.jpg")
 *                Example: {"Text Files(*.txt)|*.txt", "All Files(*.*)|*.*"}
 * @param title File open dialog window title, if empty uses default title "Open"
 * @param initialDir Initial directory (UTF8 encoded), if empty uses current working directory
 * @param defaultFileName Default displayed filename (UTF8 encoded), if empty not set
 * @param defaultExt Default extension (without dot, e.g., "txt"), automatically added when user doesn't input extension
 * @param parentHWND Parent window handle for the file open dialog
 * @return List of selected file paths (UTF8 encoded), returns empty vector if user cancels
 * @throw std::invalid_argument Thrown when filter format is incorrect
 * @throw std::runtime_error Thrown when string conversion fails or dialog call fails
 */
std::vector<std::string> getOpenMultipleFileNames(const std::vector<std::string>& filters,
                           const std::string& title = "",
                           const std::string& initialDir = "",
                           const std::string& defaultFileName = "",
                           const std::string& defaultExt = "",
                           HWND parentHWND = NULL) {
    std::wstring filter = buildFilter(filters);
    std::wstring wtitle = utf8ToWide(title);

    // Allocate larger buffer for multiple file selection (64KB)
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
 * @brief Shows a directory selection dialog for selecting a directory
 * @param title Prompt text displayed in the directory selection dialog
 * @param initialDir Initial directory (UTF8 encoded), if empty uses current working directory
 * @param parentHWND Parent window handle for the directory selection dialog
 * @return Selected directory path (UTF8 encoded), returns empty string if user cancels
 * @throw std::runtime_error Thrown when string conversion fails or dialog call fails
 * 
 * @note Since SHBrowseForFolderW doesn't support custom window titles, the parameter title refers to prompt text rather than a true "title". Also, the default window title for SHBrowseForFolderW is "Browse For Folder"
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
 * @brief Shows a color selection dialog for choosing a color
 * 
 * @param selectedColor The color selected by the user
 * @param hwndParent Parent window handle for the color selection dialog
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
 * @brief Shows a font selection dialog for choosing from system installed fonts
 * 
 * @param cfi Output parameter. Note: There's no guarantee that the path of the selected font can be found, but it's highly probable. If not found, fontPath will be an empty string.
 * @param hwndParent Parent window handle for the color selection dialog
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

#pragma region Non-Win32 Native Dialogs
// Lightweight implementation of some dialogs not available in commdlg.h using raw methods, such as prompt

#define __GCOMMMDLG_IDC_PROMPT  1001  // Prompt text
#define __GCOMMMDLG_IDC_INPUT   1002  // Input box
#define __GCOMMMDLG_IDOK        1003  // OK button
#define __GCOMMMDLG_IDCANCEL    1004  // Cancel button

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
                    L"Microsoft YaHei"
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
                    WS_EX_CLIENTEDGE,  // Use 3D border
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
                    L"OK",
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
                    L"Cancel",
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
 * @brief Shows an input dialog for user to input a string
 * 
 * @param title Input dialog title
 * @param message Prompt text displayed in the input dialog
 * @param output Output user input content
 * @param defaultContent Default content in the input field
 * @param hParent Parent window handle for the input dialog
 * @return Whether the user confirmed the input
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

#define __GCOMMMDLG_BTN_START 2000  // Option button starting ID

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
                    L"Microsoft YaHei"
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
 * @brief Shows a custom message dialog supporting multiple option buttons
 * 
 * @param title Dialog title
 * @param message Prompt text inside the dialog
 * @param options Option collection (key is return value, value is button text)
 * @param hParent Parent window handle
 * @return Selected option ID (returns 0 if window closed, returns -1 if options is empty to indicate failure)
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