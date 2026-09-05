#pragma once
#include "../core/Common.hpp"

struct ScreenBounds {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    RECT ToRect() const {
        RECT rc = { (LONG)x, (LONG)y, (LONG)(x + width), (LONG)(y + height) };
        return rc;
    }
};

class ScreenCapture {
public:
    static ScreenBounds GetVirtualScreenBounds() {
        ScreenBounds bounds;
        bounds.x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        bounds.y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        bounds.width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        bounds.height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        return bounds;
    }

    static std::unique_ptr<Gdiplus::Bitmap> CaptureVirtualScreen(const ScreenBounds& bounds) {
        HDC hdcScreen = GetDC(NULL);
        if (!hdcScreen) return nullptr;

        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        if (!hdcMem) {
            ReleaseDC(NULL, hdcScreen);
            return nullptr;
        }

        // Create 32-bit DIB section for fast Direct access & GDI+ interoperability
        BITMAPINFO bi = { 0 };
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = bounds.width;
        bi.bmiHeader.biHeight = -bounds.height; // Top-down
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        void* pBits = nullptr;
        HBITMAP hBmp = CreateDIBSection(hdcMem, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
        if (!hBmp) {
            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdcScreen);
            return nullptr;
        }

        HGDIOBJ hOld = SelectObject(hdcMem, hBmp);

        // Capture layered/transparent windows too via CAPTUREBLT
        BitBlt(hdcMem, 0, 0, bounds.width, bounds.height, hdcScreen, bounds.x, bounds.y, SRCCOPY | CAPTUREBLT);

        // Ensure 100% opaque alpha channel (0xFF) for all captured screen pixels
        // BitBlt writes RGB without setting alpha (leaves 0x00), which causes GDI+/PNG/Clipboard to treat image as transparent/blurry
        DWORD totalPixels = (DWORD)(bounds.width * bounds.height);
        DWORD* pPixels = (DWORD*)pBits;
        for (DWORD i = 0; i < totalPixels; ++i) {
            pPixels[i] |= 0xFF000000;
        }

        int dpiX = GetDeviceCaps(hdcScreen, LOGPIXELSX);
        int dpiY = GetDeviceCaps(hdcScreen, LOGPIXELSY);

        // Convert to Gdiplus::Bitmap using true 32bpp ARGB with screen DPI resolution
        auto bitmap = std::make_unique<Gdiplus::Bitmap>(bounds.width, bounds.height, bounds.width * 4, PixelFormat32bppARGB, (BYTE*)pBits);
        bitmap->SetResolution((Gdiplus::REAL)dpiX, (Gdiplus::REAL)dpiY);

        // Make an independent copy so we can clean up GDI objects
        std::unique_ptr<Gdiplus::Bitmap> result(bitmap->Clone(0, 0, bounds.width, bounds.height, PixelFormat32bppARGB));
        if (result) {
            result->SetResolution((Gdiplus::REAL)dpiX, (Gdiplus::REAL)dpiY);
        }

        SelectObject(hdcMem, hOld);
        DeleteObject(hBmp);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);

        return result;
    }

    static std::unique_ptr<Gdiplus::Bitmap> CropBitmap(Gdiplus::Bitmap* source, int srcX, int srcY, int width, int height) {
        if (!source || width <= 0 || height <= 0) return nullptr;

        int sw = source->GetWidth();
        int sh = source->GetHeight();

        // Clamp
        srcX = (std::max)(0, (std::min)(srcX, sw - 1));
        srcY = (std::max)(0, (std::min)(srcY, sh - 1));
        width = (std::min)(width, sw - srcX);
        height = (std::min)(height, sh - srcY);

        if (width <= 0 || height <= 0) return nullptr;

        auto cropped = std::unique_ptr<Gdiplus::Bitmap>(source->Clone(srcX, srcY, width, height, PixelFormat32bppARGB));
        if (cropped) {
            cropped->SetResolution(source->GetHorizontalResolution(), source->GetVerticalResolution());
        }
        return cropped;
    }
};
