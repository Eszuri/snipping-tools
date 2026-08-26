#pragma once
#include "../core/Common.hpp"
#include "../core/AppConfig.hpp"

enum class ToolbarAction {
    None,
    SetTool_Rect,
    SetTool_Ellipse,
    SetTool_Arrow,
    SetTool_Pen,
    SetTool_Highlighter,
    SetTool_StepBadge,
    SetTool_Pixelate,
    SetTool_Text,
    Undo,
    Redo,
    Pin,
    Save,
    Copy,
    Close,
    SetColor,
    SetSize
};

struct ToolbarButton {
    ToolbarAction action = ToolbarAction::None;
    AnnotationTool tool = AnnotationTool::None;
    std::wstring label;
    std::wstring symbol;
    RECT bounds = { 0, 0, 0, 0 };
    bool isSeparator = false;
    COLORREF colorValue = 0;
    int sizeValue = 0;
};

class ToolbarView {
public:
    RECT currentBounds = { 0, 0, 0, 0 };
    std::vector<ToolbarButton> buttons;
    int hoveredIndex = -1;
    bool isVisible = false;

    // Palette Colors
    const std::vector<COLORREF> palette = {
        RGB(235, 30, 60),   // Red
        RGB(255, 150, 0),   // Orange
        RGB(250, 210, 20),  // Yellow
        RGB(40, 200, 80),   // Green
        RGB(0, 140, 255),   // Blue
        RGB(160, 50, 240),  // Purple
        RGB(255, 255, 255), // White
        RGB(30, 30, 30)     // Black
    };

    ToolbarView() {
        InitButtons();
    }

    void InitButtons() {
        buttons.clear();

        // Tools
        buttons.push_back({ ToolbarAction::SetTool_Rect, AnnotationTool::Rectangle, L"Rectangle", L"▭" });
        buttons.push_back({ ToolbarAction::SetTool_Ellipse, AnnotationTool::Ellipse, L"Ellipse", L"○" });
        buttons.push_back({ ToolbarAction::SetTool_Arrow, AnnotationTool::Arrow, L"Arrow", L"➜" });
        buttons.push_back({ ToolbarAction::SetTool_Pen, AnnotationTool::Pen, L"Pen", L"✎" });
        buttons.push_back({ ToolbarAction::SetTool_Highlighter, AnnotationTool::Highlighter, L"Highlighter", L"🖍" });
        buttons.push_back({ ToolbarAction::SetTool_StepBadge, AnnotationTool::StepBadge, L"Step Counter", L"①" });
        buttons.push_back({ ToolbarAction::SetTool_Pixelate, AnnotationTool::Pixelate, L"Blur / Redact", L"▦" });
        buttons.push_back({ ToolbarAction::SetTool_Text, AnnotationTool::Text, L"Text", L"T" });

        // Separator
        ToolbarButton sep1;
        sep1.isSeparator = true;
        buttons.push_back(sep1);

        // Edit Actions
        buttons.push_back({ ToolbarAction::Undo, AnnotationTool::None, L"Undo (Ctrl+Z)", L"↺" });
        buttons.push_back({ ToolbarAction::Redo, AnnotationTool::None, L"Redo (Ctrl+Y)", L"↻" });

        // Separator
        ToolbarButton sep2;
        sep2.isSeparator = true;
        buttons.push_back(sep2);

        // Output Actions
        buttons.push_back({ ToolbarAction::Pin, AnnotationTool::None, L"Pin to Screen (F2)", L"📌" });
        buttons.push_back({ ToolbarAction::Save, AnnotationTool::None, L"Save As (Ctrl+S)", L"💾" });
        buttons.push_back({ ToolbarAction::Copy, AnnotationTool::None, L"Copy & Finish (Enter)", L"✔" });
        buttons.push_back({ ToolbarAction::Close, AnnotationTool::None, L"Cancel (Esc)", L"✕" });
    }

    void UpdateLayout(int selLeft, int selTop, int selRight, int selBottom, int screenW, int screenH) {
        int btnW = 32;
        int btnH = 32;
        int padding = 4;
        int totalW = 0;

        for (const auto& btn : buttons) {
            if (btn.isSeparator) totalW += 8;
            else totalW += btnW + padding;
        }

        // Add palette width
        int paletteItemW = 16;
        int paletteTotalW = (int)palette.size() * (paletteItemW + 3) + 10;
        totalW += paletteTotalW;

        int totalH = 40;

        // Position toolbar below selection, or above if close to bottom
        int tbX = selRight - totalW;
        int tbY = selBottom + 10;

        if (tbY + totalH > screenH - 10) {
            tbY = selTop - totalH - 10;
        }
        if (tbY < 10) {
            tbY = selTop + 10; // Inside top if both above and below are clamped
        }

        // Clamp X
        if (tbX < 10) tbX = 10;
        if (tbX + totalW > screenW - 10) tbX = screenW - totalW - 10;

        currentBounds = { (LONG)tbX, (LONG)tbY, (LONG)(tbX + totalW), (LONG)(tbY + totalH) };

        // Position buttons
        int curX = tbX + padding + 4;
        int curY = tbY + 4;

        for (auto& btn : buttons) {
            if (btn.isSeparator) {
                btn.bounds = { (LONG)curX, (LONG)(curY + 4), (LONG)(curX + 2), (LONG)(curY + btnH - 4) };
                curX += 8;
            } else {
                btn.bounds = { (LONG)curX, (LONG)curY, (LONG)(curX + btnW), (LONG)(curY + btnH) };
                curX += btnW + padding;
            }
        }
    }

    ToolbarAction HitTest(int x, int y, AppConfig& config, COLORREF& outColorSelected) {
        if (!isVisible) return ToolbarAction::None;

        for (size_t i = 0; i < buttons.size(); ++i) {
            const auto& btn = buttons[i];
            if (btn.isSeparator) continue;

            if (x >= btn.bounds.left && x <= btn.bounds.right && y >= btn.bounds.top && y <= btn.bounds.bottom) {
                if (btn.action == ToolbarAction::SetTool_Rect) config.currentTool = AnnotationTool::Rectangle;
                else if (btn.action == ToolbarAction::SetTool_Ellipse) config.currentTool = AnnotationTool::Ellipse;
                else if (btn.action == ToolbarAction::SetTool_Arrow) config.currentTool = AnnotationTool::Arrow;
                else if (btn.action == ToolbarAction::SetTool_Pen) config.currentTool = AnnotationTool::Pen;
                else if (btn.action == ToolbarAction::SetTool_Highlighter) config.currentTool = AnnotationTool::Highlighter;
                else if (btn.action == ToolbarAction::SetTool_StepBadge) config.currentTool = AnnotationTool::StepBadge;
                else if (btn.action == ToolbarAction::SetTool_Pixelate) config.currentTool = AnnotationTool::Pixelate;
                else if (btn.action == ToolbarAction::SetTool_Text) config.currentTool = AnnotationTool::Text;
                return btn.action;
            }
        }

        // Hit test palette
        int palX = (int)currentBounds.right - (int)palette.size() * 19 - 8;
        int palY = (int)currentBounds.top + 10;
        for (size_t p = 0; p < palette.size(); ++p) {
            int px = palX + (int)p * 19;
            if (x >= px && x <= px + 16 && y >= palY && y <= palY + 16) {
                outColorSelected = palette[p];
                config.currentColor = palette[p];
                return ToolbarAction::SetColor;
            }
        }

        return ToolbarAction::None;
    }

    void OnMouseMove(int x, int y) {
        hoveredIndex = -1;
        if (!isVisible) return;

        for (size_t i = 0; i < buttons.size(); ++i) {
            const auto& btn = buttons[i];
            if (btn.isSeparator) continue;
            if (x >= btn.bounds.left && x <= btn.bounds.right && y >= btn.bounds.top && y <= btn.bounds.bottom) {
                hoveredIndex = (int)i;
                break;
            }
        }
    }

    void Draw(Gdiplus::Graphics& g, const AppConfig& config) {
        if (!isVisible) return;

        int width = currentBounds.right - currentBounds.left;
        int height = currentBounds.bottom - currentBounds.top;
        if (width <= 0 || height <= 0) return;

        // Shadow
        Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(100, 0, 0, 0));
        g.FillRectangle(&shadowBrush, (INT)(currentBounds.left + 3), (INT)(currentBounds.top + 3), (INT)width, (INT)height);

        // Toolbar Background (Fluent dark aesthetic)
        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(245, 32, 33, 36));
        g.FillRectangle(&bgBrush, (INT)currentBounds.left, (INT)currentBounds.top, (INT)width, (INT)height);

        // Toolbar Border
        Gdiplus::Pen borderPen(Gdiplus::Color(255, 60, 64, 70), 1.0f);
        g.DrawRectangle(&borderPen, (INT)currentBounds.left, (INT)currentBounds.top, (INT)width, (INT)height);

        // Fonts
        Gdiplus::Font fontSymbol(L"Segoe UI Symbol", 13.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);

        // Draw buttons
        for (size_t i = 0; i < buttons.size(); ++i) {
            const auto& btn = buttons[i];

            if (btn.isSeparator) {
                Gdiplus::Pen sepPen(Gdiplus::Color(255, 65, 70, 75), 1.0f);
                g.DrawLine(&sepPen, (INT)btn.bounds.left, (INT)btn.bounds.top, (INT)btn.bounds.left, (INT)btn.bounds.bottom);
                continue;
            }

            int bw = btn.bounds.right - btn.bounds.left;
            int bh = btn.bounds.bottom - btn.bounds.top;
            Gdiplus::RectF btnRect((float)btn.bounds.left, (float)btn.bounds.top, (float)bw, (float)bh);

            bool isSelectedTool = (btn.tool != AnnotationTool::None && btn.tool == config.currentTool);
            bool isHovered = ((int)i == hoveredIndex);

            if (isSelectedTool) {
                Gdiplus::SolidBrush selBg(Gdiplus::Color(255, 0, 120, 215)); // Accent Blue
                g.FillRectangle(&selBg, btnRect);
            } else if (isHovered) {
                Gdiplus::SolidBrush hoverBg(Gdiplus::Color(255, 55, 58, 64));
                g.FillRectangle(&hoverBg, btnRect);
            }

            // Button icon/text color
            Gdiplus::Color textColor = (isSelectedTool) ? Gdiplus::Color(255, 255, 255, 255) :
                                       (btn.action == ToolbarAction::Copy) ? Gdiplus::Color(255, 60, 220, 100) :
                                       (btn.action == ToolbarAction::Close) ? Gdiplus::Color(255, 240, 80, 80) :
                                       Gdiplus::Color(255, 220, 225, 230);

            Gdiplus::SolidBrush textBrush(textColor);
            g.DrawString(btn.symbol.c_str(), -1, &fontSymbol, btnRect, &sf, &textBrush);
        }

        // Draw Palette Swatches
        int palX = (int)currentBounds.right - (int)palette.size() * 19 - 8;
        int palY = (int)currentBounds.top + 11;
        for (size_t p = 0; p < palette.size(); ++p) {
            int px = palX + (int)p * 19;
            COLORREF c = palette[p];
            Gdiplus::Color gdiColor(255, GetRValue(c), GetGValue(c), GetBValue(c));
            Gdiplus::SolidBrush palBrush(gdiColor);

            g.FillEllipse(&palBrush, (INT)px, (INT)palY, (INT)14, (INT)14);

            if (c == config.currentColor) {
                // Highlight active color with ring
                Gdiplus::Pen activePen(Gdiplus::Color(255, 255, 255, 255), 2.0f);
                g.DrawEllipse(&activePen, (INT)(px - 2), (INT)(palY - 2), (INT)18, (INT)18);
            }
        }
    }
};
