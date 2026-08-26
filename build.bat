@echo off
echo ===================================================
echo Building Ultra-Fast Native Windows Snipping Tool...
echo ===================================================

if not exist bin mkdir bin
if not exist obj mkdir obj

echo Compiling resources...
windres resources\app.rc -O coff -o obj\app.res

echo Compiling C++ source...
g++ -std=c++20 -O3 -mwindows -municode -Isrc src\main.cpp obj\app.res -o bin\NativeSnippingTool.exe ^
    -lgdi32 -lgdiplus -ldwmapi -lcomctl32 -lshcore -lmsimg32 -lole32 -luuid -lcomdlg32 -lshell32 -luser32

if %ERRORLEVEL% EQU 0 (
    echo ===================================================
    echo BUILD SUCCESS!
    echo Output binary: bin\NativeSnippingTool.exe
    echo ===================================================
) else (
    echo ===================================================
    echo BUILD FAILED!
    echo ===================================================
)
