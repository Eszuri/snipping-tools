#pragma once
#include "../core/Common.hpp"
#include "../core/AppConfig.hpp"
#include "../utils/FileHelper.hpp"
#include <functional>

class SettingsWindow {
private:
    HWND hwnd = NULL;
    HWND hwndParent = NULL;
    HWND hwndEdit = NULL;
    HFONT hFontEdit = NULL;
    HBRUSH hBrushEditBg = NULL;

    AppConfig* pConfig = nullptr;
    std::function<void()> onConfigUpdated;

    // Interactive button rects
    RECT btnChangeFolder = { 0, 0, 0, 0 };
    RECT btnOpenFolder = { 0, 0, 0, 0 };
    RECT btnFmtPng = { 0, 0, 0, 0 };
    RECT btnFmtJpg = { 0, 0, 0, 0 };
    RECT btnFmtBmp = { 0, 0, 0, 0 };
    RECT btnNameTimestamp = { 0, 0, 0, 0 };
    RECT btnNameStatic = { 0, 0, 0, 0 };
    RECT btnToggleAutoCopy = { 0, 0, 0, 0 };
    RECT btnToggleMagnifier = { 0, 0, 0, 0 };
    RECT btnToggleOpenExplorer = { 0, 0, 0, 0 };
    RECT btnClose = { 0, 0, 0, 0 };

    int hoveredBtn = -1;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        SettingsWindow* self = (SettingsWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            self = (SettingsWindow*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        }

        if (self) {
            return self->HandleMessage(hwnd, msg, wParam, lParam);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void NotifyChange() {
        if (pConfig) {
            pConfig->SaveToRegistry();
        }
        if (onConfigUpdated) {
            onConfigUpdated();
        }
        InvalidateRect(hwnd, NULL, FALSE);
    }

public:
    SettingsWindow() = default;

    ~SettingsWindow() {
        if (hFontEdit) DeleteObject(hFontEdit);
        if (hBrushEditBg) DeleteObject(hBrushEditBg);
    }

    void Show(HWND parent, AppConfig& config, std::function<void()> onUpdated) {
        hwndParent = parent;
        pConfig = &config;
        onConfigUpdated = onUpdated;

        if (hwnd && IsWindow(hwnd)) {
            SetForegroundWindow(hwnd);
            return;
        }

        static bool registered = false;
        static const WCHAR* className = L"NativeSnippingToolSettingsWindow";

        if (!registered) {
            WNDCLASSEXW wc = { 0 };
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = SettingsWindow::WndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = className;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = NULL;
            wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
            wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(101), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
            RegisterClassExW(&wc);
            registered = true;
        }

        int winW = 590;
        int winH = 570;

        RECT parentRc;
        GetWindowRect(parent, &parentRc);
        int posX = parentRc.left + ((parentRc.right - parentRc.left) - winW) / 2;
        int posY = parentRc.top + ((parentRc.bottom - parentRc.top) - winH) / 2;

        if (posX < 0) posX = 100;
        if (posY < 0) posY = 100;

        if (!hBrushEditBg) {
            hBrushEditBg = CreateSolidBrush(RGB(32, 34, 40));
        }

        // Disable parent window to make Settings modal
        if (hwndParent && IsWindow(hwndParent)) {
            EnableWindow(hwndParent, FALSE);
        }

        hwnd = CreateWindowExW(
            WS_EX_DLGMODALFRAME,
            className,
            L"Pengaturan - Snipping Tools",
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
            posX, posY, winW, winH,
            parent, NULL, GetModuleHandle(NULL), this
        );

        // Create Native EDIT control for static filename input
        if (hwnd) {
            hFontEdit = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            hwndEdit = CreateWindowExW(
                0, L"EDIT",
                config.staticFilename.c_str(),
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                190, 276, 220, 26,
                hwnd, (HMENU)1001, GetModuleHandle(NULL), NULL
            );

            SendMessageW(hwndEdit, WM_SETFONT, (WPARAM)hFontEdit, TRUE);
            EnableWindow(hwndEdit, config.namingMode == NamingMode::Static);
        }
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(240, 245, 250));
            SetBkColor(hdcStatic, RGB(32, 34, 40));
            return (LRESULT)hBrushEditBg;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == 1001 && HIWORD(wParam) == EN_CHANGE) {
                if (hwndEdit && pConfig) {
                    WCHAR buf[128] = { 0 };
                    GetWindowTextW(hwndEdit, buf, 128);
                    pConfig->staticFilename = buf;
                    pConfig->SaveToRegistry();
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                return 0;
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT clientRc;
            GetClientRect(hWnd, &clientRc);
            int width = clientRc.right - clientRc.left;
            int height = clientRc.bottom - clientRc.top;

            // Double Buffering
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hBmpMem = CreateCompatibleBitmap(hdc, width, height);
            HGDIOBJ hOld = SelectObject(hdcMem, hBmpMem);

            Gdiplus::Graphics g(hdcMem);
            g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

            // 1. Dark Fluent Background
            Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 24, 25, 28));
            g.FillRectangle(&bgBrush, 0, 0, width, height);

            Gdiplus::Font fontHeader(L"Segoe UI", 14.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::Font fontSection(L"Segoe UI", 12.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::Font fontNormal(L"Segoe UI", 11.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::Font fontBold(L"Segoe UI", 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::Font fontSmall(L"Segoe UI", 10.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

            Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
            Gdiplus::SolidBrush grayBrush(Gdiplus::Color(255, 160, 165, 175));
            Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 220, 225, 230));
            Gdiplus::Pen borderPen(Gdiplus::Color(255, 52, 55, 62), 1.0f);

            Gdiplus::StringFormat sfLeft;
            sfLeft.SetAlignment(Gdiplus::StringAlignmentNear);
            sfLeft.SetLineAlignment(Gdiplus::StringAlignmentCenter);

            Gdiplus::StringFormat sfCenter;
            sfCenter.SetAlignment(Gdiplus::StringAlignmentCenter);
            sfCenter.SetLineAlignment(Gdiplus::StringAlignmentCenter);

            // Title Header
            g.DrawString(L"Pengaturan Aplikasi", -1, &fontHeader, Gdiplus::PointF(24.0f, 16.0f), &whiteBrush);
            g.DrawLine(&borderPen, 24, 44, width - 24, 44);

            // ================= SECTION 1: PENYIMPANAN =================
            int curY = 54;
            g.DrawString(L"1. Lokasi & Format Penyimpanan", -1, &fontSection, Gdiplus::PointF(24.0f, (float)curY), &whiteBrush);

            curY += 22;
            g.DrawString(L"Folder Penyimpanan Default:", -1, &fontNormal, Gdiplus::PointF(24.0f, (float)curY), &grayBrush);

            // Path Box
            curY += 18;
            int boxW = width - 48;
            int boxH = 26;
            Gdiplus::SolidBrush boxBg(Gdiplus::Color(255, 32, 34, 40));
            g.FillRectangle(&boxBg, 24, curY, boxW, boxH);
            g.DrawRectangle(&borderPen, 24, curY, boxW, boxH);

            std::wstring effDir = pConfig ? pConfig->GetEffectiveSaveDir() : L"";
            Gdiplus::StringFormat sfPath;
            sfPath.SetTrimming(Gdiplus::StringTrimmingEllipsisPath);
            sfPath.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
            sfPath.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::RectF pathRect(32.0f, (float)curY, (float)(boxW - 16), (float)boxH);
            g.DrawString(effDir.c_str(), -1, &fontNormal, pathRect, &sfPath, &textBrush);

            // Folder Buttons: "Ubah Folder..." & "Buka di Explorer"
            curY += 32;
            btnChangeFolder = { 24, (LONG)curY, 174, (LONG)(curY + 28) };
            btnOpenFolder = { 184, (LONG)curY, 344, (LONG)(curY + 28) };

            Gdiplus::SolidBrush btnChangeBg(hoveredBtn == 1 ? Gdiplus::Color(255, 24, 140, 240) : Gdiplus::Color(255, 0, 120, 215));
            g.FillRectangle(&btnChangeBg, 24, curY, 150, 28);
            Gdiplus::RectF btn1Rect(24.0f, (float)curY, 150.0f, 28.0f);
            g.DrawString(L"Ubah Folder...", -1, &fontBold, btn1Rect, &sfCenter, &whiteBrush);

            Gdiplus::SolidBrush btnOpenBg(hoveredBtn == 2 ? Gdiplus::Color(255, 52, 56, 64) : Gdiplus::Color(255, 40, 42, 48));
            g.FillRectangle(&btnOpenBg, 184, curY, 160, 28);
            g.DrawRectangle(&borderPen, 184, curY, 160, 28);
            Gdiplus::RectF btn2Rect(184.0f, (float)curY, 160.0f, 28.0f);
            g.DrawString(L"Buka di Explorer", -1, &fontBold, btn2Rect, &sfCenter, &textBrush);

            // Format Selection Pills (PNG / JPG / BMP)
            curY += 36;
            g.DrawString(L"Format Gambar:", -1, &fontNormal, Gdiplus::PointF(24.0f, (float)(curY + 4)), &grayBrush);

            btnFmtPng = { 130, (LONG)curY, 190, (LONG)(curY + 26) };
            btnFmtJpg = { 198, (LONG)curY, 258, (LONG)(curY + 26) };
            btnFmtBmp = { 266, (LONG)curY, 326, (LONG)(curY + 26) };

            auto drawFmtPill = [&](const RECT& rc, const WCHAR* label, bool isSelected, int btnIdx) {
                int pw = rc.right - rc.left;
                int ph = rc.bottom - rc.top;
                Gdiplus::SolidBrush pbg(isSelected ? Gdiplus::Color(255, 0, 120, 215) : (hoveredBtn == btnIdx ? Gdiplus::Color(255, 52, 56, 64) : Gdiplus::Color(255, 38, 40, 46)));
                g.FillRectangle(&pbg, (INT)rc.left, (INT)rc.top, pw, ph);
                g.DrawRectangle(&borderPen, (INT)rc.left, (INT)rc.top, pw, ph);
                Gdiplus::RectF prect((float)rc.left, (float)rc.top, (float)pw, (float)ph);
                g.DrawString(label, -1, &fontBold, prect, &sfCenter, isSelected ? &whiteBrush : &textBrush);
            };

            drawFmtPill(btnFmtPng, L"PNG", pConfig && pConfig->defaultSaveFormat == L"png", 3);
            drawFmtPill(btnFmtJpg, L"JPG", pConfig && pConfig->defaultSaveFormat == L"jpg", 4);
            drawFmtPill(btnFmtBmp, L"BMP", pConfig && pConfig->defaultSaveFormat == L"bmp", 5);

            // ================= SECTION 2: METODE PENAMAAN FILE =================
            curY += 38;
            g.DrawLine(&borderPen, 24, curY, width - 24, curY);
            curY += 10;

            g.DrawString(L"2. Metode Penamaan Berkas (Simpan & Simpan Sebagai)", -1, &fontSection, Gdiplus::PointF(24.0f, (float)curY), &whiteBrush);

            bool isTimestamp = pConfig && pConfig->namingMode == NamingMode::Timestamp;
            bool isStatic = pConfig && pConfig->namingMode == NamingMode::Static;

            // Radio 1: Timestamp
            curY += 22;
            btnNameTimestamp = { 24, (LONG)curY, (LONG)(width - 24), (LONG)(curY + 22) };
            Gdiplus::Pen radioPen(isTimestamp ? Gdiplus::Color(255, 0, 140, 255) : Gdiplus::Color(255, 80, 85, 95), 1.5f);
            g.DrawEllipse(&radioPen, 24, curY + 2, 16, 16);
            if (isTimestamp) {
                Gdiplus::SolidBrush dotBrush(Gdiplus::Color(255, 0, 140, 255));
                g.FillEllipse(&dotBrush, 28, curY + 6, 8, 8);
            }
            g.DrawString(L"Waktu Otomatis (contoh: Cuplikan_20260826_130852.png)", -1, &fontNormal, Gdiplus::PointF(48.0f, (float)(curY + 2)), &textBrush);

            // Radio 2: Nama Statis
            curY += 26;
            btnNameStatic = { 24, (LONG)curY, (LONG)(width - 24), (LONG)(curY + 22) };
            Gdiplus::Pen radioPen2(isStatic ? Gdiplus::Color(255, 0, 140, 255) : Gdiplus::Color(255, 80, 85, 95), 1.5f);
            g.DrawEllipse(&radioPen2, 24, curY + 2, 16, 16);
            if (isStatic) {
                Gdiplus::SolidBrush dotBrush(Gdiplus::Color(255, 0, 140, 255));
                g.FillEllipse(&dotBrush, 28, curY + 6, 8, 8);
            }
            g.DrawString(L"Nama Tetap (tidak berubah-ubah)", -1, &fontNormal, Gdiplus::PointF(48.0f, (float)(curY + 2)), &textBrush);

            // Static Name Input Row
            curY += 24;
            g.DrawString(L"Nama Berkas Default:", -1, &fontSmall, Gdiplus::PointF(48.0f, (float)(curY + 4)), &grayBrush);

            std::wstring previewExt = L"." + (pConfig ? pConfig->defaultSaveFormat : L"png");
            g.DrawString(previewExt.c_str(), -1, &fontBold, Gdiplus::PointF(418.0f, (float)(curY + 4)), &whiteBrush);

            // ================= SECTION 3: PERILAKU APLIKASI =================
            curY += 38;
            g.DrawLine(&borderPen, 24, curY, width - 24, curY);
            curY += 10;

            g.DrawString(L"3. Perilaku Aplikasi", -1, &fontSection, Gdiplus::PointF(24.0f, (float)curY), &whiteBrush);

            // Checkbox: Auto-Copy
            curY += 22;
            btnToggleAutoCopy = { 24, (LONG)curY, (LONG)(width - 24), (LONG)(curY + 22) };
            bool isAutoCopy = pConfig && pConfig->autoCopyOnCapture;
            Gdiplus::SolidBrush checkBrush(isAutoCopy ? Gdiplus::Color(255, 0, 120, 215) : Gdiplus::Color(255, 50, 54, 60));
            g.FillRectangle(&checkBrush, 24, curY + 2, 16, 16);
            g.DrawRectangle(&borderPen, 24, curY + 2, 16, 16);
            if (isAutoCopy) {
                Gdiplus::Pen checkPen(Gdiplus::Color(255, 255, 255, 255), 2.0f);
                g.DrawLine(&checkPen, 27, curY + 10, 31, curY + 14);
                g.DrawLine(&checkPen, 31, curY + 14, 37, curY + 5);
            }
            g.DrawString(L"Otomatis salin gambar ke Papan Klip setelah menyimpan gambar", -1, &fontNormal, Gdiplus::PointF(48.0f, (float)(curY + 2)), &textBrush);

            // Checkbox: Magnifier
            curY += 26;
            btnToggleMagnifier = { 24, (LONG)curY, (LONG)(width - 24), (LONG)(curY + 22) };
            bool isMagnifier = pConfig && pConfig->showMagnifier;
            Gdiplus::SolidBrush magBrush(isMagnifier ? Gdiplus::Color(255, 0, 120, 215) : Gdiplus::Color(255, 50, 54, 60));
            g.FillRectangle(&magBrush, 24, curY + 2, 16, 16);
            g.DrawRectangle(&borderPen, 24, curY + 2, 16, 16);
            if (isMagnifier) {
                Gdiplus::Pen checkPen(Gdiplus::Color(255, 255, 255, 255), 2.0f);
                g.DrawLine(&checkPen, 27, curY + 10, 31, curY + 14);
                g.DrawLine(&checkPen, 31, curY + 14, 37, curY + 5);
            }
            g.DrawString(L"Tampilkan kaca pembesar saat memilih area cuplikan layar", -1, &fontNormal, Gdiplus::PointF(48.0f, (float)(curY + 2)), &textBrush);

            // Checkbox: Open Explorer after save
            curY += 26;
            btnToggleOpenExplorer = { 24, (LONG)curY, (LONG)(width - 24), (LONG)(curY + 22) };
            bool isOpenExp = pConfig && pConfig->openExplorerAfterSave;
            Gdiplus::SolidBrush expBrush(isOpenExp ? Gdiplus::Color(255, 0, 120, 215) : Gdiplus::Color(255, 50, 54, 60));
            g.FillRectangle(&expBrush, 24, curY + 2, 16, 16);
            g.DrawRectangle(&borderPen, 24, curY + 2, 16, 16);
            if (isOpenExp) {
                Gdiplus::Pen checkPen(Gdiplus::Color(255, 255, 255, 255), 2.0f);
                g.DrawLine(&checkPen, 27, curY + 10, 31, curY + 14);
                g.DrawLine(&checkPen, 31, curY + 14, 37, curY + 5);
            }
            g.DrawString(L"Buka File Explorer setelah menyimpan (Simpan / Simpan Sebagai)", -1, &fontNormal, Gdiplus::PointF(48.0f, (float)(curY + 2)), &textBrush);

            // ================= BOTTOM BAR =================
            int bottomY = height - 52;
            g.DrawLine(&borderPen, 0, bottomY, width, bottomY);

            btnClose = { (LONG)(width - 124), (LONG)(bottomY + 10), (LONG)(width - 24), (LONG)(bottomY + 42) };
            Gdiplus::SolidBrush btnCloseBg(hoveredBtn == 8 ? Gdiplus::Color(255, 60, 64, 72) : Gdiplus::Color(255, 45, 48, 56));
            g.FillRectangle(&btnCloseBg, width - 124, bottomY + 10, 100, 32);
            g.DrawRectangle(&borderPen, width - 124, bottomY + 10, 100, 32);

            Gdiplus::RectF closeRect((float)(width - 124), (float)(bottomY + 10), 100.0f, 32.0f);
            g.DrawString(L"Tutup", -1, &fontBold, closeRect, &sfCenter, &whiteBrush);

            BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hOld);
            DeleteObject(hBmpMem);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            int prevHover = hoveredBtn;
            hoveredBtn = -1;

            auto inRc = [&](const RECT& rc) {
                return (x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom);
            };

            if (inRc(btnChangeFolder)) hoveredBtn = 1;
            else if (inRc(btnOpenFolder)) hoveredBtn = 2;
            else if (inRc(btnFmtPng)) hoveredBtn = 3;
            else if (inRc(btnFmtJpg)) hoveredBtn = 4;
            else if (inRc(btnFmtBmp)) hoveredBtn = 5;
            else if (inRc(btnClose)) hoveredBtn = 8;

            if (prevHover != hoveredBtn) {
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            auto inRc = [&](const RECT& rc) {
                return (x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom);
            };

            if (inRc(btnChangeFolder)) {
                std::wstring newFolder;
                if (FileHelper::PickFolder(hWnd, newFolder)) {
                    if (pConfig) {
                        pConfig->customSaveDir = newFolder;
                        NotifyChange();
                    }
                }
                return 0;
            } else if (inRc(btnOpenFolder)) {
                if (pConfig) {
                    ShellExecuteW(NULL, L"open", pConfig->GetEffectiveSaveDir().c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
                return 0;
            } else if (inRc(btnFmtPng)) {
                if (pConfig) { pConfig->defaultSaveFormat = L"png"; NotifyChange(); }
                return 0;
            } else if (inRc(btnFmtJpg)) {
                if (pConfig) { pConfig->defaultSaveFormat = L"jpg"; NotifyChange(); }
                return 0;
            } else if (inRc(btnFmtBmp)) {
                if (pConfig) { pConfig->defaultSaveFormat = L"bmp"; NotifyChange(); }
                return 0;
            } else if (inRc(btnNameTimestamp)) {
                if (pConfig) {
                    pConfig->namingMode = NamingMode::Timestamp;
                    if (hwndEdit) EnableWindow(hwndEdit, FALSE);
                    NotifyChange();
                }
                return 0;
            } else if (inRc(btnNameStatic)) {
                if (pConfig) {
                    pConfig->namingMode = NamingMode::Static;
                    if (hwndEdit) {
                        EnableWindow(hwndEdit, TRUE);
                        SetFocus(hwndEdit);
                    }
                    NotifyChange();
                }
                return 0;
            } else if (inRc(btnToggleAutoCopy)) {
                if (pConfig) { pConfig->autoCopyOnCapture = !pConfig->autoCopyOnCapture; NotifyChange(); }
                return 0;
            } else if (inRc(btnToggleMagnifier)) {
                if (pConfig) { pConfig->showMagnifier = !pConfig->showMagnifier; NotifyChange(); }
                return 0;
            } else if (inRc(btnToggleOpenExplorer)) {
                if (pConfig) { pConfig->openExplorerAfterSave = !pConfig->openExplorerAfterSave; NotifyChange(); }
                return 0;
            } else if (inRc(btnClose)) {
                DestroyWindow(hWnd);
                return 0;
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

        case WM_DESTROY: {
            if (hwndParent && IsWindow(hwndParent)) {
                EnableWindow(hwndParent, TRUE);
                SetForegroundWindow(hwndParent);
                SetActiveWindow(hwndParent);
            }
            hwnd = NULL;
            hwndEdit = NULL;
            return 0;
        }
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
};
