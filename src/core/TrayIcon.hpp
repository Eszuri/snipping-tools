#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SNIP 2001
#define ID_TRAY_FOLDER 2002
#define ID_TRAY_ABOUT 2003
#define ID_TRAY_EXIT 2004

class TrayIcon {
private:
    HWND hwnd = NULL;
    NOTIFYICONDATAW nid = { 0 };
    bool isAdded = false;

public:
    TrayIcon() = default;

    ~TrayIcon() {
        Remove();
    }

    bool Create(HWND hWnd, HICON hIcon, const std::wstring& tip) {
        hwnd = hWnd;
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
        wcsncpy_s(nid.szTip, tip.c_str(), _TRUNCATE);

        isAdded = (Shell_NotifyIconW(NIM_ADD, &nid) != FALSE);
        return isAdded;
    }

    void Remove() {
        if (isAdded) {
            Shell_NotifyIconW(NIM_DELETE, &nid);
            isAdded = false;
        }
    }

    void ShowContextMenu() {
        POINT pt;
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_SNIP, L"📸 Snip Now (Ctrl+Shift+X)");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_FOLDER, L"📁 Open Screenshots Folder");
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_ABOUT, L"ℹ About Native Snipping Tool");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"✕ Exit");

        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
};
