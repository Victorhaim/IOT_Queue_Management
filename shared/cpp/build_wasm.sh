#!/bin/bash

# Emscripten build script for QueueManager WebAssembly module
# This compiles the C++ QueueManager to WebAssembly for use in Flutter web

# Set the output directory
OUTPUT_DIR="../build/wasm"
mkdir -p "$OUTPUT_DIR"

# Compile QueueManager C++ code to WebAssembly
emcc \
    QueueManager.cpp \
    QueueStructures.cpp \
    -O3 \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='[
        "_queue_manager_create",
        "_queue_manager_destroy", 
        "_queue_manager_enqueue",
        "_queue_manager_dequeue",
        "_queue_manager_enqueue_on_line",
        "_queue_manager_size",
        "_queue_manager_is_empty",
        "_queue_manager_is_full", 
        "_queue_manager_get_next_line_number",
        "_queue_manager_get_number_of_lines",
        "_queue_manager_get_line_count",
        "_queue_manager_set_line_count",
        "_queue_manager_reset",
        "_queue_manager_get_line_counts_array",
        "_queue_manager_update_from_array",
        "_malloc",
        "_free"
    ]' \
    -s EXPORTED_RUNTIME_METHODS='["cwrap", "ccall", "getValue", "setValue"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME='QueueManagerModule' \
    -s ENVIRONMENT='web' \
    -s FILESYSTEM=0 \
    --no-entry \
    -o "$OUTPUT_DIR/queue_manager.js"

echo "WebAssembly module built successfully!"
echo "Output files:"
echo "  - $OUTPUT_DIR/queue_manager.js"
echo "  - $OUTPUT_DIR/queue_manager.wasm"

# Copy the generated files to Flutter web assets
FLUTTER_WEB_DIR="../../flutter_app/web"
mkdir -p "$FLUTTER_WEB_DIR/wasm"
cp "$OUTPUT_DIR/queue_manager.js" "$FLUTTER_WEB_DIR/wasm/"
cp "$OUTPUT_DIR/queue_manager.wasm" "$FLUTTER_WEB_DIR/wasm/"

echo "Files copied to Flutter web assets!"