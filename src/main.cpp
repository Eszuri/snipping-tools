#include "core/Common.hpp"
#include "core/AppConfig.hpp"
#include "ui/MainWindow.hpp"

// Initialize DPI Awareness dynamically for modern Windows
void InitializeDpiAwareness() {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        auto setDpiContext = (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (setDpiContext) {
            setDpiContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }

    HMODULE hShcore = LoadLibraryW(L"Shcore.dll");
    if (hShcore) {
        typedef HRESULT(WINAPI* SetProcessDpiAwarenessProc)(int);
        auto setDpiAwareness = (SetProcessDpiAwarenessProc)GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (setDpiAwareness) {
            setDpiAwareness(2); // PROCESS_PER_MONITOR_DPI_AWARE
        }
        FreeLibrary(hShcore);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // 1. Initialize DPI Awareness
    InitializeDpiAwareness();

    // 2. Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) {
        MessageBoxW(NULL, L"Failed to initialize GDI+", L"Error", MB_ICONERROR);
        return 1;
    }

    // 3. Create Main Application Window
    MainWindow mainWindow;
    if (!mainWindow.Create()) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    // 4. Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 5. Clean up
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    PWSTR pCmdLine = (argc > 1) ? argv[1] : (PWSTR)L"";
    int ret = wWinMain(hInstance, hPrevInstance, pCmdLine, nCmdShow);
    if (argv) LocalFree(argv);
    return ret;
}
