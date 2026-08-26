#pragma once
#include "../core/Common.hpp"
#include "../core/AppConfig.hpp"
#include "../annotation/AnnotationEngine.hpp"
#include "../annotation/Shapes.hpp"
#include "../utils/ClipboardHelper.hpp"
#include "../utils/FileHelper.hpp"
#include "OverlayWindow.hpp"
#include "SettingsWindow.hpp"

enum class MainWindowButtonId {
    None,
    NewSnip,
    ToggleDelay,
    ToggleMode,
    Tool_Pen,
    Tool_Rect,
    Tool_Circle,
    Undo,
    Redo,
    Save,
    SaveAs,
    Settings
};

struct UiButton {
    MainWindowButtonId id = MainWindowButtonId::None;
    AnnotationTool tool = AnnotationTool::None;
    std::wstring label;
    RECT bounds = { 0, 0, 0, 0 };
    bool isSeparator = false;
    bool isPrimary = false;
};

class MainWindow {
private:
    HWND hwnd = NULL;
    AppConfig config;
    OverlayWindow overlay;
    SettingsWindow settingsWindow;
    AnnotationEngine annotationEngine;

    std::unique_ptr<Gdiplus::Bitmap> capturedBmp;
    bool hasImage = false;
    std::wstring currentStatusMsg = L"";

    std::vector<UiButton> topButtons;
    int hoveredButtonIdx = -1;

    // Zoom & Pan Navigation Engine
    float userZoom = 1.0f; // 0.25x to 8.0x
    int panOffsetX = 0;
    int panOffsetY = 0;
    bool isPanning = false;
    POINT panStartPt = { 0, 0 };
    POINT panOriginalOffset = { 0, 0 };

    // Dropdown Popups State
    bool isPenDropdownOpen = false;
    bool isModeDropdownOpen = false;
    bool isDelayDropdownOpen = false;
    RECT penDropdownBounds = { 0, 0, 0, 0 };
    RECT modeDropdownBounds = { 0, 0, 0, 0 };
    RECT delayDropdownBounds = { 0, 0, 0, 0 };

    // Palette Colors
    const std::vector<COLORREF> palette = {
        RGB(235, 30, 60),   // Merah
        RGB(255, 150, 0),   // Oranye
        RGB(250, 210, 20),  // Kuning
        RGB(40, 200, 80),   // Hijau
        RGB(0, 140, 255),   // Biru
        RGB(160, 50, 240),  // Ungu
        RGB(255, 255, 255), // Putih
        RGB(30, 30, 30)     // Hitam
    };

    // Stroke width options: 5 steps (1px, 2px, 4px, 6px, 10px)
    const std::vector<int> strokeSizes = { 1, 2, 4, 6, 10 };

    // Canvas Mouse Drawing State
    bool isDrawingOnCanvas = false;
    POINT drawStartPt = { 0, 0 };
    POINT currentMouseCanvas = { 0, 0 };

    static const int TOP_BAR_HEIGHT = 48;
    static const int STATUS_BAR_HEIGHT = 28;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        MainWindow* self = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            self = (MainWindow*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        }

        if (self) {
            return self->HandleMessage(hwnd, msg, wParam, lParam);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void CloseAllDropdowns() {
        isPenDropdownOpen = false;
        isModeDropdownOpen = false;
        isDelayDropdownOpen = false;
    }

    void InitButtons() {
        topButtons.clear();

        // 1. Primary "Snip Baru" Button
        UiButton btnNew;
        btnNew.id = MainWindowButtonId::NewSnip;
        btnNew.label = L"Snip Baru";
        btnNew.isPrimary = true;
        topButtons.push_back(btnNew);

        // 2. Mode Selector Button with Dropdown Indicator
        std::wstring modeStr = (config.snipMode == SnipMode::Rectangle) ? L"Mode: Persegi ▾" :
                               (config.snipMode == SnipMode::Window) ? L"Mode: Jendela ▾" : L"Mode: Layar Penuh ▾";
        topButtons.push_back({ MainWindowButtonId::ToggleMode, AnnotationTool::None, modeStr });

        // 3. Delay Selector Button with Dropdown Indicator
        std::wstring delayStr = (config.delaySeconds == 0) ? L"Tunda: Mati ▾" :
                                (L"Tunda: " + std::to_wstring(config.delaySeconds) + L"d ▾");
        topButtons.push_back({ MainWindowButtonId::ToggleDelay, AnnotationTool::None, delayStr });

        // Separator
        UiButton sep1;
        sep1.isSeparator = true;
        topButtons.push_back(sep1);

        if (hasImage) {
            // 4. Pen Tool with Dropdown Indicator
            topButtons.push_back({ MainWindowButtonId::Tool_Pen, AnnotationTool::Pen, L"Pena" });

            // 5. Shapes: Persegi & Lingkaran
            topButtons.push_back({ MainWindowButtonId::Tool_Rect, AnnotationTool::Rectangle, L"Persegi" });
            topButtons.push_back({ MainWindowButtonId::Tool_Circle, AnnotationTool::Ellipse, L"Lingkaran" });

            // Separator
            UiButton sep2;
            sep2.isSeparator = true;
            topButtons.push_back(sep2);

            // 6. Urungkan & Kembalikan (Undo & Redo)
            topButtons.push_back({ MainWindowButtonId::Undo, AnnotationTool::None, L"Urungkan" });
            topButtons.push_back({ MainWindowButtonId::Redo, AnnotationTool::None, L"Kembalikan" });

            // Separator
            UiButton sep3;
            sep3.isSeparator = true;
            topButtons.push_back(sep3);

            // 7. Output Actions: Simpan dan Simpan Sebagai
            topButtons.push_back({ MainWindowButtonId::Save, AnnotationTool::None, L"Simpan" });
            topButtons.push_back({ MainWindowButtonId::SaveAs, AnnotationTool::None, L"Simpan Sebagai" });
        }

        // Separator before Settings
        UiButton sepSet;
        sepSet.isSeparator = true;
        topButtons.push_back(sepSet);

        // 8. Settings Button
        topButtons.push_back({ MainWindowButtonId::Settings, AnnotationTool::None, L"Pengaturan" });
    }

    void UpdateLayout() {
        InitButtons();

        RECT clientRc;
        GetClientRect(hwnd, &clientRc);
        int clientW = clientRc.right - clientRc.left;

        int curX = 12;
        int curY = 8;
        int btnH = 32;

        for (auto& btn : topButtons) {
            if (btn.isSeparator) {
                btn.bounds = { (LONG)curX, (LONG)(curY + 4), (LONG)(curX + 2), (LONG)(curY + btnH - 4) };
                curX += 10;
            } else {
                int btnW = 36;
                if (btn.isPrimary) {
                    btnW = 108;
                } else if (btn.id == MainWindowButtonId::ToggleDelay) {
                    btnW = 105;
                } else if (btn.id == MainWindowButtonId::ToggleMode) {
                    btnW = 150;
                } else if (btn.id == MainWindowButtonId::Tool_Pen) {
                    btnW = 48; // Dropdown indicator
                } else if (btn.id == MainWindowButtonId::Tool_Rect) {
                    btnW = 86;
                } else if (btn.id == MainWindowButtonId::Tool_Circle) {
                    btnW = 92;
                } else if (btn.id == MainWindowButtonId::Undo) {
                    btnW = 98;
                } else if (btn.id == MainWindowButtonId::Redo) {
                    btnW = 105; // "Kembalikan"
                } else if (btn.id == MainWindowButtonId::Save) {
                    btnW = 84;
                } else if (btn.id == MainWindowButtonId::SaveAs) {
                    btnW = 132;
                } else if (btn.id == MainWindowButtonId::Settings) {
                    btnW = 110;
                }

                btn.bounds = { (LONG)curX, (LONG)curY, (LONG)(curX + btnW), (LONG)(curY + btnH) };

                // Set dropdown positions directly below their buttons
                if (btn.id == MainWindowButtonId::ToggleMode) {
                    modeDropdownBounds = { (LONG)curX, (LONG)(TOP_BAR_HEIGHT + 2), (LONG)(curX + 175), (LONG)(TOP_BAR_HEIGHT + 104) };
                } else if (btn.id == MainWindowButtonId::ToggleDelay) {
                    delayDropdownBounds = { (LONG)curX, (LONG)(TOP_BAR_HEIGHT + 2), (LONG)(curX + 155), (LONG)(TOP_BAR_HEIGHT + 134) };
                } else if (btn.id == MainWindowButtonId::Tool_Pen) {
                    penDropdownBounds = { (LONG)curX, (LONG)(TOP_BAR_HEIGHT + 2), (LONG)(curX + 228), (LONG)(TOP_BAR_HEIGHT + 125) };
                }

                curX += btnW + 6;
            }
        }
    }

    RECT GetImageDisplayRect() {
        RECT clientRc;
        GetClientRect(hwnd, &clientRc);
        int clientW = clientRc.right - clientRc.left;
        int clientH = clientRc.bottom - clientRc.top;

        int canvasW = clientW;
        int canvasH = clientH - TOP_BAR_HEIGHT - STATUS_BAR_HEIGHT;

        if (!hasImage || !capturedBmp || canvasW <= 0 || canvasH <= 0) {
            RECT rc = { 0, 0, 0, 0 };
            return rc;
        }

        int imgW = capturedBmp->GetWidth();
        int imgH = capturedBmp->GetHeight();

        // Fit base scale
        float fitScale = 1.0f;
        if (imgW > canvasW - 40 || imgH > canvasH - 40) {
            float sx = (float)(canvasW - 40) / (float)imgW;
            float sy = (float)(canvasH - 40) / (float)imgH;
            fitScale = (std::min)(sx, sy);
        }

        float totalScale = fitScale * userZoom;
        int dispW = (int)(imgW * totalScale);
        int dispH = (int)(imgH * totalScale);

        int baseCenterX = (canvasW - dispW) / 2;
        int baseCenterY = TOP_BAR_HEIGHT + (canvasH - dispH) / 2;

        int finalX = baseCenterX + panOffsetX;
        int finalY = baseCenterY + panOffsetY;

        RECT rc = { (LONG)finalX, (LONG)finalY, (LONG)(finalX + dispW), (LONG)(finalY + dispH) };
        return rc;
    }

    POINT ScreenToImageCoords(int clientX, int clientY) {
        RECT dispRc = GetImageDisplayRect();
        int dispW = dispRc.right - dispRc.left;
        int dispH = dispRc.bottom - dispRc.top;

        if (!hasImage || !capturedBmp || dispW <= 0 || dispH <= 0) {
            return { 0, 0 };
        }

        float scaleX = (float)capturedBmp->GetWidth() / (float)dispW;
        float scaleY = (float)capturedBmp->GetHeight() / (float)dispH;

        int imgX = (int)((clientX - dispRc.left) * scaleX);
        int imgY = (int)((clientY - dispRc.top) * scaleY);

        imgX = (std::max)(0, (std::min)(imgX, (int)capturedBmp->GetWidth() - 1));
        imgY = (std::max)(0, (std::min)(imgY, (int)capturedBmp->GetHeight() - 1));

        return { (LONG)imgX, (LONG)imgY };
    }

    std::unique_ptr<Gdiplus::Bitmap> RenderFinalImage() {
        if (!hasImage || !capturedBmp) return nullptr;

        int w = capturedBmp->GetWidth();
        int h = capturedBmp->GetHeight();

        auto finalBmp = std::make_unique<Gdiplus::Bitmap>(w, h, PixelFormat32bppARGB);
        Gdiplus::Graphics g(finalBmp.get());
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        g.DrawImage(capturedBmp.get(), 0, 0, w, h);
        annotationEngine.RenderAll(g, capturedBmp.get());

        return finalBmp;
    }

    void StartNewSnip() {
        CloseAllDropdowns();
        ShowWindow(hwnd, SW_HIDE);

        overlay.StartCapture(
            config,
            [this](std::unique_ptr<Gdiplus::Bitmap> bmp) {
                // On Capture Complete: Open result in MainWindow
                capturedBmp = std::move(bmp);
                hasImage = (capturedBmp != nullptr);
                annotationEngine.Clear();
                currentStatusMsg = L"";

                // Reset zoom and pan on new snip
                userZoom = 1.0f;
                panOffsetX = 0;
                panOffsetY = 0;

                // Auto copy to clipboard on capture if enabled
                if (hasImage && config.autoCopyOnCapture) {
                    ClipboardHelper::CopyBitmapToClipboard(capturedBmp.get(), hwnd);
                }

                UpdateLayout();
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
            },
            [this]() {
                // On Cancel: Restore MainWindow
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        );
    }

    void ExecuteCopy() {
        auto finalBmp = RenderFinalImage();
        if (finalBmp) {
            ClipboardHelper::CopyBitmapToClipboard(finalBmp.get(), hwnd);
            currentStatusMsg = L"✓ Gambar berhasil disalin ke Clipboard (Ctrl+C).";
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }

    void ExecuteSave() {
        auto finalBmp = RenderFinalImage();
        if (finalBmp) {
            std::wstring savedPath;
            if (FileHelper::QuickSave(finalBmp.get(), config, savedPath)) {
                ClipboardHelper::CopyBitmapToClipboard(finalBmp.get(), hwnd);
                currentStatusMsg = L"✓ Tersimpan di: " + savedPath + L" (disalin ke Clipboard)";
                if (config.openExplorerAfterSave) {
                    ShellExecuteW(NULL, L"open", L"explorer.exe", (L"/select,\"" + savedPath + L"\"").c_str(), NULL, SW_SHOWNORMAL);
                }
            } else {
                currentStatusMsg = L"✕ Gagal menyimpan gambar.";
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }

    void ExecuteSaveAs() {
        auto finalBmp = RenderFinalImage();
        if (finalBmp) {
            std::wstring savedPath;
            if (FileHelper::PromptAndSave(finalBmp.get(), config, hwnd, savedPath)) {
                ClipboardHelper::CopyBitmapToClipboard(finalBmp.get(), hwnd);
                currentStatusMsg = L"✓ Tersimpan sebagai: " + savedPath + L" (disalin ke Clipboard)";
                if (config.openExplorerAfterSave) {
                    ShellExecuteW(NULL, L"open", L"explorer.exe", (L"/select,\"" + savedPath + L"\"").c_str(), NULL, SW_SHOWNORMAL);
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }

    void DrawVectorIcon(Gdiplus::Graphics& g, MainWindowButtonId id, const Gdiplus::RectF& rect, Gdiplus::Color color, bool isHovered, bool isSelected) {
        float cx = rect.X + rect.Width / 2.0f;
        float cy = rect.Y + rect.Height / 2.0f;

        Gdiplus::Pen pen(color, 1.8f);
        pen.SetStartCap(Gdiplus::LineCapRound);
        pen.SetEndCap(Gdiplus::LineCapRound);
        pen.SetLineJoin(Gdiplus::LineJoinRound);

        Gdiplus::SolidBrush brush(color);

        switch (id) {
        case MainWindowButtonId::Tool_Pen: {
            // Diagonal pen/pencil vector
            float px = cx - 5.0f;
            float py = cy - 5.0f;
            g.DrawLine(&pen, px - 1.0f, py + 8.0f, px + 7.0f, py);
            g.DrawLine(&pen, px + 1.0f, py + 10.0f, px + 9.0f, py + 2.0f);
            
            // Pen nib tip
            Gdiplus::PointF tip[3] = { { px - 4.0f, py + 11.0f }, { px + 1.0f, py + 10.0f }, { px - 1.0f, py + 6.0f } };
            g.FillPolygon(&brush, tip, 3);

            // Dropdown triangle ▼
            Gdiplus::PointF arr[3] = { { cx + 8.0f, cy - 2.0f }, { cx + 15.0f, cy - 2.0f }, { cx + 11.5f, cy + 3.0f } };
            g.FillPolygon(&brush, arr, 3);
            break;
        }
        case MainWindowButtonId::Tool_Rect: {
            // Centered rectangle
            g.DrawRectangle(&pen, (INT)(cx - 7.0f), (INT)(cy - 5.0f), 14, 10);
            break;
        }
        case MainWindowButtonId::Tool_Circle: {
            // Centered circle
            g.DrawEllipse(&pen, (INT)(cx - 6.0f), (INT)(cy - 6.0f), 12, 12);
            break;
        }
        case MainWindowButtonId::Undo: {
            // Modern Fluent Undo: sharp left arrowhead + smooth arching tail
            Gdiplus::PointF arrow[3] = {
                { cx - 6.0f, cy - 2.0f }, // Left Tip
                { cx - 1.5f, cy - 6.0f }, // Top Barb
                { cx - 1.5f, cy + 2.0f }  // Bottom Barb
            };
            g.FillPolygon(&brush, arrow, 3);

            Gdiplus::GraphicsPath path;
            path.AddLine(cx - 1.5f, cy - 2.0f, cx + 1.5f, cy - 2.0f);
            path.AddArc(cx - 1.5f, cy - 2.0f, 6.5f, 6.5f, -90.0f, 90.0f);
            path.AddLine(cx + 5.0f, cy + 1.25f, cx + 5.0f, cy + 4.5f);
            
            Gdiplus::Pen archPen(color, 1.8f);
            archPen.SetStartCap(Gdiplus::LineCapRound);
            archPen.SetEndCap(Gdiplus::LineCapRound);
            g.DrawPath(&archPen, &path);
            break;
        }
        case MainWindowButtonId::Redo: {
            // Modern Fluent Redo: sharp right arrowhead + smooth arching tail
            Gdiplus::PointF arrow[3] = {
                { cx + 6.0f, cy - 2.0f }, // Right Tip
                { cx + 1.5f, cy - 6.0f }, // Top Barb
                { cx + 1.5f, cy + 2.0f }  // Bottom Barb
            };
            g.FillPolygon(&brush, arrow, 3);

            Gdiplus::GraphicsPath path;
            path.AddLine(cx + 1.5f, cy - 2.0f, cx - 1.5f, cy - 2.0f);
            path.AddArc(cx - 5.0f, cy - 2.0f, 6.5f, 6.5f, -90.0f, -90.0f);
            path.AddLine(cx - 5.0f, cy + 1.25f, cx - 5.0f, cy + 4.5f);
            
            Gdiplus::Pen archPen(color, 1.8f);
            archPen.SetStartCap(Gdiplus::LineCapRound);
            archPen.SetEndCap(Gdiplus::LineCapRound);
            g.DrawPath(&archPen, &path);
            break;
        }
        case MainWindowButtonId::Settings: {
            // Crisp Modern Fluent 6-tooth Gear Icon
            Gdiplus::GraphicsPath gearPath;
            float outerR = 6.2f;
            float innerR = 4.4f;
            float holeR = 2.2f;
            const int numTeeth = 6;
            
            std::vector<Gdiplus::PointF> pts;
            for (int i = 0; i < numTeeth; ++i) {
                float baseAngle = i * 60.0f;
                float a1 = (baseAngle - 15.0f) * 3.14159265f / 180.0f;
                float a2 = (baseAngle - 8.0f) * 3.14159265f / 180.0f;
                float a3 = (baseAngle + 8.0f) * 3.14159265f / 180.0f;
                float a4 = (baseAngle + 15.0f) * 3.14159265f / 180.0f;

                pts.push_back({ cx + innerR * cosf(a1), cy + innerR * sinf(a1) });
                pts.push_back({ cx + outerR * cosf(a2), cy + outerR * sinf(a2) });
                pts.push_back({ cx + outerR * cosf(a3), cy + outerR * sinf(a3) });
                pts.push_back({ cx + innerR * cosf(a4), cy + innerR * sinf(a4) });
            }
            gearPath.AddPolygon(pts.data(), (INT)pts.size());
            gearPath.AddEllipse(cx - holeR, cy - holeR, holeR * 2.0f, holeR * 2.0f);

            g.FillPath(&brush, &gearPath);
            break;
        }
        default:
            break;
        }
    }

    void DrawPenDropdown(Gdiplus::Graphics& g) {
        if (!isPenDropdownOpen) return;

        int x = penDropdownBounds.left;
        int y = penDropdownBounds.top;
        int w = penDropdownBounds.right - penDropdownBounds.left;
        int h = penDropdownBounds.bottom - penDropdownBounds.top;

        // Drop Shadow
        Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(100, 0, 0, 0));
        g.FillRectangle(&shadowBrush, x + 3, y + 3, w, h);

        // Background Box
        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(250, 32, 34, 40));
        g.FillRectangle(&bgBrush, x, y, w, h);

        // Border Frame
        Gdiplus::Pen borderPen(Gdiplus::Color(255, 65, 70, 80), 1.0f);
        g.DrawRectangle(&borderPen, x, y, w, h);

        // Section 1: Color Header
        Gdiplus::Font fontSmall(L"Segoe UI", 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush labelBrush(Gdiplus::Color(255, 170, 175, 185));
        g.DrawString(L"Warna Pena:", -1, &fontSmall, Gdiplus::PointF((float)(x + 10), (float)(y + 8)), &labelBrush);

        // 8 Color Swatches in 2 rows of 4
        int swatchSize = 18;
        int gap = 8;
        int startColorX = x + 12;
        int startColorY = y + 26;

        for (size_t i = 0; i < palette.size(); ++i) {
            int row = (int)i / 4;
            int col = (int)i % 4;
            int sx = startColorX + col * (swatchSize + gap + 18);
            int sy = startColorY + row * (swatchSize + gap);

            COLORREF c = palette[i];
            Gdiplus::Color gdiColor(255, GetRValue(c), GetGValue(c), GetBValue(c));
            Gdiplus::SolidBrush colorBrush(gdiColor);
            g.FillEllipse(&colorBrush, sx, sy, swatchSize, swatchSize);

            if (c == config.currentColor) {
                Gdiplus::Pen activePen(Gdiplus::Color(255, 0, 140, 255), 2.0f);
                g.DrawEllipse(&activePen, sx - 2, sy - 2, swatchSize + 4, swatchSize + 4);
            } else {
                Gdiplus::Pen swatchBorder(Gdiplus::Color(255, 80, 85, 95), 1.0f);
                g.DrawEllipse(&swatchBorder, sx, sy, swatchSize, swatchSize);
            }
        }

        // Separator
        int sepY = y + 78;
        g.DrawLine(&borderPen, x + 8, sepY, x + w - 8, sepY);

        // Section 2: Stroke Size Header & 5 Options
        g.DrawString(L"Ketebalan:", -1, &fontSmall, Gdiplus::PointF((float)(x + 10), (float)(sepY + 8)), &labelBrush);

        int sizeBtnW = 24;
        int sizeGap = 6;
        int startSizeX = x + 72;
        int startSizeY = sepY + 5;

        for (size_t i = 0; i < strokeSizes.size(); ++i) {
            int sz = strokeSizes[i];
            int bx = startSizeX + (int)i * (sizeBtnW + sizeGap);
            int by = startSizeY;

            bool isSelected = (sz == config.currentStrokeWidth);

            if (isSelected) {
                Gdiplus::SolidBrush selBg(Gdiplus::Color(255, 0, 120, 215));
                g.FillRectangle(&selBg, bx, by, sizeBtnW, 20);
            } else {
                Gdiplus::SolidBrush btnBg(Gdiplus::Color(255, 45, 48, 56));
                g.FillRectangle(&btnBg, bx, by, sizeBtnW, 20);
            }

            int dotRadius = (sz == 1) ? 1 : (sz == 2) ? 2 : (sz == 4) ? 3 : (sz == 6) ? 5 : 7;
            Gdiplus::SolidBrush dotBrush(Gdiplus::Color(255, 255, 255, 255));
            g.FillEllipse(&dotBrush, bx + sizeBtnW / 2 - dotRadius, by + 10 - dotRadius, dotRadius * 2, dotRadius * 2);
        }
    }

    void DrawModeDropdown(Gdiplus::Graphics& g) {
        if (!isModeDropdownOpen) return;

        int x = modeDropdownBounds.left;
        int y = modeDropdownBounds.top;
        int w = modeDropdownBounds.right - modeDropdownBounds.left;
        int h = modeDropdownBounds.bottom - modeDropdownBounds.top;

        // Shadow & Background
        Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(100, 0, 0, 0));
        g.FillRectangle(&shadowBrush, x + 3, y + 3, w, h);

        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(250, 32, 34, 40));
        g.FillRectangle(&bgBrush, x, y, w, h);

        Gdiplus::Pen borderPen(Gdiplus::Color(255, 65, 70, 80), 1.0f);
        g.DrawRectangle(&borderPen, x, y, w, h);

        const struct { SnipMode mode; const WCHAR* label; } items[] = {
            { SnipMode::Rectangle, L"▢ Persegi" },
            { SnipMode::Window, L"🗔 Jendela" },
            { SnipMode::FullScreen, L"🖥 Layar Penuh" }
        };

        Gdiplus::Font fontItem(L"Segoe UI", 11.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::Font fontItemBold(L"Segoe UI", 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::StringFormat sfLeft;
        sfLeft.SetAlignment(Gdiplus::StringAlignmentNear);
        sfLeft.SetLineAlignment(Gdiplus::StringAlignmentCenter);

        int itemH = 28;
        for (int i = 0; i < 3; ++i) {
            int iy = y + 6 + i * itemH;
            bool isSelected = (config.snipMode == items[i].mode);

            if (isSelected) {
                Gdiplus::SolidBrush selBg(Gdiplus::Color(255, 0, 120, 215));
                g.FillRectangle(&selBg, x + 4, iy, w - 8, itemH);
            }

            Gdiplus::SolidBrush textBrush(isSelected ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 220, 225, 230));
            Gdiplus::RectF itemRect((float)(x + 12), (float)iy, (float)(w - 24), (float)itemH);
            g.DrawString(items[i].label, -1, isSelected ? &fontItemBold : &fontItem, itemRect, &sfLeft, &textBrush);
        }
    }

    void DrawDelayDropdown(Gdiplus::Graphics& g) {
        if (!isDelayDropdownOpen) return;

        int x = delayDropdownBounds.left;
        int y = delayDropdownBounds.top;
        int w = delayDropdownBounds.right - delayDropdownBounds.left;
        int h = delayDropdownBounds.bottom - delayDropdownBounds.top;

        // Shadow & Background
        Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(100, 0, 0, 0));
        g.FillRectangle(&shadowBrush, x + 3, y + 3, w, h);

        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(250, 32, 34, 40));
        g.FillRectangle(&bgBrush, x, y, w, h);

        Gdiplus::Pen borderPen(Gdiplus::Color(255, 65, 70, 80), 1.0f);
        g.DrawRectangle(&borderPen, x, y, w, h);

        const struct { int seconds; const WCHAR* label; } items[] = {
            { 0, L"⏱ Tanpa Jeda" },
            { 3, L"⏱ Tunda 3 detik" },
            { 5, L"⏱ Tunda 5 detik" },
            { 10, L"⏱ Tunda 10 detik" }
        };

        Gdiplus::Font fontItem(L"Segoe UI", 11.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::Font fontItemBold(L"Segoe UI", 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::StringFormat sfLeft;
        sfLeft.SetAlignment(Gdiplus::StringAlignmentNear);
        sfLeft.SetLineAlignment(Gdiplus::StringAlignmentCenter);

        int itemH = 28;
        for (int i = 0; i < 4; ++i) {
            int iy = y + 6 + i * itemH;
            bool isSelected = (config.delaySeconds == items[i].seconds);

            if (isSelected) {
                Gdiplus::SolidBrush selBg(Gdiplus::Color(255, 0, 120, 215));
                g.FillRectangle(&selBg, x + 4, iy, w - 8, itemH);
            }

            Gdiplus::SolidBrush textBrush(isSelected ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 220, 225, 230));
            Gdiplus::RectF itemRect((float)(x + 12), (float)iy, (float)(w - 24), (float)itemH);
            g.DrawString(items[i].label, -1, isSelected ? &fontItemBold : &fontItem, itemRect, &sfLeft, &textBrush);
        }
    }

public:
    MainWindow() = default;

    bool Create() {
        static bool registered = false;
        static const WCHAR* className = L"NativeSnippingToolMainWindow";

        if (!registered) {
            WNDCLASSEXW wc = { 0 };
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
            wc.lpfnWndProc = MainWindow::WndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = className;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = NULL;
            wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
            wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(101), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
            RegisterClassExW(&wc);
            registered = true;
        }

        int winW = 1260;
        int winH = 720;
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int posX = (screenW - winW) / 2;
        int posY = (screenH - winH) / 2;

        hwnd = CreateWindowExW(
            0,
            className,
            L"Snipping Tools",
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            posX, posY, winW, winH,
            NULL, NULL, GetModuleHandle(NULL), this
        );

        if (!hwnd) return false;

        // Register App Shortcut: Ctrl+N & PrintScreen
        RegisterHotKey(hwnd, 3001, MOD_CONTROL | MOD_NOREPEAT, 'N');
        RegisterHotKey(hwnd, 3002, MOD_NOREPEAT, VK_SNAPSHOT);

        config.LoadFromRegistry();

        UpdateLayout();
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        return true;
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE: {
            UpdateLayout();
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        case WM_HOTKEY: {
            if (wParam == 3001 || wParam == 3002) {
                StartNewSnip();
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (isCtrl && hasImage) {
                short delta = GET_WHEEL_DELTA_WPARAM(wParam);
                float prevZoom = userZoom;
                if (delta > 0) {
                    userZoom *= 1.15f; // Zoom in
                } else {
                    userZoom /= 1.15f; // Zoom out
                }
                if (userZoom < 0.25f) userZoom = 0.25f;
                if (userZoom > 8.0f) userZoom = 8.0f;

                // Zoom anchored directly to the center of the canvas
                float zoomRatio = userZoom / prevZoom;
                panOffsetX = (int)(panOffsetX * zoomRatio);
                panOffsetY = (int)(panOffsetY * zoomRatio);

                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            break;
        }

        case WM_MBUTTONDOWN: {
            if (hasImage) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                SetCapture(hWnd);
                isPanning = true;
                panStartPt = { (LONG)x, (LONG)y };
                panOriginalOffset = { (LONG)panOffsetX, (LONG)panOffsetY };
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                return 0;
            }
            break;
        }

        case WM_MBUTTONUP: {
            if (isPanning) {
                isPanning = false;
                ReleaseCapture();
                InvalidateRect(hWnd, NULL, FALSE);
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

            // 1. App Background (Fluent Dark Charcoal)
            Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 24, 25, 28));
            g.FillRectangle(&bgBrush, 0, 0, width, height);

            // 2. Top Bar Background
            Gdiplus::SolidBrush topBg(Gdiplus::Color(255, 32, 33, 38));
            g.FillRectangle(&topBg, 0, 0, width, TOP_BAR_HEIGHT);

            Gdiplus::Pen borderPen(Gdiplus::Color(255, 48, 50, 56), 1.0f);
            g.DrawLine(&borderPen, 0, TOP_BAR_HEIGHT, width, TOP_BAR_HEIGHT);

            // 3. Draw Top Bar Buttons
            Gdiplus::Font fontText(L"Segoe UI", 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::Font fontTextBold(L"Segoe UI", 12.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::StringFormat sf;
            sf.SetAlignment(Gdiplus::StringAlignmentCenter);
            sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);

            Gdiplus::StringFormat sfLeft;
            sfLeft.SetAlignment(Gdiplus::StringAlignmentNear);
            sfLeft.SetLineAlignment(Gdiplus::StringAlignmentCenter);

            for (size_t i = 0; i < topButtons.size(); ++i) {
                const auto& btn = topButtons[i];

                if (btn.isSeparator) {
                    Gdiplus::Pen sepPen(Gdiplus::Color(255, 60, 64, 72), 1.0f);
                    g.DrawLine(&sepPen, (INT)btn.bounds.left, (INT)btn.bounds.top, (INT)btn.bounds.left, (INT)btn.bounds.bottom);
                    continue;
                }

                int bw = btn.bounds.right - btn.bounds.left;
                int bh = btn.bounds.bottom - btn.bounds.top;
                Gdiplus::RectF btnRect((float)btn.bounds.left, (float)btn.bounds.top, (float)bw, (float)bh);

                bool isSelectedTool = (btn.tool != AnnotationTool::None && btn.tool == config.currentTool);
                bool isHovered = ((int)i == hoveredButtonIdx);

                if (btn.isPrimary) {
                    // Fluent Primary Blue Button (Snip Baru)
                    Gdiplus::SolidBrush primBg(isHovered ? Gdiplus::Color(255, 24, 140, 240) : Gdiplus::Color(255, 0, 120, 215));
                    g.FillRectangle(&primBg, btnRect);

                    // Plus icon + text
                    Gdiplus::Pen plusPen(Gdiplus::Color(255, 255, 255, 255), 2.0f);
                    plusPen.SetStartCap(Gdiplus::LineCapRound);
                    plusPen.SetEndCap(Gdiplus::LineCapRound);
                    float pcx = btnRect.X + 16.0f;
                    float pcy = btnRect.Y + btnRect.Height / 2.0f;
                    g.DrawLine(&plusPen, pcx - 4.0f, pcy, pcx + 4.0f, pcy);
                    g.DrawLine(&plusPen, pcx, pcy - 4.0f, pcx, pcy + 4.0f);

                    Gdiplus::SolidBrush whiteText(Gdiplus::Color(255, 255, 255, 255));
                    Gdiplus::RectF textRect(btnRect.X + 26.0f, btnRect.Y, btnRect.Width - 26.0f, btnRect.Height);
                    g.DrawString(btn.label.c_str(), -1, &fontTextBold, textRect, &sfLeft, &whiteText);
                } else if (btn.id == MainWindowButtonId::ToggleDelay || btn.id == MainWindowButtonId::ToggleMode) {
                    // Dropdown/Toggle pill
                    bool isOpen = (btn.id == MainWindowButtonId::ToggleMode && isModeDropdownOpen) ||
                                  (btn.id == MainWindowButtonId::ToggleDelay && isDelayDropdownOpen);

                    Gdiplus::SolidBrush pillBg(isHovered || isOpen ? Gdiplus::Color(255, 48, 52, 60) : Gdiplus::Color(255, 40, 42, 48));
                    g.FillRectangle(&pillBg, btnRect);

                    Gdiplus::Pen pillBorder(isOpen ? Gdiplus::Color(255, 0, 140, 255) : Gdiplus::Color(255, 60, 64, 72), 1.0f);
                    g.DrawRectangle(&pillBorder, (INT)btn.bounds.left, (INT)btn.bounds.top, (INT)bw, (INT)bh);

                    // Mini-icon
                    float micx = btnRect.X + 14.0f;
                    float micy = btnRect.Y + btnRect.Height / 2.0f;
                    Gdiplus::Pen miniPen(Gdiplus::Color(255, 170, 175, 185), 1.4f);

                    if (btn.id == MainWindowButtonId::ToggleMode) {
                        g.DrawRectangle(&miniPen, (INT)(micx - 5), (INT)(micy - 4), 10, 8);
                    } else if (btn.id == MainWindowButtonId::ToggleDelay) {
                        g.DrawEllipse(&miniPen, (INT)(micx - 5), (INT)(micy - 5), 10, 10);
                        g.DrawLine(&miniPen, micx, micy, micx + 3, micy);
                        g.DrawLine(&miniPen, micx, micy, micx, micy - 3);
                    }

                    Gdiplus::SolidBrush textColor(Gdiplus::Color(255, 220, 225, 230));
                    Gdiplus::RectF textRect(btnRect.X + 26.0f, btnRect.Y, btnRect.Width - 26.0f, btnRect.Height);
                    g.DrawString(btn.label.c_str(), -1, &fontText, textRect, &sfLeft, &textColor);
                } else if (btn.id == MainWindowButtonId::Undo || btn.id == MainWindowButtonId::Redo) {
                    // Urungkan / Kembalikan with Icon and Text
                    Gdiplus::SolidBrush actBg(isHovered ? Gdiplus::Color(255, 48, 52, 60) : Gdiplus::Color(255, 38, 40, 46));
                    g.FillRectangle(&actBg, btnRect);

                    Gdiplus::Pen actBorder(Gdiplus::Color(255, 58, 62, 70), 1.0f);
                    g.DrawRectangle(&actBorder, (INT)btn.bounds.left, (INT)btn.bounds.top, (INT)bw, (INT)bh);

                    Gdiplus::Color iconColor = isHovered ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 220, 225, 230);
                    Gdiplus::RectF iconRect(btnRect.X + 8.0f, btnRect.Y, 16.0f, btnRect.Height);
                    DrawVectorIcon(g, btn.id, iconRect, iconColor, isHovered, false);

                    Gdiplus::SolidBrush textBrush(iconColor);
                    Gdiplus::RectF textRect(btnRect.X + 28.0f, btnRect.Y, btnRect.Width - 28.0f, btnRect.Height);
                    g.DrawString(btn.label.c_str(), -1, &fontTextBold, textRect, &sfLeft, &textBrush);
                } else if (btn.id == MainWindowButtonId::Save || btn.id == MainWindowButtonId::SaveAs) {
                    // Action Buttons (Simpan / Simpan Sebagai)
                    Gdiplus::SolidBrush actBg(isHovered ? Gdiplus::Color(255, 48, 52, 60) : Gdiplus::Color(255, 38, 40, 46));
                    g.FillRectangle(&actBg, btnRect);

                    Gdiplus::Pen actBorder(Gdiplus::Color(255, 58, 62, 70), 1.0f);
                    g.DrawRectangle(&actBorder, (INT)btn.bounds.left, (INT)btn.bounds.top, (INT)bw, (INT)bh);

                    // Mini Floppy disk vector icon
                    float ficx = btnRect.X + 16.0f;
                    float ficy = btnRect.Y + btnRect.Height / 2.0f;
                    Gdiplus::Color diskColor = (btn.id == MainWindowButtonId::Save) ? Gdiplus::Color(255, 60, 170, 255) : Gdiplus::Color(255, 220, 225, 230);
                    Gdiplus::Pen diskPen(diskColor, 1.4f);
                    g.DrawRectangle(&diskPen, (INT)(ficx - 6), (INT)(ficy - 6), 12, 12);
                    g.DrawRectangle(&diskPen, (INT)(ficx - 3), (INT)(ficy - 6), 6, 4);

                    Gdiplus::SolidBrush textBrush(diskColor);
                    Gdiplus::RectF textRect(btnRect.X + 28.0f, btnRect.Y, btnRect.Width - 28.0f, btnRect.Height);
                    g.DrawString(btn.label.c_str(), -1, &fontTextBold, textRect, &sfLeft, &textBrush);
                } else if (btn.id == MainWindowButtonId::Settings) {
                    // Pengaturan Button with Crisp Gear Icon & Text
                    Gdiplus::SolidBrush actBg(isHovered ? Gdiplus::Color(255, 48, 52, 60) : Gdiplus::Color(255, 38, 40, 46));
                    g.FillRectangle(&actBg, btnRect);

                    Gdiplus::Pen actBorder(Gdiplus::Color(255, 58, 62, 70), 1.0f);
                    g.DrawRectangle(&actBorder, (INT)btn.bounds.left, (INT)btn.bounds.top, (INT)bw, (INT)bh);

                    Gdiplus::Color iconColor = isHovered ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 220, 225, 230);
                    Gdiplus::RectF iconRect(btnRect.X + 8.0f, btnRect.Y, 16.0f, btnRect.Height);
                    DrawVectorIcon(g, btn.id, iconRect, iconColor, isHovered, false);

                    Gdiplus::SolidBrush textBrush(iconColor);
                    Gdiplus::RectF textRect(btnRect.X + 28.0f, btnRect.Y, btnRect.Width - 28.0f, btnRect.Height);
                    g.DrawString(btn.label.c_str(), -1, &fontTextBold, textRect, &sfLeft, &textBrush);
                } else if (btn.id == MainWindowButtonId::Tool_Rect || btn.id == MainWindowButtonId::Tool_Circle) {
                    // Shapes (Persegi, Lingkaran) with Icon and Text
                    Gdiplus::SolidBrush actBg(isSelectedTool ? Gdiplus::Color(255, 0, 120, 215) : (isHovered ? Gdiplus::Color(255, 50, 54, 62) : Gdiplus::Color(255, 38, 40, 46)));
                    g.FillRectangle(&actBg, btnRect);

                    Gdiplus::Pen actBorder(Gdiplus::Color(255, 58, 62, 70), 1.0f);
                    g.DrawRectangle(&actBorder, (INT)btn.bounds.left, (INT)btn.bounds.top, (INT)bw, (INT)bh);

                    Gdiplus::Color iconColor = isSelectedTool ? Gdiplus::Color(255, 255, 255, 255) : (isHovered ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 220, 225, 230));
                    Gdiplus::RectF iconRect(btnRect.X + 8.0f, btnRect.Y, 16.0f, btnRect.Height);
                    DrawVectorIcon(g, btn.id, iconRect, iconColor, isHovered, isSelectedTool);

                    Gdiplus::SolidBrush textBrush(iconColor);
                    Gdiplus::RectF textRect(btnRect.X + 28.0f, btnRect.Y, btnRect.Width - 28.0f, btnRect.Height);
                    g.DrawString(btn.label.c_str(), -1, &fontTextBold, textRect, &sfLeft, &textBrush);
                } else {
                    // Pen Tool Button
                    if (isSelectedTool) {
                        Gdiplus::SolidBrush selBg(Gdiplus::Color(255, 0, 120, 215));
                        g.FillRectangle(&selBg, btnRect);
                    } else if (isHovered) {
                        Gdiplus::SolidBrush hoverBg(Gdiplus::Color(255, 50, 54, 62));
                        g.FillRectangle(&hoverBg, btnRect);
                    }

                    Gdiplus::Color iconColor = isSelectedTool ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 220, 225, 230);
                    DrawVectorIcon(g, btn.id, btnRect, iconColor, isHovered, isSelectedTool);

                    // Active color underline
                    if (btn.id == MainWindowButtonId::Tool_Pen) {
                        Gdiplus::SolidBrush colUnderline(Gdiplus::Color(255, GetRValue(config.currentColor), GetGValue(config.currentColor), GetBValue(config.currentColor)));
                        g.FillRectangle(&colUnderline, (INT)(btn.bounds.left + 6), (INT)(btn.bounds.bottom - 4), (INT)(bw - 12), 3);
                    }
                }
            }

            // 4. Center Content View
            if (!hasImage || !capturedBmp) {
                // ================= HOME STATE (WELCOME PAGE) =================
                int centerY = (height - TOP_BAR_HEIGHT - STATUS_BAR_HEIGHT) / 2 + TOP_BAR_HEIGHT;

                // Beautiful Modern Solid App Icon Badge
                float icW = 68.0f;
                float icH = 68.0f;
                float icX = (float)width / 2.0f - icW / 2.0f;
                float icY = (float)(centerY - 110);

                // Subtle shadow
                Gdiplus::GraphicsPath shadowPath;
                float sRad = 18.0f;
                float sx = icX + 2.0f, sy = icY + 3.0f;
                shadowPath.AddArc(sx, sy, sRad * 2, sRad * 2, 180, 90);
                shadowPath.AddArc(sx + icW - sRad * 2, sy, sRad * 2, sRad * 2, 270, 90);
                shadowPath.AddArc(sx + icW - sRad * 2, sy + icH - sRad * 2, sRad * 2, sRad * 2, 0, 90);
                shadowPath.AddArc(sx, sy + icH - sRad * 2, sRad * 2, sRad * 2, 90, 90);
                shadowPath.CloseFigure();
                Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(80, 0, 0, 0));
                g.FillPath(&shadowBrush, &shadowPath);

                // Rounded Blue Card (#007AFF)
                Gdiplus::GraphicsPath bgPath;
                bgPath.AddArc(icX, icY, sRad * 2, sRad * 2, 180, 90);
                bgPath.AddArc(icX + icW - sRad * 2, icY, sRad * 2, sRad * 2, 270, 90);
                bgPath.AddArc(icX + icW - sRad * 2, icY + icH - sRad * 2, sRad * 2, sRad * 2, 0, 90);
                bgPath.AddArc(icX, icY + icH - sRad * 2, sRad * 2, sRad * 2, 90, 90);
                bgPath.CloseFigure();

                Gdiplus::SolidBrush iconBadgeBg(Gdiplus::Color(255, 0, 122, 255));
                g.FillPath(&iconBadgeBg, &bgPath);

                // Inner Solid White Camera Symbol
                float camCX = (float)width / 2.0f;
                float camCY = icY + icH / 2.0f;

                // Viewfinder Top Bump
                Gdiplus::GraphicsPath bumpPath;
                float bx = camCX - 6.0f, by = camCY - 16.0f, bw = 12.0f, bh = 5.0f, br = 2.0f;
                bumpPath.AddArc(bx, by, br * 2, br * 2, 180, 90);
                bumpPath.AddArc(bx + bw - br * 2, by, br * 2, br * 2, 270, 90);
                bumpPath.AddArc(bx + bw - br * 2, by + bh - br * 2, br * 2, br * 2, 0, 90);
                bumpPath.AddArc(bx, by + bh - br * 2, br * 2, br * 2, 90, 90);
                bumpPath.CloseFigure();
                Gdiplus::SolidBrush whiteIconBrush(Gdiplus::Color(255, 255, 255, 255));
                g.FillPath(&whiteIconBrush, &bumpPath);

                // Camera Body
                Gdiplus::GraphicsPath bodyPath;
                float cx0 = camCX - 19.0f, cy0 = camCY - 11.0f, cw = 38.0f, ch = 27.0f, cr = 6.0f;
                bodyPath.AddArc(cx0, cy0, cr * 2, cr * 2, 180, 90);
                bodyPath.AddArc(cx0 + cw - cr * 2, cy0, cr * 2, cr * 2, 270, 90);
                bodyPath.AddArc(cx0 + cw - cr * 2, cy0 + ch - cr * 2, cr * 2, cr * 2, 0, 90);
                bodyPath.AddArc(cx0, cy0 + ch - cr * 2, cr * 2, cr * 2, 90, 90);
                bodyPath.CloseFigure();
                g.FillPath(&whiteIconBrush, &bodyPath);

                // Blue Flash dot
                Gdiplus::SolidBrush blueDot(Gdiplus::Color(255, 0, 122, 255));
                g.FillEllipse(&blueDot, (INT)(camCX + 9.5f), (INT)(cy0 + 4.0f), 4, 4);

                // Outer Lens Circle (Blue)
                float lensR = 8.5f;
                float lensCY = cy0 + 13.5f;
                g.FillEllipse(&blueDot, (INT)(camCX - lensR), (INT)(lensCY - lensR), (INT)(lensR * 2.0f), (INT)(lensR * 2.0f));

                // Inner Lens Center (White)
                float inR = 4.0f;
                g.FillEllipse(&whiteIconBrush, (INT)(camCX - inR), (INT)(lensCY - inR), (INT)(inR * 2.0f), (INT)(inR * 2.0f));

                // Title
                Gdiplus::Font fontTitle(L"Segoe UI", 20.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
                Gdiplus::SolidBrush titleBrush(Gdiplus::Color(255, 255, 255, 255));
                Gdiplus::RectF titleRect(0.0f, (float)(centerY - 40), (float)width, 30.0f);
                g.DrawString(L"Snipping Tools", -1, &fontTitle, titleRect, &sf, &titleBrush);

                // Subtitle
                Gdiplus::Font fontSub(L"Segoe UI", 13.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
                Gdiplus::SolidBrush subBrush(Gdiplus::Color(255, 160, 165, 175));
                Gdiplus::RectF subRect(0.0f, (float)(centerY - 6), (float)width, 24.0f);
                g.DrawString(L"Klik tombol 'Snip Baru' atau tekan pintasan Ctrl+N / PrtScn untuk mulai menangkap layar.", -1, &fontSub, subRect, &sf, &subBrush);

                // Keyboard Shortcut Info Card
                int cardW = 440;
                int cardH = 92;
                int cardX = (width - cardW) / 2;
                int cardY = centerY + 30;

                Gdiplus::SolidBrush cardBg(Gdiplus::Color(255, 32, 34, 40));
                g.FillRectangle(&cardBg, cardX, cardY, cardW, cardH);

                Gdiplus::Pen cardBorder(Gdiplus::Color(255, 48, 52, 60), 1.0f);
                g.DrawRectangle(&cardBorder, cardX, cardY, cardW, cardH);

                Gdiplus::Font fontKey(L"Segoe UI", 11.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
                Gdiplus::SolidBrush keyBrush(Gdiplus::Color(255, 200, 205, 215));

                g.DrawString(L"• Ctrl + N / PrtScn        : Ambil cuplikan (Snip) baru", -1, &fontKey, Gdiplus::PointF((float)(cardX + 16), (float)(cardY + 12)), &keyBrush);
                g.DrawString(L"• Ctrl + Scroll Mouse     : Perbesar / Perkecil (Zoom) gambar", -1, &fontKey, Gdiplus::PointF((float)(cardX + 16), (float)(cardY + 36)), &keyBrush);
                g.DrawString(L"• Ctrl + Drag / MMB Drag  : Geser posisi gambar saat diperbesar", -1, &fontKey, Gdiplus::PointF((float)(cardX + 16), (float)(cardY + 60)), &keyBrush);

            } else {
                // ================= PREVIEW / EDITOR STATE (ZOOM & PAN ENABLED) =================
                RECT dispRc = GetImageDisplayRect();
                int dispW = dispRc.right - dispRc.left;
                int dispH = dispRc.bottom - dispRc.top;

                int canvasClipY = TOP_BAR_HEIGHT;
                int canvasClipH = height - TOP_BAR_HEIGHT - STATUS_BAR_HEIGHT;
                g.SetClip(Gdiplus::Rect(0, canvasClipY, width, canvasClipH));

                if (dispW > 0 && dispH > 0) {
                    // Image Shadow
                    Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(120, 0, 0, 0));
                    g.FillRectangle(&shadowBrush, dispRc.left + 4, dispRc.top + 4, dispW, dispH);

                    // Image Border Frame
                    Gdiplus::Pen framePen(Gdiplus::Color(255, 60, 65, 75), 1.0f);
                    g.DrawRectangle(&framePen, dispRc.left - 1, dispRc.top - 1, dispW + 2, dispH + 2);

                    // Draw Captured Image with smooth interpolation
                    g.SetInterpolationMode(userZoom > 2.0f ? Gdiplus::InterpolationModeNearestNeighbor : Gdiplus::InterpolationModeHighQualityBilinear);
                    g.DrawImage(capturedBmp.get(), Gdiplus::Rect(dispRc.left, dispRc.top, dispW, dispH), 0, 0, capturedBmp->GetWidth(), capturedBmp->GetHeight(), Gdiplus::UnitPixel);

                    // Draw Vector Annotations on Top
                    float scaleX = (float)dispW / (float)capturedBmp->GetWidth();
                    float scaleY = (float)dispH / (float)capturedBmp->GetHeight();

                    g.TranslateTransform((Gdiplus::REAL)dispRc.left, (Gdiplus::REAL)dispRc.top);
                    g.ScaleTransform(scaleX, scaleY);

                    annotationEngine.RenderAll(g, capturedBmp.get());

                    g.ResetTransform();
                }

                g.ResetClip();
            }

            // 5. Draw Dropdowns if open
            if (isModeDropdownOpen) {
                DrawModeDropdown(g);
            }
            if (isDelayDropdownOpen) {
                DrawDelayDropdown(g);
            }
            if (isPenDropdownOpen) {
                DrawPenDropdown(g);
            }

            // 6. Bottom Status Bar with Zoom Level & Version v1.0
            int statusY = height - STATUS_BAR_HEIGHT;
            Gdiplus::SolidBrush statusBg(Gdiplus::Color(255, 28, 30, 34));
            g.FillRectangle(&statusBg, 0, statusY, width, STATUS_BAR_HEIGHT);
            g.DrawLine(&borderPen, 0, statusY, width, statusY);

            int zoomPct = (int)(userZoom * 100.0f + 0.5f);
            std::wstring statusText = hasImage && capturedBmp ?
                (!currentStatusMsg.empty() ? currentStatusMsg : (L"✓ " + std::to_wstring(capturedBmp->GetWidth()) + L" × " + std::to_wstring(capturedBmp->GetHeight()) + L" px | Perbesaran: " + std::to_wstring(zoomPct) + L"% • Ctrl+Scroll Perbesar | Ctrl+Drag Geser | Ctrl+S Simpan")) :
                L"Siap menangkap layar. Tekan 'Snip Baru' atau Ctrl+N.";

            Gdiplus::Font fontStatus(L"Segoe UI", 11.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::SolidBrush statusBrush(Gdiplus::Color(255, 160, 165, 175));
            g.DrawString(statusText.c_str(), -1, &fontStatus, Gdiplus::PointF(16.0f, (float)(statusY + 6)), &statusBrush);

            // Version v1.0 Badge in Bottom-Right Corner
            Gdiplus::Font fontVer(L"Segoe UI", 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::SolidBrush verBrush(Gdiplus::Color(255, 120, 125, 135));
            Gdiplus::StringFormat sfRight;
            sfRight.SetAlignment(Gdiplus::StringAlignmentFar);
            sfRight.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::RectF verRect((float)(width - 120), (float)statusY, 104.0f, (float)STATUS_BAR_HEIGHT);
            g.DrawString(L"v1.0", -1, &fontVer, verRect, &sfRight, &verBrush);

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
            currentMouseCanvas = { (LONG)x, (LONG)y };

            // 1. Handling Pan Drag
            if (isPanning) {
                panOffsetX = panOriginalOffset.x + (x - panStartPt.x);
                panOffsetY = panOriginalOffset.y + (y - panStartPt.y);
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }

            // Check button hover
            int prevHover = hoveredButtonIdx;
            hoveredButtonIdx = -1;

            if (y <= TOP_BAR_HEIGHT) {
                for (size_t i = 0; i < topButtons.size(); ++i) {
                    const auto& btn = topButtons[i];
                    if (btn.isSeparator) continue;
                    if (x >= btn.bounds.left && x <= btn.bounds.right && y >= btn.bounds.top && y <= btn.bounds.bottom) {
                        hoveredButtonIdx = (int)i;
                        break;
                    }
                }
            }

            if (prevHover != hoveredButtonIdx) {
                InvalidateRect(hWnd, NULL, FALSE);
            }

            // 2. Canvas drawing interaction
            if (isDrawingOnCanvas && hasImage) {
                POINT imgPt = ScreenToImageCoords(x, y);
                Gdiplus::Color drawColor(255, GetRValue(config.currentColor), GetGValue(config.currentColor), GetBValue(config.currentColor));

                if (config.currentTool == AnnotationTool::Pen) {
                    AnnotationShape* active = annotationEngine.GetActiveShape();
                    if (active) {
                        PenShape* p = dynamic_cast<PenShape*>(active);
                        if (p) {
                            p->AddPoint(imgPt.x, imgPt.y);
                        }
                    }
                } else if (config.currentTool == AnnotationTool::Rectangle) {
                    annotationEngine.SetActiveShape(std::make_unique<RectShape>(drawStartPt.x, drawStartPt.y, imgPt.x, imgPt.y, drawColor, (float)config.currentStrokeWidth));
                } else if (config.currentTool == AnnotationTool::Ellipse) {
                    annotationEngine.SetActiveShape(std::make_unique<EllipseShape>(drawStartPt.x, drawStartPt.y, imgPt.x, imgPt.y, drawColor, (float)config.currentStrokeWidth));
                }
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (hasImage) {
                bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                RECT dispRc = GetImageDisplayRect();
                if (isCtrl) {
                    SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                } else if (x >= dispRc.left && x <= dispRc.right && y >= dispRc.top && y <= dispRc.bottom) {
                    SetCursor(LoadCursor(NULL, IDC_CROSS));
                } else {
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                }
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            // 1. Pan with Ctrl + Left Drag
            bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (isCtrl && hasImage) {
                SetCapture(hWnd);
                isPanning = true;
                panStartPt = { (LONG)x, (LONG)y };
                panOriginalOffset = { (LONG)panOffsetX, (LONG)panOffsetY };
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                return 0;
            }

            // 2. Check Mode Dropdown Click if Open
            if (isModeDropdownOpen) {
                if (x >= modeDropdownBounds.left && x <= modeDropdownBounds.right &&
                    y >= modeDropdownBounds.top && y <= modeDropdownBounds.bottom) {
                    
                    int dy = modeDropdownBounds.top;
                    int itemH = 28;
                    int idx = (y - (dy + 6)) / itemH;

                    if (idx == 0) config.snipMode = SnipMode::Rectangle;
                    else if (idx == 1) config.snipMode = SnipMode::Window;
                    else if (idx == 2) config.snipMode = SnipMode::FullScreen;

                    config.SaveToRegistry();
                    isModeDropdownOpen = false;
                    UpdateLayout();
                    InvalidateRect(hWnd, NULL, FALSE);
                    return 0;
                } else {
                    isModeDropdownOpen = false;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }

            // 3. Check Delay Dropdown Click if Open
            if (isDelayDropdownOpen) {
                if (x >= delayDropdownBounds.left && x <= delayDropdownBounds.right &&
                    y >= delayDropdownBounds.top && y <= delayDropdownBounds.bottom) {
                    
                    int dy = delayDropdownBounds.top;
                    int itemH = 28;
                    int idx = (y - (dy + 6)) / itemH;

                    if (idx == 0) config.delaySeconds = 0;
                    else if (idx == 1) config.delaySeconds = 3;
                    else if (idx == 2) config.delaySeconds = 5;
                    else if (idx == 3) config.delaySeconds = 10;

                    config.SaveToRegistry();
                    isDelayDropdownOpen = false;
                    UpdateLayout();
                    InvalidateRect(hWnd, NULL, FALSE);
                    return 0;
                } else {
                    isDelayDropdownOpen = false;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }

            // 4. Check Pen Dropdown Click if Open
            if (isPenDropdownOpen) {
                if (x >= penDropdownBounds.left && x <= penDropdownBounds.right &&
                    y >= penDropdownBounds.top && y <= penDropdownBounds.bottom) {
                    
                    int dx = penDropdownBounds.left;
                    int dy = penDropdownBounds.top;

                    // Check Color Swatches
                    int swatchSize = 18;
                    int gap = 8;
                    int startColorX = dx + 12;
                    int startColorY = dy + 26;

                    for (size_t i = 0; i < palette.size(); ++i) {
                        int row = (int)i / 4;
                        int col = (int)i % 4;
                        int sx = startColorX + col * (swatchSize + gap + 18);
                        int sy = startColorY + row * (swatchSize + gap);

                        if (x >= sx - 3 && x <= sx + swatchSize + 3 && y >= sy - 3 && y <= sy + swatchSize + 3) {
                            config.currentColor = palette[i];
                            config.currentTool = AnnotationTool::Pen;
                            config.SaveToRegistry();
                            InvalidateRect(hWnd, NULL, FALSE);
                            return 0;
                        }
                    }

                    // Check Stroke Sizes (5 steps)
                    int sepY = dy + 78;
                    int sizeBtnW = 24;
                    int sizeGap = 6;
                    int startSizeX = dx + 72;
                    int startSizeY = sepY + 5;

                    for (size_t i = 0; i < strokeSizes.size(); ++i) {
                        int bx = startSizeX + (int)i * (sizeBtnW + sizeGap);
                        int by = startSizeY;

                        if (x >= bx && x <= bx + sizeBtnW && y >= by && y <= by + 20) {
                            config.currentStrokeWidth = strokeSizes[i];
                            config.currentTool = AnnotationTool::Pen;
                            config.SaveToRegistry();
                            InvalidateRect(hWnd, NULL, FALSE);
                            return 0;
                        }
                    }

                    return 0;
                } else {
                    isPenDropdownOpen = false;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }

            // 5. Top Bar Button Clicks
            if (y <= TOP_BAR_HEIGHT) {
                for (const auto& btn : topButtons) {
                    if (btn.isSeparator) continue;
                    if (x >= btn.bounds.left && x <= btn.bounds.right && y >= btn.bounds.top && y <= btn.bounds.bottom) {
                        if (btn.id == MainWindowButtonId::NewSnip) {
                            StartNewSnip();
                        } else if (btn.id == MainWindowButtonId::ToggleMode) {
                            bool wasOpen = isModeDropdownOpen;
                            CloseAllDropdowns();
                            isModeDropdownOpen = !wasOpen;
                            InvalidateRect(hWnd, NULL, FALSE);
                            return 0;
                        } else if (btn.id == MainWindowButtonId::ToggleDelay) {
                            bool wasOpen = isDelayDropdownOpen;
                            CloseAllDropdowns();
                            isDelayDropdownOpen = !wasOpen;
                            InvalidateRect(hWnd, NULL, FALSE);
                            return 0;
                        } else if (btn.id == MainWindowButtonId::Tool_Pen) {
                            config.currentTool = AnnotationTool::Pen;
                            bool wasOpen = isPenDropdownOpen;
                            CloseAllDropdowns();
                            isPenDropdownOpen = !wasOpen;
                            InvalidateRect(hWnd, NULL, FALSE);
                            return 0;
                        } else if (btn.id == MainWindowButtonId::Tool_Rect) {
                            config.currentTool = AnnotationTool::Rectangle;
                            CloseAllDropdowns();
                        } else if (btn.id == MainWindowButtonId::Tool_Circle) {
                            config.currentTool = AnnotationTool::Ellipse;
                            CloseAllDropdowns();
                        } else if (btn.id == MainWindowButtonId::Undo) {
                            annotationEngine.Undo();
                            InvalidateRect(hWnd, NULL, FALSE);
                        } else if (btn.id == MainWindowButtonId::Redo) {
                            annotationEngine.Redo();
                            InvalidateRect(hWnd, NULL, FALSE);
                        } else if (btn.id == MainWindowButtonId::Save) {
                            ExecuteSave();
                        } else if (btn.id == MainWindowButtonId::SaveAs) {
                            ExecuteSaveAs();
                        } else if (btn.id == MainWindowButtonId::Settings) {
                            CloseAllDropdowns();
                            settingsWindow.Show(hwnd, config, [this]() {
                                InvalidateRect(hwnd, NULL, FALSE);
                            });
                            return 0;
                        }
                        InvalidateRect(hWnd, NULL, FALSE);
                        return 0;
                    }
                }
                return 0;
            }

            // 6. Canvas drawing click
            if (hasImage) {
                RECT dispRc = GetImageDisplayRect();
                if (x >= dispRc.left && x <= dispRc.right && y >= dispRc.top && y <= dispRc.bottom) {
                    SetCapture(hWnd);
                    isDrawingOnCanvas = true;
                    drawStartPt = ScreenToImageCoords(x, y);

                    Gdiplus::Color drawColor(255, GetRValue(config.currentColor), GetGValue(config.currentColor), GetBValue(config.currentColor));

                    if (config.currentTool == AnnotationTool::Pen) {
                        auto penShape = std::make_unique<PenShape>(drawColor, (float)config.currentStrokeWidth);
                        penShape->AddPoint(drawStartPt.x, drawStartPt.y);
                        annotationEngine.SetActiveShape(std::move(penShape));
                    } else if (config.currentTool == AnnotationTool::Rectangle) {
                        annotationEngine.SetActiveShape(std::make_unique<RectShape>(drawStartPt.x, drawStartPt.y, drawStartPt.x, drawStartPt.y, drawColor, (float)config.currentStrokeWidth));
                    } else if (config.currentTool == AnnotationTool::Ellipse) {
                        annotationEngine.SetActiveShape(std::make_unique<EllipseShape>(drawStartPt.x, drawStartPt.y, drawStartPt.x, drawStartPt.y, drawColor, (float)config.currentStrokeWidth));
                    }
                }
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (isPanning) {
                isPanning = false;
                ReleaseCapture();
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (isDrawingOnCanvas) {
                ReleaseCapture();
                isDrawingOnCanvas = false;
                annotationEngine.CommitActiveShape();
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_KEYDOWN: {
            bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

            if (wParam == 'N' && isCtrl) {
                StartNewSnip();
                return 0;
            } else if (wParam == 'C' && isCtrl) {
                ExecuteCopy();
                return 0;
            } else if (wParam == 'S' && isCtrl && isShift) {
                ExecuteSaveAs();
                return 0;
            } else if (wParam == 'S' && isCtrl) {
                ExecuteSave();
                return 0;
            } else if (wParam == 'Z' && isCtrl) {
                annotationEngine.Undo();
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            } else if (wParam == 'Y' && isCtrl) {
                annotationEngine.Redo();
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            } else if (wParam == '0' && isCtrl) {
                // Reset zoom and pan
                userZoom = 1.0f;
                panOffsetX = 0;
                panOffsetY = 0;
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            break;
        }

        case WM_DESTROY: {
            UnregisterHotKey(hWnd, 3001);
            UnregisterHotKey(hWnd, 3002);
            capturedBmp.reset();
            PostQuitMessage(0);
            return 0;
        }
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
};
