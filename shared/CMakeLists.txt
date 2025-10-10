cmake_minimum_required(VERSION 3.16)
project(queue_simulators)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Simple source compilation - no libraries
add_executable(queue_simulator_shortest 
    ../simulations/QueueSimulatorShortest.cpp 
    queueManagment/QueueManager.cpp
    queueManagment/ThroughputTracker.cpp
    firebase/FirebaseStructureBuilder.cpp
    firebase/FirebaseClient.cpp
    http/SimpleHttpClient.cpp
)

add_executable(queue_simulator_farthest 
    ../simulations/QueueSimulatorFarthest.cpp 
    queueManagment/QueueManager.cpp
    queueManagment/ThroughputTracker.cpp
    firebase/FirebaseStructureBuilder.cpp
    firebase/FirebaseClient.cpp
    http/SimpleHttpClient.cpp
)

add_executable(queue_simulator_project 
    ../simulations/QueueSimulatorProject.cpp 
    queueManagment/QueueManager.cpp
    queueManagment/ThroughputTracker.cpp
    firebase/FirebaseStructureBuilder.cpp
    firebase/FirebaseClient.cpp
    http/SimpleHttpClient.cpp
)

# Include paths
target_include_directories(queue_simulator_shortest PRIVATE . queueManagment firebase http)
target_include_directories(queue_simulator_farthest PRIVATE . queueManagment firebase http)
target_include_directories(queue_simulator_project PRIVATE . queueManagment firebase http)

# Put executables in build folder
set_target_properties(queue_simulator_shortest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")
set_target_properties(queue_simulator_farthest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")
set_target_properties(queue_simulator_project PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# Windows HTTP library
if(WIN32)
    target_link_libraries(queue_simulator_shortest winhttp)
    target_link_libraries(queue_simulator_farthest winhttp)
    target_link_libraries(queue_simulator_project winhttp)
endif()