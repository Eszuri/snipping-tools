#pragma once
#include "../core/Common.hpp"

class MagnifierView {
public:
    static void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* screenBmp, POINT ptVirtual, int virtualX, int virtualY, int screenW, int screenH, int selW = 0, int selH = 0) {
        if (!screenBmp) return;

        int zoom = 4;
        int boxSize = 120; // 120x120 px loupe box
        int sampleGrid = boxSize / (zoom * 2); // Radius in source pixels to sample

        // Convert virtual mouse point to bitmap local coords
        int bmpX = ptVirtual.x - virtualX;
        int bmpY = ptVirtual.y - virtualY;

        if (bmpX < 0 || bmpY < 0 || bmpX >= (int)screenBmp->GetWidth() || bmpY >= (int)screenBmp->GetHeight()) {
            return;
        }

        // Get color of center pixel
        Gdiplus::Color centerColor;
        screenBmp->GetPixel(bmpX, bmpY, &centerColor);

        // Position loupe box near cursor (offset so it doesn't block view)
        int loupeX = bmpX + 24;
        int loupeY = bmpY + 24;

        int infoHeight = 44;
        int totalHeight = boxSize + infoHeight;

        // Flip to left/top if near right/bottom screen boundary
        if (loupeX + boxSize > screenW - 10) {
            loupeX = bmpX - boxSize - 24;
        }
        if (loupeY + totalHeight > screenH - 10) {
            loupeY = bmpY - totalHeight - 24;
        }

        loupeX = (std::max)(5, loupeX);
        loupeY = (std::max)(5, loupeY);

        // 1. Draw zoomed image inside box
        int srcX = bmpX - sampleGrid;
        int srcY = bmpY - sampleGrid;
        int srcW = sampleGrid * 2;
        int srcH = sampleGrid * 2;

        // Background shadow & border
        Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(120, 0, 0, 0));
        g.FillRectangle(&shadowBrush, loupeX + 3, loupeY + 3, boxSize, totalHeight);

        // Pixel zoom area
        g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
        g.DrawImage(screenBmp, Gdiplus::Rect(loupeX, loupeY, boxSize, boxSize), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel);

        // Draw crosshair at center pixel
        int centerBoxX = loupeX + boxSize / 2;
        int centerBoxY = loupeY + boxSize / 2;
        int pixelSizeOnLoupe = zoom * 2;

        Gdiplus::Pen crosshairPen(Gdiplus::Color(255, 255, 60, 60), 1.5f);
        g.DrawRectangle(&crosshairPen, centerBoxX - pixelSizeOnLoupe / 2, centerBoxY - pixelSizeOnLoupe / 2, pixelSizeOnLoupe, pixelSizeOnLoupe);

        // 2. Info panel below loupe
        Gdiplus::SolidBrush infoBgBrush(Gdiplus::Color(240, 24, 24, 28));
        g.FillRectangle(&infoBgBrush, loupeX, loupeY + boxSize, boxSize, infoHeight);

        // Border around loupe
        Gdiplus::Pen borderPen(Gdiplus::Color(255, 80, 80, 85), 1.5f);
        g.DrawRectangle(&borderPen, loupeX, loupeY, boxSize, totalHeight);

        // 3. Render color swatch, HEX, and dimensions
        int swatchSize = 14;
        int swatchX = loupeX + 8;
        int swatchY = loupeY + boxSize + 6;

        Gdiplus::SolidBrush swatchBrush(centerColor);
        g.FillRectangle(&swatchBrush, swatchX, swatchY, swatchSize, swatchSize);
        Gdiplus::Pen swatchBorder(Gdiplus::Color(255, 255, 255, 255), 1.0f);
        g.DrawRectangle(&swatchBorder, swatchX, swatchY, swatchSize, swatchSize);

        // Format HEX string: #RRGGBB
        std::wstringstream hexSs;
        hexSs << L"#"
              << std::uppercase << std::hex << std::setfill(L'0')
              << std::setw(2) << (int)centerColor.GetR()
              << std::setw(2) << (int)centerColor.GetG()
              << std::setw(2) << (int)centerColor.GetB();

        // Format RGB string: RGB(r, g, b)
        std::wstringstream rgbSs;
        rgbSs << L"RGB: " << (int)centerColor.GetR() << L"," << (int)centerColor.GetG() << L"," << (int)centerColor.GetB();

        Gdiplus::Font fontSmall(L"Segoe UI", 9.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::Font fontBold(L"Segoe UI", 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 240, 240, 240));

        g.DrawString(hexSs.str().c_str(), -1, &fontBold, Gdiplus::PointF((float)(swatchX + swatchSize + 6), (float)(swatchY - 1)), &textBrush);
        g.DrawString(rgbSs.str().c_str(), -1, &fontSmall, Gdiplus::PointF((float)(loupeX + 8), (float)(swatchY + swatchSize + 4)), &textBrush);

        // If selection is active, show dimension badge at top of loupe
        if (selW > 0 && selH > 0) {
            std::wstringstream dimSs;
            dimSs << selW << L" x " << selH << L" px";
            Gdiplus::RectF dimRect((float)loupeX, (float)(loupeY - 20), (float)boxSize, 18.0f);
            
            Gdiplus::SolidBrush dimBg(Gdiplus::Color(200, 0, 0, 0));
            g.FillRectangle(&dimBg, dimRect);
            
            Gdiplus::StringFormat sf;
            sf.SetAlignment(Gdiplus::StringAlignmentCenter);
            sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            g.DrawString(dimSs.str().c_str(), -1, &fontSmall, dimRect, &sf, &textBrush);
        }
    }
};
