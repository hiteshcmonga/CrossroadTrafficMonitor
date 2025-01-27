cmake_minimum_required(VERSION 3.15)
project(TrafficMonitoringSystem VERSION 1.0 LANGUAGES CXX)

# Enable C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Define source directories
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

# Include source directory
include_directories(${SRC_DIR})

# Add source files
file(GLOB SRC_FILES ${SRC_DIR}/*.cpp)

# Add the main executable
add_executable(interactive_main ${SRC_FILES})

# Add dependencies
target_link_libraries(interactive_main pthread)

# Add tests
enable_testing()
add_subdirectory(tests)
