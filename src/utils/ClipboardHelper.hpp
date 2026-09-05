#pragma once
#include "../core/Common.hpp"
#include "FileHelper.hpp"

class ClipboardHelper {
public:
    static bool CopyBitmapToClipboard(Gdiplus::Bitmap* bitmap, HWND hwndOwner = NULL) {
        if (!bitmap) return false;

        UINT width = bitmap->GetWidth();
        UINT height = bitmap->GetHeight();
        if (width == 0 || height == 0) return false;

        Gdiplus::Rect rect(0, 0, width, height);
        Gdiplus::BitmapData bmpData;
        
        // Lock 32-bit ARGB pixels
        if (bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData) != Gdiplus::Ok) {
            return false;
        }

        // Calculate Pixels Per Meter matching the exact image DPI
        LONG ppmX = (LONG)(bitmap->GetHorizontalResolution() * (1000.0f / 25.4f) + 0.5f);
        LONG ppmY = (LONG)(bitmap->GetVerticalResolution() * (1000.0f / 25.4f) + 0.5f);

        // =========================================================================
        // 1. Standard CF_DIB (Universal compatibility for Microsoft Word, Office, Paint, etc.)
        // Standard CF_DIB uses standard bottom-up BITMAPINFOHEADER and BI_RGB
        // =========================================================================
        DWORD dibSize = sizeof(BITMAPINFOHEADER) + width * height * 4;
        HGLOBAL hDib = GlobalAlloc(GHND, dibSize);
        if (hDib) {
            BYTE* pData = (BYTE*)GlobalLock(hDib);
            if (pData) {
                BITMAPINFOHEADER* pBi = (BITMAPINFOHEADER*)pData;
                pBi->biSize = sizeof(BITMAPINFOHEADER);
                pBi->biWidth = (LONG)width;
                pBi->biHeight = (LONG)height; // Positive = standard bottom-up DIB (universally supported)
                pBi->biPlanes = 1;
                pBi->biBitCount = 32;
                pBi->biCompression = BI_RGB;
                pBi->biSizeImage = width * height * 4;
                pBi->biXPelsPerMeter = ppmX;
                pBi->biYPelsPerMeter = ppmY;

                BYTE* pPixels = pData + sizeof(BITMAPINFOHEADER);
                // Copy scanlines in bottom-up order
                for (UINT y = 0; y < height; ++y) {
                    BYTE* srcRow = (BYTE*)bmpData.Scan0 + (height - 1 - y) * bmpData.Stride;
                    BYTE* dstRow = pPixels + y * width * 4;
                    memcpy(dstRow, srcRow, width * 4);
                }
                GlobalUnlock(hDib);
            }
        }

        // =========================================================================
        // 2. CF_DIBV5 (For applications supporting full 32-bit Alpha Channel / DIBV5)
        // =========================================================================
        DWORD dibV5Size = sizeof(BITMAPV5HEADER) + width * height * 4;
        HGLOBAL hDibV5 = GlobalAlloc(GHND, dibV5Size);
        if (hDibV5) {
            BYTE* pData = (BYTE*)GlobalLock(hDibV5);
            if (pData) {
                BITMAPV5HEADER* pBi5 = (BITMAPV5HEADER*)pData;
                pBi5->bV5Size = sizeof(BITMAPV5HEADER);
                pBi5->bV5Width = (LONG)width;
                pBi5->bV5Height = -(LONG)height; // Top-down
                pBi5->bV5Planes = 1;
                pBi5->bV5BitCount = 32;
                pBi5->bV5Compression = BI_BITFIELDS;
                pBi5->bV5AlphaMask = 0xFF000000;
                pBi5->bV5RedMask   = 0x00FF0000;
                pBi5->bV5GreenMask = 0x0000FF00;
                pBi5->bV5BlueMask  = 0x000000FF;
                pBi5->bV5XPelsPerMeter = ppmX;
                pBi5->bV5YPelsPerMeter = ppmY;

                BYTE* pPixels = pData + sizeof(BITMAPV5HEADER);
                for (UINT y = 0; y < height; ++y) {
                    BYTE* srcRow = (BYTE*)bmpData.Scan0 + y * bmpData.Stride;
                    BYTE* dstRow = pPixels + y * width * 4;
                    memcpy(dstRow, srcRow, width * 4);
                }
                GlobalUnlock(hDibV5);
            }
        }

        // =========================================================================
        // 3. PNG Format (For modern apps like Telegram, Discord, Slack, Browsers)
        // Uses the exact same lossless PNG encoding as Save / Save As
        // =========================================================================
        static UINT cfPng = RegisterClipboardFormatW(L"PNG");
        static UINT cfImagePng = RegisterClipboardFormatW(L"image/png");
        HGLOBAL hPng = NULL;
        HGLOBAL hImagePng = NULL;
        IStream* pStream = NULL;
        if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) == S_OK) {
            CLSID pngClsid;
            if (FileHelper::GetEncoderClsid(L"image/png", &pngClsid) != -1) {
                if (bitmap->Save(pStream, &pngClsid, NULL) == Gdiplus::Ok) {
                    STATSTG stat;
                    if (pStream->Stat(&stat, STATFLAG_NONAME) == S_OK && stat.cbSize.QuadPart > 0) {
                        SIZE_T pngSize = (SIZE_T)stat.cbSize.QuadPart;
                        HGLOBAL hStreamGlobal = NULL;
                        if (GetHGlobalFromStream(pStream, &hStreamGlobal) == S_OK && hStreamGlobal) {
                            void* pSrc = GlobalLock(hStreamGlobal);
                            if (pSrc) {
                                hPng = GlobalAlloc(GHND, pngSize);
                                if (hPng) {
                                    void* pDst = GlobalLock(hPng);
                                    if (pDst) {
                                        memcpy(pDst, pSrc, pngSize);
                                        GlobalUnlock(hPng);
                                    }
                                }

                                hImagePng = GlobalAlloc(GHND, pngSize);
                                if (hImagePng) {
                                    void* pDst2 = GlobalLock(hImagePng);
                                    if (pDst2) {
                                        memcpy(pDst2, pSrc, pngSize);
                                        GlobalUnlock(hImagePng);
                                    }
                                }
                                GlobalUnlock(hStreamGlobal);
                            }
                        }
                    }
                }
            }
            pStream->Release();
        }

        bitmap->UnlockBits(&bmpData);

        if (!OpenClipboard(hwndOwner)) {
            if (hDib) GlobalFree(hDib);
            if (hDibV5) GlobalFree(hDibV5);
            if (hPng) GlobalFree(hPng);
            if (hImagePng) GlobalFree(hImagePng);
            return false;
        }

        EmptyClipboard();

        // Register all standard formats so every Windows app pastes in exact original quality
        if (hDib) {
            SetClipboardData(CF_DIB, hDib);
        }
        if (hDibV5) {
            SetClipboardData(CF_DIBV5, hDibV5);
        }
        if (hPng && cfPng != 0) {
            SetClipboardData(cfPng, hPng);
        }
        if (hImagePng && cfImagePng != 0) {
            SetClipboardData(cfImagePng, hImagePng);
        }

        CloseClipboard();
        return true;
    }
};
