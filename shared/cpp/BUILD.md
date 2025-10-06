# Build Script for Queue Manager Shared Library

## Prerequisites
You need a C++ compiler installed:
- **Windows**: Visual Studio 2019+ or MinGW-w64
- **macOS**: Xcode Command Line Tools
- **Linux**: GCC or Clang

## Manual Build Instructions

### Windows (Visual Studio)
```cmd
cd shared\cpp
cl /LD /EHsc QueueManager.cpp QueueStructures.cpp /Fe:queue_manager_shared.dll
copy queue_manager_shared.dll ..\..\flutter_app\lib\native\windows\
```

### Windows (MinGW-w64)
```cmd
cd shared\cpp
g++ -shared -fPIC -o queue_manager_shared.dll QueueManager.cpp QueueStructures.cpp
copy queue_manager_shared.dll ..\..\flutter_app\lib\native\windows\
```

### macOS
```bash
cd shared/cpp
clang++ -shared -fPIC -o libqueue_manager_shared.dylib QueueManager.cpp QueueStructures.cpp
cp libqueue_manager_shared.dylib ../../flutter_app/lib/native/macos/
```

### Linux
```bash
cd shared/cpp
g++ -shared -fPIC -o libqueue_manager_shared.so QueueManager.cpp QueueStructures.cpp
cp libqueue_manager_shared.so ../../flutter_app/lib/native/linux/
```

## Alternative: Use CMake (if installed)
```bash
cd shared/cpp
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## For ESP32
The ESP32 will include the `.cpp` and `.h` files directly in its build process.
Update your `platformio.ini` or Arduino IDE to include the shared/cpp directory.

## Testing
After building, you can test the FFI integration by running:
```bash
cd flutter_app
flutter pub get
flutter run
```