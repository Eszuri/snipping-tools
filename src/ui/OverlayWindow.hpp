#pragma once
#include "../core/Common.hpp"
#include "../core/AppConfig.hpp"
#include "../capture/ScreenCapture.hpp"
#include "../capture/WindowDetector.hpp"
#include "MagnifierView.hpp"
#include <functional>

enum class OverlayDragState {
    None,
    CreatingSelection
};

class OverlayWindow {
private:
    HWND hwnd = NULL;
    AppConfig config;
    ScreenBounds screenBounds;
    std::unique_ptr<Gdiplus::Bitmap> backgroundBmp;

    RECT selRect = { 0, 0, 0, 0 };
    OverlayDragState dragState = OverlayDragState::None;
    POINT dragStartPt = { 0, 0 };
    POINT currentCursorLocal = { 0, 0 };

    RECT hoverWindowRect = { 0, 0, 0, 0 };
    bool hasHoverWindow = false;

    std::function<void(std::unique_ptr<Gdiplus::Bitmap>)> onCompleteCallback;
    std::function<void()> onCancelCallback;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        OverlayWindow* self = (OverlayWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            self = (OverlayWindow*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        }

        if (self) {
            return self->HandleMessage(hwnd, msg, wParam, lParam);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void FinishWithBitmap(std::unique_ptr<Gdiplus::Bitmap> bmp) {
        auto cb = onCompleteCallback;
        CloseOverlay();
        if (cb && bmp) {
            cb(std::move(bmp));
        }
    }

    void CancelOverlay() {
        auto cb = onCancelCallback;
        CloseOverlay();
        if (cb) {
            cb();
        }
    }

public:
    OverlayWindow() = default;

    bool StartCapture(
        const AppConfig& appConfig,
        std::function<void(std::unique_ptr<Gdiplus::Bitmap>)> onComplete,
        std::function<void()> onCancel
    ) {
        config = appConfig;
        onCompleteCallback = onComplete;
        onCancelCallback = onCancel;

        // If delay is requested, wait before capturing screen (allows opening popup menus)
        if (config.delaySeconds > 0) {
            Sleep(config.delaySeconds * 1000);
        }

        screenBounds = ScreenCapture::GetVirtualScreenBounds();
        backgroundBmp = ScreenCapture::CaptureVirtualScreen(screenBounds);
        if (!backgroundBmp) return false;

        // If FullScreen mode is requested, instantly finish with full screen bitmap
        if (config.snipMode == SnipMode::FullScreen) {
            auto fullBmp = ScreenCapture::CropBitmap(backgroundBmp.get(), 0, 0, screenBounds.width, screenBounds.height);
            if (onCompleteCallback && fullBmp) {
                onCompleteCallback(std::move(fullBmp));
            }
            return true;
        }

        selRect = { 0, 0, 0, 0 };
        dragState = OverlayDragState::None;
        hasHoverWindow = false;

        static bool registered = false;
        static const WCHAR* className = L"NativeSnipOverlayCaptureWindow";

        if (!registered) {
            WNDCLASSEXW wc = { 0 };
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = OverlayWindow::WndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = className;
            wc.hCursor = LoadCursor(NULL, IDC_CROSS);
            wc.hbrBackground = NULL;
            RegisterClassExW(&wc);
            registered = true;
        }

        hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            className,
            L"Snip Capture",
            WS_POPUP | WS_VISIBLE,
            screenBounds.x, screenBounds.y, screenBounds.width, screenBounds.height,
            NULL, NULL, GetModuleHandle(NULL), this
        );

        if (!hwnd) return false;

        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        return true;
    }

    void CloseOverlay() {
        if (hwnd) {
            HWND h = hwnd;
            hwnd = NULL;
            DestroyWindow(h);
        }
        backgroundBmp.reset();
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // Double Buffering
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hBmpMem = CreateCompatibleBitmap(hdc, screenBounds.width, screenBounds.height);
            HGDIOBJ hOld = SelectObject(hdcMem, hBmpMem);

            Gdiplus::Graphics g(hdcMem);
            g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

            // 1. Base captured screen
            if (backgroundBmp) {
                g.DrawImage(backgroundBmp.get(), 0, 0, screenBounds.width, screenBounds.height);
            }

            // 2. Dim mask
            Gdiplus::SolidBrush dimBrush(Gdiplus::Color(140, 0, 0, 0));

            if (dragState != OverlayDragState::CreatingSelection) {
                // Hover window snap or full dimmed screen
                if (hasHoverWindow && config.snipMode == SnipMode::Window) {
                    g.FillRectangle(&dimBrush, 0, 0, screenBounds.width, screenBounds.height);

                    int hx = hoverWindowRect.left - screenBounds.x;
                    int hy = hoverWindowRect.top - screenBounds.y;
                    int hw = hoverWindowRect.right - hoverWindowRect.left;
                    int hh = hoverWindowRect.bottom - hoverWindowRect.top;

                    if (backgroundBmp) {
                        g.DrawImage(backgroundBmp.get(), Gdiplus::Rect(hx, hy, hw, hh), (INT)hx, (INT)hy, (INT)hw, (INT)hh, Gdiplus::UnitPixel);
                    }

                    Gdiplus::Pen snapPen(Gdiplus::Color(255, 0, 150, 255), 2.5f);
                    g.DrawRectangle(&snapPen, (INT)hx, (INT)hy, (INT)hw, (INT)hh);
                } else {
                    g.FillRectangle(&dimBrush, 0, 0, screenBounds.width, screenBounds.height);

                    // Crosshair guides
                    Gdiplus::Pen guidePen(Gdiplus::Color(70, 255, 255, 255), 1.0f);
                    g.DrawLine(&guidePen, (INT)0, (INT)currentCursorLocal.y, (INT)screenBounds.width, (INT)currentCursorLocal.y);
                    g.DrawLine(&guidePen, (INT)currentCursorLocal.x, (INT)0, (INT)currentCursorLocal.x, (INT)screenBounds.height);
                }

                // Magnifier Loupe
                if (config.showMagnifier) {
                    POINT ptVirt = { currentCursorLocal.x + screenBounds.x, currentCursorLocal.y + screenBounds.y };
                    MagnifierView::Draw(g, backgroundBmp.get(), ptVirt, screenBounds.x, screenBounds.y, screenBounds.width, screenBounds.height);
                }
            } else {
                // LIVE PREVIEW DURING DRAG
                int sl = (std::min)(selRect.left, selRect.right);
                int st = (std::min)(selRect.top, selRect.bottom);
                int sr = (std::max)(selRect.left, selRect.right);
                int sb = (std::max)(selRect.top, selRect.bottom);
                int sw = sr - sl;
                int sh = sb - st;

                // Dim areas outside selection (inside remains bright and clear)
                g.FillRectangle(&dimBrush, 0, 0, screenBounds.width, st);
                g.FillRectangle(&dimBrush, 0, sb, screenBounds.width, screenBounds.height - sb);
                g.FillRectangle(&dimBrush, 0, st, sl, sh);
                g.FillRectangle(&dimBrush, sr, st, screenBounds.width - sr, sh);

                // Dual-contrast border
                Gdiplus::Pen outerGlow(Gdiplus::Color(120, 255, 255, 255), 2.5f);
                g.DrawRectangle(&outerGlow, sl, st, sw, sh);

                Gdiplus::Pen selPen(Gdiplus::Color(255, 0, 140, 255), 1.5f);
                g.DrawRectangle(&selPen, sl, st, sw, sh);

                // Crosshair lines
                Gdiplus::Pen guidePen(Gdiplus::Color(60, 0, 140, 255), 1.0f);
                g.DrawLine(&guidePen, (INT)0, (INT)currentCursorLocal.y, (INT)screenBounds.width, (INT)currentCursorLocal.y);
                g.DrawLine(&guidePen, (INT)currentCursorLocal.x, (INT)0, (INT)currentCursorLocal.x, (INT)screenBounds.height);

                // Live Dimension Badge
                if (sw > 0 && sh > 0) {
                    std::wstringstream dimSs;
                    dimSs << sw << L" × " << sh << L" px";

                    int badgeW = 108;
                    int badgeH = 22;
                    int badgeX = sl;
                    int badgeY = st - badgeH - 6;
                    if (badgeY < 6) {
                        badgeY = st + 6;
                    }

                    Gdiplus::SolidBrush badgeBg(Gdiplus::Color(230, 20, 22, 26));
                    g.FillRectangle(&badgeBg, badgeX, badgeY, badgeW, badgeH);

                    Gdiplus::Pen badgeBorder(Gdiplus::Color(255, 0, 140, 255), 1.0f);
                    g.DrawRectangle(&badgeBorder, badgeX, badgeY, badgeW, badgeH);

                    Gdiplus::Font fontBadge(L"Segoe UI", 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
                    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));
                    Gdiplus::StringFormat sf;
                    sf.SetAlignment(Gdiplus::StringAlignmentCenter);
                    sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
                    g.DrawString(dimSs.str().c_str(), -1, &fontBadge, Gdiplus::RectF((float)badgeX, (float)badgeY, (float)badgeW, (float)badgeH), &sf, &textBrush);
                }

                // Magnifier Loupe
                if (config.showMagnifier) {
                    POINT ptVirt = { currentCursorLocal.x + screenBounds.x, currentCursorLocal.y + screenBounds.y };
                    MagnifierView::Draw(g, backgroundBmp.get(), ptVirt, screenBounds.x, screenBounds.y, screenBounds.width, screenBounds.height, sw, sh);
                }
            }

            BitBlt(hdc, 0, 0, screenBounds.width, screenBounds.height, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hOld);
            DeleteObject(hBmpMem);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            currentCursorLocal = { x, y };

            if (dragState == OverlayDragState::CreatingSelection) {
                selRect.right = x;
                selRect.bottom = y;
                InvalidateRect(hWnd, NULL, FALSE);
            } else {
                POINT ptVirt = { x + screenBounds.x, y + screenBounds.y };
                RECT detected;
                if (WindowDetector::GetWindowRectAtPoint(ptVirt, detected)) {
                    hoverWindowRect = detected;
                    hasHoverWindow = true;
                } else {
                    hasHoverWindow = false;
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            SetCapture(hWnd);
            dragStartPt = { x, y };
            dragState = OverlayDragState::CreatingSelection;
            selRect = { (LONG)x, (LONG)y, (LONG)x, (LONG)y };
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        case WM_LBUTTONUP: {
            ReleaseCapture();

            int sl = (std::min)(selRect.left, selRect.right);
            int st = (std::min)(selRect.top, selRect.bottom);
            int sr = (std::max)(selRect.left, selRect.right);
            int sb = (std::max)(selRect.top, selRect.bottom);
            int sw = sr - sl;
            int sh = sb - st;

            if (sw > 8 && sh > 8) {
                // CROP AND RETURN IMMEDIATELY TO MAIN APP PREVIEW
                auto cropped = ScreenCapture::CropBitmap(backgroundBmp.get(), sl, st, sw, sh);
                FinishWithBitmap(std::move(cropped));
                return 0;
            } else if (hasHoverWindow && config.snipMode == SnipMode::Window) {
                int hx = hoverWindowRect.left - screenBounds.x;
                int hy = hoverWindowRect.top - screenBounds.y;
                int hw = hoverWindowRect.right - hoverWindowRect.left;
                int hh = hoverWindowRect.bottom - hoverWindowRect.top;

                auto cropped = ScreenCapture::CropBitmap(backgroundBmp.get(), hx, hy, hw, hh);
                FinishWithBitmap(std::move(cropped));
                return 0;
            }

            dragState = OverlayDragState::None;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        case WM_RBUTTONDOWN:
        case WM_KEYDOWN: {
            if (msg == WM_RBUTTONDOWN || wParam == VK_ESCAPE) {
                CancelOverlay();
                return 0;
            }
            break;
        }

        case WM_DESTROY: {
            backgroundBmp.reset();
            return 0;
        }
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
};
