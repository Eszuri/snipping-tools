#pragma once
#include "../core/Common.hpp"
#include "../core/AppConfig.hpp"
#include <shobjidl.h>

class FileHelper {
public:
    static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
        UINT num = 0;
        UINT size = 0;
        Gdiplus::GetImageEncodersSize(&num, &size);
        if (size == 0) return -1;

        Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
        if (pImageCodecInfo == NULL) return -1;

        Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
        for (UINT j = 0; j < num; ++j) {
            if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
                *pClsid = pImageCodecInfo[j].Clsid;
                free(pImageCodecInfo);
                return j;
            }
        }
        free(pImageCodecInfo);
        return -1;
    }

    static SIZE_T GetPngSizeBytes(Gdiplus::Bitmap* bitmap) {
        if (!bitmap) return 0;
        IStream* pStream = NULL;
        SIZE_T pngSize = 0;
        if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) == S_OK) {
            CLSID pngClsid;
            if (GetEncoderClsid(L"image/png", &pngClsid) != -1) {
                if (bitmap->Save(pStream, &pngClsid, NULL) == Gdiplus::Ok) {
                    STATSTG stat;
                    if (pStream->Stat(&stat, STATFLAG_NONAME) == S_OK) {
                        pngSize = (SIZE_T)stat.cbSize.QuadPart;
                    }
                }
            }
            pStream->Release();
        }
        return pngSize;
    }

    static std::wstring FormatFileSize(SIZE_T bytes) {
        wchar_t buf[64];
        if (bytes < 1024) {
            swprintf_s(buf, L"%zu B", bytes);
        } else if (bytes < 1024 * 1024) {
            double kb = (double)bytes / 1024.0;
            if (kb >= 100.0) {
                swprintf_s(buf, L"%.0f KB", kb);
            } else {
                swprintf_s(buf, L"%.1f KB", kb);
            }
        } else {
            double mb = (double)bytes / (1024.0 * 1024.0);
            if (mb >= 100.0) {
                swprintf_s(buf, L"%.0f MB", mb);
            } else {
                swprintf_s(buf, L"%.2f MB", mb);
            }
        }
        return buf;
    }

    static std::wstring GenerateTimestampFilename(const std::wstring& ext = L"png") {
        auto now = std::time(nullptr);
        std::tm tm;
        localtime_s(&tm, &now);

        std::wstringstream wss;
        wss << L"Cuplikan_"
            << std::put_time(&tm, L"%Y%m%d_%H%M%S")
            << L"." << ext;
        return wss.str();
    }

    static std::wstring GenerateFilename(const AppConfig& config) {
        if (config.namingMode == NamingMode::Static) {
            std::wstring base = config.staticFilename.empty() ? L"CuplikanLayar" : config.staticFilename;
            return base + L".png";
        } else {
            return GenerateTimestampFilename(L"png");
        }
    }

    static bool SaveBitmapToFile(Gdiplus::Bitmap* bitmap, const std::wstring& filePath) {
        if (!bitmap) return false;

        std::wstring actualPath = filePath;
        if (actualPath.size() < 4 || _wcsicmp(actualPath.substr(actualPath.size() - 4).c_str(), L".png") != 0) {
            actualPath += L".png";
        }

        CLSID encoderClsid;
        if (GetEncoderClsid(L"image/png", &encoderClsid) < 0) {
            return false;
        }

        return bitmap->Save(actualPath.c_str(), &encoderClsid, NULL) == Gdiplus::Ok;
    }

    // 1. Direct Quick Save to Effective Directory using configured naming mode
    static bool QuickSave(Gdiplus::Bitmap* bitmap, const AppConfig& config, std::wstring& outSavedPath) {
        if (!bitmap) return false;
        std::wstring saveDir = config.GetEffectiveSaveDir();
        CreateDirectoryW(saveDir.c_str(), NULL);
        std::wstring filename = GenerateFilename(config);
        outSavedPath = saveDir + L"\\" + filename;
        return SaveBitmapToFile(bitmap, outSavedPath);
    }

    // 2. Save As with File Dialog Picker starting in Effective Directory
    static bool PromptAndSave(Gdiplus::Bitmap* bitmap, const AppConfig& config, HWND hwndOwner, std::wstring& outSavedPath) {
        if (!bitmap) return false;

        WCHAR szFileName[MAX_PATH] = L"";
        std::wstring defaultName = GenerateFilename(config);
        wcsncpy_s(szFileName, defaultName.c_str(), _TRUNCATE);

        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner = hwndOwner;
        ofn.lpstrFilter = L"Gambar PNG (*.png)\0*.png\0";
        ofn.lpstrFile = szFileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"png";

        std::wstring initialDir = config.GetEffectiveSaveDir();
        ofn.lpstrInitialDir = initialDir.c_str();

        if (GetSaveFileNameW(&ofn)) {
            outSavedPath = szFileName;
            if (outSavedPath.size() < 4 || _wcsicmp(outSavedPath.substr(outSavedPath.size() - 4).c_str(), L".png") != 0) {
                outSavedPath += L".png";
            }
            return SaveBitmapToFile(bitmap, outSavedPath);
        }
        return false;
    }

    // 3. Modern Windows Folder Picker
    static bool PickFolder(HWND hwndOwner, std::wstring& outSelectedFolder) {
        IFileDialog* pfd = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
            DWORD dwOptions;
            if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
                pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
            }
            pfd->SetTitle(L"Pilih Folder Default Penyimpanan Cuplikan Layar");
            if (SUCCEEDED(pfd->Show(hwndOwner))) {
                IShellItem* psi = nullptr;
                if (SUCCEEDED(pfd->GetResult(&psi))) {
                    PWSTR pszPath = nullptr;
                    if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                        outSelectedFolder = pszPath;
                        CoTaskMemFree(pszPath);
                        psi->Release();
                        pfd->Release();
                        return true;
                    }
                    psi->Release();
                }
            }
            pfd->Release();
        }
        return false;
    }
};
