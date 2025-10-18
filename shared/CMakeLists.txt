cmake_minimum_required(VERSION 3.16)
project(queue_simulators)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Simple source compilation - no libraries
add_executable(queue_simulator_shortest 
    ../simulations/QueueSimulatorShortest.cpp 
    cpp/QueueManager.cpp
    cpp/ThroughputTracker.cpp
    cpp/Person.cpp
    cpp/FirebaseStructureBuilder.cpp
    cpp/FirebasePeopleStructureBuilder.cpp
    cpp/FirebaseClient.cpp
    cpp/SimpleHttpClient.cpp
)

add_executable(queue_simulator_farthest 
    ../simulations/QueueSimulatorFarthest.cpp 
    cpp/QueueManager.cpp
    cpp/ThroughputTracker.cpp
    cpp/Person.cpp
    cpp/FirebaseStructureBuilder.cpp
    cpp/FirebasePeopleStructureBuilder.cpp
    cpp/FirebaseClient.cpp
    cpp/SimpleHttpClient.cpp
)

add_executable(queue_simulator_project 
    ../simulations/QueueSimulatorProject.cpp 
    cpp/QueueManager.cpp
    cpp/ThroughputTracker.cpp
    cpp/Person.cpp
    cpp/FirebaseStructureBuilder.cpp
    cpp/FirebasePeopleStructureBuilder.cpp
    cpp/FirebaseClient.cpp
    cpp/SimpleHttpClient.cpp
)

# Include paths
target_include_directories(queue_simulator_shortest PRIVATE . cpp)
target_include_directories(queue_simulator_farthest PRIVATE . cpp)
target_include_directories(queue_simulator_project PRIVATE . cpp)

# Put executables in bin folder
set_target_properties(queue_simulator_shortest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set_target_properties(queue_simulator_farthest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set_target_properties(queue_simulator_project PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Windows HTTP library
if(WIN32)
    target_link_libraries(queue_simulator_shortest winhttp)
    target_link_libraries(queue_simulator_farthest winhttp)
    target_link_libraries(queue_simulator_project winhttp)
else()
    # macOS/Linux: link with curl
    target_link_libraries(queue_simulator_shortest curl)
    target_link_libraries(queue_simulator_farthest curl)
    target_link_libraries(queue_simulator_project curl)
endif()