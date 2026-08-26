#pragma once
#include "../core/Common.hpp"

class ClipboardHelper {
public:
    static bool CopyBitmapToClipboard(Gdiplus::Bitmap* bitmap, HWND hwndOwner = NULL) {
        if (!bitmap) return false;

        UINT width = bitmap->GetWidth();
        UINT height = bitmap->GetHeight();

        Gdiplus::Rect rect(0, 0, width, height);
        Gdiplus::BitmapData bmpData;
        
        // Lock 32-bit ARGB
        if (bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData) != Gdiplus::Ok) {
            return false;
        }

        BITMAPV5HEADER bi = { 0 };
        bi.bV5Size = sizeof(BITMAPV5HEADER);
        bi.bV5Width = width;
        bi.bV5Height = -(LONG)height; // Top-down DIB
        bi.bV5Planes = 1;
        bi.bV5BitCount = 32;
        bi.bV5Compression = BI_BITFIELDS;
        bi.bV5AlphaMask = 0xFF000000;
        bi.bV5RedMask   = 0x00FF0000;
        bi.bV5GreenMask = 0x0000FF00;
        bi.bV5BlueMask  = 0x000000FF;

        DWORD dwSize = sizeof(BITMAPV5HEADER) + width * height * 4;
        HGLOBAL hGlobal = GlobalAlloc(GHND, dwSize);
        if (!hGlobal) {
            bitmap->UnlockBits(&bmpData);
            return false;
        }

        BYTE* pData = (BYTE*)GlobalLock(hGlobal);
        if (pData) {
            memcpy(pData, &bi, sizeof(BITMAPV5HEADER));
            BYTE* pPixels = pData + sizeof(BITMAPV5HEADER);
            for (UINT y = 0; y < height; ++y) {
                memcpy(pPixels + y * width * 4, (BYTE*)bmpData.Scan0 + y * bmpData.Stride, width * 4);
            }
            GlobalUnlock(hGlobal);
        }

        bitmap->UnlockBits(&bmpData);

        if (!OpenClipboard(hwndOwner)) {
            GlobalFree(hGlobal);
            return false;
        }

        EmptyClipboard();
        SetClipboardData(CF_DIBV5, hGlobal);
        CloseClipboard();
        return true;
    }
};
