@echo off
REM Windows batch script for building QueueManager WebAssembly module
REM This compiles the C++ QueueManager to WebAssembly for use in Flutter web

REM Set the output directory
set OUTPUT_DIR=..\build\wasm
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

REM Compile QueueManager C++ code to WebAssembly
emcc ^
    QueueManager.cpp ^
    QueueStructures.cpp ^
    -O3 ^
    -s WASM=1 ^
    -s EXPORTED_FUNCTIONS="[\"_queue_manager_create\",\"_queue_manager_destroy\",\"_queue_manager_enqueue\",\"_queue_manager_dequeue\",\"_queue_manager_enqueue_on_line\",\"_queue_manager_size\",\"_queue_manager_is_empty\",\"_queue_manager_is_full\",\"_queue_manager_get_next_line_number\",\"_queue_manager_get_number_of_lines\",\"_queue_manager_get_line_count\",\"_queue_manager_set_line_count\",\"_queue_manager_reset\",\"_queue_manager_get_line_counts_array\",\"_queue_manager_update_from_array\",\"_malloc\",\"_free\"]" ^
    -s EXPORTED_RUNTIME_METHODS="[\"cwrap\", \"ccall\", \"getValue\", \"setValue\"]" ^
    -s ALLOW_MEMORY_GROWTH=1 ^
    -s MODULARIZE=1 ^
    -s EXPORT_NAME="QueueManagerModule" ^
    -s ENVIRONMENT="web" ^
    -s FILESYSTEM=0 ^
    --no-entry ^
    -o "%OUTPUT_DIR%\queue_manager.js"

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

echo WebAssembly module built successfully!
echo Output files:
echo   - %OUTPUT_DIR%\queue_manager.js  
echo   - %OUTPUT_DIR%\queue_manager.wasm

REM Copy the generated files to Flutter web assets
set FLUTTER_WEB_DIR=..\..\flutter_app\web
if not exist "%FLUTTER_WEB_DIR%\wasm" mkdir "%FLUTTER_WEB_DIR%\wasm"
copy "%OUTPUT_DIR%\queue_manager.js" "%FLUTTER_WEB_DIR%\wasm\" >nul
copy "%OUTPUT_DIR%\queue_manager.wasm" "%FLUTTER_WEB_DIR%\wasm\" >nul

if %ERRORLEVEL% neq 0 (
    echo Copy failed!
    exit /b 1
)

echo Files copied to Flutter web assets!
echo   - %FLUTTER_WEB_DIR%\wasm\queue_manager.js
echo   - %FLUTTER_WEB_DIR%\wasm\queue_manager.wasm