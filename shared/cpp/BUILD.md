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
cl /LD /EHsc QueueManager.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp /Fe:queue_manager_shared.dll
copy queue_manager_shared.dll ..\..\flutter_app\lib\native\windows\
```

### Windows (MinGW-w64)
```cmd
cd shared\cpp
g++ -shared -fPIC -o queue_manager_shared.dll QueueManager.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp
copy queue_manager_shared.dll ..\..\flutter_app\lib\native\windows\
```

### macOS
```bash
cd shared/cpp
# Build universal (if you need both arm64 and x86_64, add -arch arm64 -arch x86_64)
g++ -std=c++17 -O2 -pthread \
    ../../simulations/QueueSimulator.cpp QueueManager.cpp \
    FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp \
    ThroughputTracker.cpp \
    -lcurl -o queue_sim

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
g++ -shared -fPIC -o libqueue_manager_shared.so QueueManager.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp
cp libqueue_manager_shared.so ../../flutter_app/lib/native/linux/
```

## Simulators (Cross-platform HTTP)

The simulators now use:
 - WinHTTP on Windows (no change)
 - libcurl on macOS/Linux (ensure libcurl is installed)

There are three different queue management simulators available:
1. **QueueSimulator** - Uses SHORTEST_WAIT_TIME strategy (considers queue length + throughput)
2. **QueueSimulatorShortest** - Uses FEWEST_PEOPLE strategy (always chooses line with fewest people)
3. **QueueSimulatorFarthest** - Uses FARTHEST_FROM_ENTRANCE strategy (chooses line where last person is farthest from entrance)

### Install libcurl (if needed)
macOS (usually already present, optional upgrade):
```bash
brew install curl
```
Ubuntu/Debian:
```bash
sudo apt-get update && sudo apt-get install -y libcurl4-openssl-dev
```

### Build & Run Simulators (macOS/Linux)

**Original Simulator (Shortest Wait Time):**
```bash
cd shared/cpp
clang++ -std=c++17 -O2 -pthread \
	../../simulations/QueueSimulator.cpp QueueManager.cpp \
	FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp \
	ThroughputTracker.cpp \
	-lcurl -o queue_sim
./queue_sim
```

**Fewest People Simulator:**
```bash
cd shared/cpp
clang++ -std=c++17 -O2 -pthread \
	../../simulations/QueueSimulatorShortest.cpp QueueManager.cpp \
	FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp \
	ThroughputTracker.cpp \
	-lcurl -o queue_sim_shortest
./queue_sim_shortest
```

**Farthest From Entrance Simulator:**
```bash
cd shared/cpp
clang++ -std=c++17 -O2 -pthread \
	../../simulations/QueueSimulatorFarthest.cpp QueueManager.cpp \
	FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp \
	ThroughputTracker.cpp \
	-lcurl -o queue_sim_farthest
./queue_sim_farthest
```

**Build all simulators at once:**
```bash
cd shared/cpp
# Build all three simulators
clang++ -std=c++17 -O2 -pthread ../../simulations/QueueSimulator.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp -lcurl -o queue_sim
clang++ -std=c++17 -O2 -pthread ../../simulations/QueueSimulatorShortest.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp -lcurl -o queue_sim_shortest
clang++ -std=c++17 -O2 -pthread ../../simulations/QueueSimulatorFarthest.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp -lcurl -o queue_sim_farthest
```

### Build & Run Simulators (Windows using MSVC)

**Original Simulator (Shortest Wait Time):**
```cmd
cd shared\cpp
cl /EHsc /O2 ..\..\simulations\QueueSimulator.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp winhttp.lib /Fe:queue_sim.exe
queue_sim.exe
```

**Fewest People Simulator:**
```cmd
cd shared\cpp
cl /EHsc /O2 ..\..\simulations\QueueSimulatorShortest.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp winhttp.lib /Fe:queue_sim_shortest.exe
queue_sim_shortest.exe
```

**Farthest From Entrance Simulator:**
```cmd
cd shared\cpp
cl /EHsc /O2 ..\..\simulations\QueueSimulatorFarthest.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp winhttp.lib /Fe:queue_sim_farthest.exe
queue_sim_farthest.exe
```

### Build & Run Simulators (Windows MinGW + curl)
Optionally you can also link libcurl on Windows instead of WinHTTP by removing the `_WIN32` define (not default):

**Original Simulator:**
```bash
g++ -std=c++17 -O2 -pthread ../../simulations/QueueSimulator.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp -lcurl -o queue_sim.exe
```

**Fewest People Simulator:**
```bash
g++ -std=c++17 -O2 -pthread ../../simulations/QueueSimulatorShortest.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp -lcurl -o queue_sim_shortest.exe
```

**Farthest From Entrance Simulator:**
```bash
g++ -std=c++17 -O2 -pthread ../../simulations/QueueSimulatorFarthest.cpp QueueManager.cpp FirebaseClient.cpp SimpleHttpClient.cpp FirebaseStructureBuilder.cpp ThroughputTracker.cpp -lcurl -o queue_sim_farthest.exe
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