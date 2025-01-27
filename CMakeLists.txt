cmake_minimum_required(VERSION 3.15)
project(TrafficMonitoringSystem VERSION 1.0 LANGUAGES CXX)

# Enable C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include Boost headers from the third_party directory
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)
include_directories(${THIRD_PARTY_DIR}/boost)

# Subdirectories
add_subdirectory(src)
#add_subdirectory(tests)

# Enable testing
enable_testing()
