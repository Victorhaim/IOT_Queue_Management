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
cl /LD /EHsc QueueManager.cpp FirebaseStructureBuilder.cpp /Fe:queue_manager_shared.dll
copy queue_manager_shared.dll ..\..\flutter_app\lib\native\windows\
```

### Windows (MinGW-w64)
```cmd
cd shared\cpp
g++ -shared -fPIC -o queue_manager_shared.dll QueueManager.cpp FirebaseStructureBuilder.cpp
copy queue_manager_shared.dll ..\..\flutter_app\lib\native\windows\
```

### macOS
```bash
cd shared/cpp
# Build universal (if you need both arm64 and x86_64, add -arch arm64 -arch x86_64)
clang++ -std=c++17 -O2 -fPIC -dynamiclib \
	-o libqueue_manager_shared.dylib \
	QueueManager.cpp FirebaseStructureBuilder.cpp

# Create (or reuse) destination inside the Flutter project
mkdir -p ../../flutter_app/native/macos
cp libqueue_manager_shared.dylib ../../flutter_app/native/macos/

# (Optional) Normalize install name for bundling later
# install_name_tool -id @rpath/libqueue_manager_shared.dylib ../../flutter_app/native/macos/libqueue_manager_shared.dylib
```

Note: The previous path `flutter_app/lib/native/macos` did not exist. We now place the dylib under `flutter_app/native/macos`. Adjust your Dart FFI loader accordingly (e.g., `DynamicLibrary.open('native/macos/libqueue_manager_shared.dylib')`).

### Linux
```bash
cd shared/cpp
g++ -shared -fPIC -o libqueue_manager_shared.so QueueManager.cpp FirebaseStructureBuilder.cpp
cp libqueue_manager_shared.so ../../flutter_app/lib/native/linux/
```

## Simulator (Cross-platform HTTP)

The simulator now uses:
 - WinHTTP on Windows (no change)
 - libcurl on macOS/Linux (ensure libcurl is installed)

### Install libcurl (if needed)
macOS (usually already present, optional upgrade):
```bash
brew install curl
```
Ubuntu/Debian:
```bash
sudo apt-get update && sudo apt-get install -y libcurl4-openssl-dev
```

### Build & Run Simulator (macOS/Linux)
```bash
cd shared/cpp
clang++ -std=c++17 -O2 -pthread \
	QueueSimulator.cpp QueueManager.cpp \
	FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp \
	-lcurl -o queue_sim
./queue_sim
```

### Build & Run Simulator (Windows using MSVC)
```cmd
cd shared\cpp
cl /EHsc /O2 QueueSimulator.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp winhttp.lib /Fe:queue_sim.exe
queue_sim.exe
```

### Build & Run Simulator (Windows MinGW + curl)
Optionally you can also link libcurl on Windows instead of WinHTTP by removing the `_WIN32` define (not default):
```bash
g++ -std=c++17 -O2 -pthread QueueSimulator.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp -lcurl -o queue_sim.exe
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