cmake_minimum_required(VERSION 3.20)
project(NativeSnippingTool LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    resources/app.rc
)

add_executable(NativeSnippingTool WIN32 ${SOURCES})

target_include_directories(NativeSnippingTool PRIVATE src)

target_link_libraries(NativeSnippingTool PRIVATE
    gdi32
    gdiplus
    dwmapi
    comctl32
    shcore
    msimg32
    ole32
    uuid
    comdlg32
    shell32
    user32
)

set_target_properties(NativeSnippingTool PROPERTIES
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
