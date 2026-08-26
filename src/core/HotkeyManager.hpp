#pragma once
#include <windows.h>

#define HOTKEY_SNIP_MAIN 1001
#define HOTKEY_SNIP_PRTSCN 1002

class HotkeyManager {
private:
    HWND hwnd = NULL;
    bool prtScnRegistered = false;

public:
    HotkeyManager() = default;

    bool Register(HWND hWnd) {
        hwnd = hWnd;
        
        // 1. Primary Shortcut: Ctrl + Shift + X
        BOOL res1 = RegisterHotKey(hwnd, HOTKEY_SNIP_MAIN, MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'X');

        // 2. Secondary Shortcut: PrintScreen
        BOOL res2 = RegisterHotKey(hwnd, HOTKEY_SNIP_PRTSCN, MOD_NOREPEAT, VK_SNAPSHOT);
        prtScnRegistered = (res2 != FALSE);

        return res1 != FALSE || res2 != FALSE;
    }

    void Unregister() {
        if (hwnd) {
            UnregisterHotKey(hwnd, HOTKEY_SNIP_MAIN);
            if (prtScnRegistered) {
                UnregisterHotKey(hwnd, HOTKEY_SNIP_PRTSCN);
            }
        }
    }
};
