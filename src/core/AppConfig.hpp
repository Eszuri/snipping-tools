#pragma once
#include "Common.hpp"

enum class AnnotationTool {
    None,
    Rectangle,
    Ellipse,
    Arrow,
    Pen,
    Highlighter,
    StepBadge,
    Pixelate,
    Text
};

enum class SnipMode {
    Rectangle,
    Window,
    FullScreen
};

enum class NamingMode {
    Timestamp, // Snip_YYYYMMDD_HHMMSS
    Static     // Custom static name e.g. Screenshot
};

struct AppConfig {
    COLORREF currentColor = RGB(235, 30, 60); // Default vibrant red
    int currentStrokeWidth = 4;
    int currentFontSize = 16;
    AnnotationTool currentTool = AnnotationTool::Pen;
    SnipMode snipMode = SnipMode::Rectangle;
    int delaySeconds = 0; // 0, 3, 5, 10
    bool autoCopyOnSave = true; // Copy to clipboard only after saving (Save / Save As)
    bool showMagnifier = true;
    bool openExplorerAfterSave = false;
    int magnifierZoom = 4;
    std::wstring customSaveDir = L""; // User custom save directory

    NamingMode namingMode = NamingMode::Timestamp;
    std::wstring staticFilename = L"Screenshot";

    std::wstring GetEffectiveSaveDir() const {
        if (!customSaveDir.empty()) {
            CreateDirectoryW(customSaveDir.c_str(), NULL);
            return customSaveDir;
        }
        return GetDefaultSaveDir();
    }

    void LoadFromRegistry() {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\NativeSnippingTool", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwType = REG_DWORD;
            DWORD dwVal = 0;
            DWORD dwSize = sizeof(DWORD);

            if (RegQueryValueExW(hKey, L"CurrentColor", NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
                currentColor = (COLORREF)dwVal;
            }
            dwSize = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"CurrentStrokeWidth", NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
                currentStrokeWidth = (int)dwVal;
            }
            dwSize = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"SnipMode", NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
                snipMode = (SnipMode)dwVal;
            }
            dwSize = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"DelaySeconds", NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
                delaySeconds = (int)dwVal;
            }
            dwSize = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"NamingMode", NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
                namingMode = (NamingMode)dwVal;
            }
            dwSize = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"AutoCopy", NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
                autoCopyOnSave = (dwVal != 0);
            }
            dwSize = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"ShowMagnifier", NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
                showMagnifier = (dwVal != 0);
            }
            dwSize = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"OpenExplorerAfterSave", NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
                openExplorerAfterSave = (dwVal != 0);
            }

            WCHAR szBuffer[MAX_PATH] = { 0 };
            DWORD strSize = sizeof(szBuffer);
            dwType = REG_SZ;
            if (RegQueryValueExW(hKey, L"SaveDirectory", NULL, &dwType, (LPBYTE)szBuffer, &strSize) == ERROR_SUCCESS) {
                customSaveDir = szBuffer;
            }

            strSize = sizeof(szBuffer);
            if (RegQueryValueExW(hKey, L"StaticFilename", NULL, &dwType, (LPBYTE)szBuffer, &strSize) == ERROR_SUCCESS) {
                staticFilename = szBuffer;
            }

            RegCloseKey(hKey);
        }
    }

    void SaveToRegistry() const {
        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\NativeSnippingTool", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            DWORD dwVal = (DWORD)currentColor;
            RegSetValueExW(hKey, L"CurrentColor", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));

            dwVal = (DWORD)currentStrokeWidth;
            RegSetValueExW(hKey, L"CurrentStrokeWidth", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));

            dwVal = (DWORD)snipMode;
            RegSetValueExW(hKey, L"SnipMode", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));

            dwVal = (DWORD)delaySeconds;
            RegSetValueExW(hKey, L"DelaySeconds", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));

            dwVal = (DWORD)namingMode;
            RegSetValueExW(hKey, L"NamingMode", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));

            dwVal = autoCopyOnSave ? 1 : 0;
            RegSetValueExW(hKey, L"AutoCopy", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));

            dwVal = showMagnifier ? 1 : 0;
            RegSetValueExW(hKey, L"ShowMagnifier", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));

            dwVal = openExplorerAfterSave ? 1 : 0;
            RegSetValueExW(hKey, L"OpenExplorerAfterSave", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));

            if (!customSaveDir.empty()) {
                RegSetValueExW(hKey, L"SaveDirectory", 0, REG_SZ, (const BYTE*)customSaveDir.c_str(), (DWORD)((customSaveDir.size() + 1) * sizeof(WCHAR)));
            } else {
                RegDeleteValueW(hKey, L"SaveDirectory");
            }

            if (!staticFilename.empty()) {
                RegSetValueExW(hKey, L"StaticFilename", 0, REG_SZ, (const BYTE*)staticFilename.c_str(), (DWORD)((staticFilename.size() + 1) * sizeof(WCHAR)));
            }

            RegDeleteValueW(hKey, L"DefaultFormat");

            RegCloseKey(hKey);
        }
    }

    static std::wstring GetDefaultSaveDir() {
        PWSTR path = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Pictures, 0, NULL, &path))) {
            std::wstring result = path;
            CoTaskMemFree(path);
            result += L"\\Screenshots";
            CreateDirectoryW(result.c_str(), NULL);
            return result;
        }
        return L".";
    }
};
