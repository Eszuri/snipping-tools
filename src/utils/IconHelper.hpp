#pragma once
#include "../core/Common.hpp"
#include <unordered_map>
#include <memory>
#include <string>

enum class AppIconId {
    SnipNew  = 201,
    Mode     = 202,
    Delay    = 203,
    Pen      = 204,
    Rect     = 205,
    Circle   = 206,
    Undo     = 207,
    Redo     = 208,
    Save     = 209,
    SaveAs   = 210,
    Settings = 211,
    ModeRect = 212,
    ModeWindow = 213,
    ModeFullScreen = 214,
    TimerOff = 215
};

class IconHelper {
private:
    std::unordered_map<AppIconId, std::unique_ptr<Gdiplus::Bitmap>> cachedIcons;
    std::wstring exeDir;

    std::wstring GetExecutableDir() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring ws(path);
        size_t pos = ws.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            return ws.substr(0, pos);
        }
        return L".";
    }

    std::unique_ptr<Gdiplus::Bitmap> LoadFromResource(int resId) {
        HRSRC hRes = FindResourceW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(resId), MAKEINTRESOURCEW(10)); // RT_RCDATA is 10
        if (!hRes) return nullptr;
        DWORD resSize = SizeofResource(GetModuleHandleW(NULL), hRes);
        HGLOBAL hGlobal = LoadResource(GetModuleHandleW(NULL), hRes);
        if (!hGlobal) return nullptr;
        void* pData = LockResource(hGlobal);
        if (!pData) return nullptr;

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, resSize);
        if (!hMem) return nullptr;
        void* pMem = GlobalLock(hMem);
        if (pMem) {
            memcpy(pMem, pData, resSize);
            GlobalUnlock(hMem);
        }

        IStream* pStream = nullptr;
        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK) {
            auto bmp = std::make_unique<Gdiplus::Bitmap>(pStream);
            pStream->Release();
            if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) {
                return bmp;
            }
        }
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Bitmap> LoadFromFile(const std::wstring& filename) {
        // 1. Check relative to executable directory: assets/icons/<filename>
        std::wstring fullPath = exeDir + L"\\assets\\icons\\" + filename;
        if (GetFileAttributesW(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            auto bmp = std::make_unique<Gdiplus::Bitmap>(fullPath.c_str());
            if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) return bmp;
        }

        // 2. Check in current working directory: assets/icons/<filename>
        std::wstring cwdPath = L"assets\\icons\\" + filename;
        if (GetFileAttributesW(cwdPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            auto bmp = std::make_unique<Gdiplus::Bitmap>(cwdPath.c_str());
            if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) return bmp;
        }

        return nullptr;
    }

public:
    IconHelper() {
        exeDir = GetExecutableDir();
    }

    void Init() {
        struct IconMeta {
            AppIconId id;
            std::wstring filename;
        };

        const IconMeta metas[] = {
            { AppIconId::SnipNew,  L"snip_new.png" },
            { AppIconId::Mode,     L"mode.png" },
            { AppIconId::Delay,    L"delay.png" },
            { AppIconId::Pen,      L"pen.png" },
            { AppIconId::Rect,     L"rect.png" },
            { AppIconId::Circle,   L"circle.png" },
            { AppIconId::Undo,     L"undo.png" },
            { AppIconId::Redo,     L"redo.png" },
            { AppIconId::Save,     L"save.png" },
            { AppIconId::SaveAs,   L"save_as.png" },
            { AppIconId::Settings, L"settings.png" },
            { AppIconId::ModeRect, L"mode.png" },
            { AppIconId::ModeWindow, L"mode_window.png" },
            { AppIconId::ModeFullScreen, L"mode_fullscreen.png" },
            { AppIconId::TimerOff, L"timer_off.png" }
        };

        for (const auto& meta : metas) {
            // Load from physical asset file first, fallback to embedded resource
            auto bmp = LoadFromFile(meta.filename);
            if (!bmp) {
                bmp = LoadFromResource((int)meta.id);
            }
            if (bmp) {
                cachedIcons[meta.id] = std::move(bmp);
            }
        }
    }

    bool HasIcon(AppIconId id) const {
        auto it = cachedIcons.find(id);
        return it != cachedIcons.end() && it->second != nullptr;
    }

    void Draw(Gdiplus::Graphics& g, AppIconId id, const Gdiplus::RectF& destRect, Gdiplus::Color tint = Gdiplus::Color(255, 255, 255, 255)) {
        auto it = cachedIcons.find(id);
        if (it == cachedIcons.end() || !it->second) return;

        Gdiplus::Bitmap* bmp = it->second.get();

        float r = tint.GetR() / 255.0f;
        float g_col = tint.GetG() / 255.0f;
        float b = tint.GetB() / 255.0f;
        float a = tint.GetA() / 255.0f;

        Gdiplus::ColorMatrix colorMatrix = {
            r,    0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, g_col,0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, b,    0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, a,    0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
        };

        Gdiplus::ImageAttributes imgAttr;
        imgAttr.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

        auto prevInterp = g.GetInterpolationMode();
        auto prevOffset = g.GetPixelOffsetMode();
        auto prevSmooth = g.GetSmoothingMode();

        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

        g.DrawImage(bmp, destRect, 0, 0, (Gdiplus::REAL)bmp->GetWidth(), (Gdiplus::REAL)bmp->GetHeight(), Gdiplus::UnitPixel, &imgAttr);

        g.SetInterpolationMode(prevInterp);
        g.SetPixelOffsetMode(prevOffset);
        g.SetSmoothingMode(prevSmooth);
    }
};
