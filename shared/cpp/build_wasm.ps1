# PowerShell script for building QueueManager WebAssembly module
# This compiles the C++ QueueManager to WebAssembly for use in Flutter web

# Set up Emscripten environment
$env:PATH = "C:\emsdk;C:\emsdk\upstream\emscripten;" + $env:PATH

# Set the output directory
$OUTPUT_DIR = "..\build\wasm"
if (!(Test-Path $OUTPUT_DIR)) {
    New-Item -ItemType Directory -Path $OUTPUT_DIR -Force | Out-Null
}

Write-Host "Building QueueManager WebAssembly module..."

# Compile QueueManager C++ code to WebAssembly
$emccArgs = @(
    "QueueManager.cpp",
    "QueueStructures.cpp",
    "-O3",
    "-s", "WASM=1",
    "-s", "EXPORTED_FUNCTIONS=`"[`"_queue_manager_create`",`"_queue_manager_destroy`",`"_queue_manager_enqueue`",`"_queue_manager_dequeue`",`"_queue_manager_enqueue_on_line`",`"_queue_manager_size`",`"_queue_manager_is_empty`",`"_queue_manager_is_full`",`"_queue_manager_get_next_line_number`",`"_queue_manager_get_number_of_lines`",`"_queue_manager_get_line_count`",`"_queue_manager_set_line_count`",`"_queue_manager_reset`",`"_queue_manager_get_line_counts_array`",`"_queue_manager_update_from_array`",`"_malloc`",`"_free`"]`"",
    "-s", "EXPORTED_RUNTIME_METHODS=`"[`"cwrap`", `"ccall`", `"getValue`", `"setValue`"]`"",
    "-s", "ALLOW_MEMORY_GROWTH=1",
    "-s", "MODULARIZE=1",
    "-s", "EXPORT_NAME=`"QueueManagerModule`"",
    "-s", "ENVIRONMENT=`"web`"",
    "-s", "FILESYSTEM=0",
    "--no-entry",
    "-o", "$OUTPUT_DIR\queue_manager.js"
)

try {
    & emcc @emccArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed with exit code $LASTEXITCODE"
    }
    
    Write-Host "WebAssembly module built successfully!" -ForegroundColor Green
    Write-Host "Output files:"
    Write-Host "  - $OUTPUT_DIR\queue_manager.js"
    Write-Host "  - $OUTPUT_DIR\queue_manager.wasm"
    
    # Copy the generated files to Flutter web assets
    $FLUTTER_WEB_DIR = "..\..\flutter_app\web"
    if (!(Test-Path "$FLUTTER_WEB_DIR\wasm")) {
        New-Item -ItemType Directory -Path "$FLUTTER_WEB_DIR\wasm" -Force | Out-Null
    }
    
    Copy-Item "$OUTPUT_DIR\queue_manager.js" "$FLUTTER_WEB_DIR\wasm\" -Force
    Copy-Item "$OUTPUT_DIR\queue_manager.wasm" "$FLUTTER_WEB_DIR\wasm\" -Force
    
    Write-Host "Files copied to Flutter web assets!" -ForegroundColor Green
    Write-Host "  - $FLUTTER_WEB_DIR\wasm\queue_manager.js"
    Write-Host "  - $FLUTTER_WEB_DIR\wasm\queue_manager.wasm"
    
} catch {
    Write-Host "Build failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}