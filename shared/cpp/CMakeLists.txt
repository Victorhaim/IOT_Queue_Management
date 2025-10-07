cmake_minimum_required(VERSION 3.16)
project(queue_manager_shared)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    QueueManager.cpp
    QueueStructures.cpp
    FirebaseClient.cpp
    SimpleHttpClient.cpp
)

# Create shared library for simulation and ESP32
add_library(queue_manager_shared SHARED ${SOURCES})

# Set output directory for different platforms
if(WIN32)
    set_target_properties(queue_manager_shared PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../../flutter_app/lib/native/windows"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../../flutter_app/lib/native/windows"
    )
elseif(APPLE)
    set_target_properties(queue_manager_shared PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../flutter_app/lib/native/macos"
    )
elseif(UNIX)
    set_target_properties(queue_manager_shared PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../flutter_app/lib/native/linux"
    )
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(queue_manager_shared PRIVATE /W4)
else()
    target_compile_options(queue_manager_shared PRIVATE -Wall -Wextra -Wpedantic)
endif()

# For ESP32 compatibility, we can also create a static library
add_library(queue_manager_static STATIC ${SOURCES})
set_target_properties(queue_manager_static PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../lib"
)

# Create simulator executable
add_executable(queue_simulator QueueSimulator.cpp ${SOURCES})

# Link Windows HTTP library for the simulator and shared library
if(WIN32)
    target_link_libraries(queue_simulator PRIVATE winhttp)
    target_link_libraries(queue_manager_shared PRIVATE winhttp)
endif()

# Set simulator executable output directory
set_target_properties(queue_simulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)