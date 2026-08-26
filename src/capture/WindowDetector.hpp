#pragma once
#include "../core/Common.hpp"

class WindowDetector {
public:
    static bool GetWindowRectAtPoint(POINT ptScreen, RECT& outRect) {
        HWND hwnd = WindowFromPoint(ptScreen);
        if (!hwnd) return false;

        // Traverse to top-level parent window (not desktop)
        HWND root = GetAncestor(hwnd, GA_ROOT);
        if (root && root != GetDesktopWindow()) {
            hwnd = root;
        }

        // Try DwmGetWindowAttribute for accurate visual bounds excluding invisible shadows
        RECT dwmRect;
        HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &dwmRect, sizeof(RECT));
        if (SUCCEEDED(hr)) {
            outRect = dwmRect;
            return true;
        }

        // Fallback to standard GetWindowRect
        return GetWindowRect(hwnd, &outRect) != FALSE;
    }
};
