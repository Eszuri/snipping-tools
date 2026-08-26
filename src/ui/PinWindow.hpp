#pragma once
#include "../core/Common.hpp"
#include "../utils/ClipboardHelper.hpp"

class PinWindow {
private:
    HWND hwnd = NULL;
    std::unique_ptr<Gdiplus::Bitmap> pinnedBitmap;
    BYTE currentAlpha = 255;
    bool isDragging = false;
    POINT dragStartPoint = { 0, 0 };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        PinWindow* self = (PinWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            self = (PinWindow*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        }

        if (self) {
            return self->HandleMessage(hwnd, msg, wParam, lParam);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            if (pinnedBitmap) {
                Gdiplus::Graphics g(hdc);
                g.DrawImage(pinnedBitmap.get(), 0, 0);

                // Draw subtle border around pinned window
                Gdiplus::Pen borderPen(Gdiplus::Color(180, 0, 120, 215), 2.0f);
                g.DrawRectangle(&borderPen, 0, 0, (INT)pinnedBitmap->GetWidth() - 1, (INT)pinnedBitmap->GetHeight() - 1);
            }
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            SetCapture(hWnd);
            isDragging = true;
            GetCursorPos(&dragStartPoint);
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (isDragging) {
                POINT cur;
                GetCursorPos(&cur);
                int dx = cur.x - dragStartPoint.x;
                int dy = cur.y - dragStartPoint.y;

                RECT rc;
                GetWindowRect(hWnd, &rc);
                SetWindowPos(hWnd, NULL, rc.left + dx, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                dragStartPoint = cur;
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (isDragging) {
                isDragging = false;
                ReleaseCapture();
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            // Adjust opacity with mouse wheel
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int newAlpha = (int)currentAlpha + (delta > 0 ? 25 : -25);
            if (newAlpha < 50) newAlpha = 50;
            if (newAlpha > 255) newAlpha = 255;
            currentAlpha = (BYTE)newAlpha;
            SetLayeredWindowAttributes(hWnd, 0, currentAlpha, LWA_ALPHA);
            return 0;
        }

        case WM_RBUTTONUP: {
            // Right-click context menu
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 101, L"Copy to Clipboard");
            AppendMenuW(hMenu, MF_STRING, 102, L"Close Pin (Esc)");

            SetForegroundWindow(hWnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);

            if (cmd == 101 && pinnedBitmap) {
                ClipboardHelper::CopyBitmapToClipboard(pinnedBitmap.get(), hWnd);
            } else if (cmd == 102) {
                DestroyWindow(hWnd);
            }
            return 0;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_NCDESTROY: {
            delete this;
            return 0;
        }
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

public:
    PinWindow(std::unique_ptr<Gdiplus::Bitmap> bmp, int screenX, int screenY)
        : pinnedBitmap(std::move(bmp)) {
        
        static bool registered = false;
        static const WCHAR* className = L"NativeSnipPinWindow";

        if (!registered) {
            WNDCLASSEXW wc = { 0 };
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = PinWindow::WndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = className;
            wc.hCursor = LoadCursor(NULL, IDC_SIZEALL);
            wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
            RegisterClassExW(&wc);
            registered = true;
        }

        int w = pinnedBitmap ? pinnedBitmap->GetWidth() : 200;
        int h = pinnedBitmap ? pinnedBitmap->GetHeight() : 150;

        hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            className,
            L"Pinned Snip",
            WS_POPUP | WS_VISIBLE,
            screenX, screenY, w, h,
            NULL, NULL, GetModuleHandle(NULL), this
        );

        if (hwnd) {
            SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }
    }

    static void Create(std::unique_ptr<Gdiplus::Bitmap> bmp, int x, int y) {
        new PinWindow(std::move(bmp), x, y);
    }
};
