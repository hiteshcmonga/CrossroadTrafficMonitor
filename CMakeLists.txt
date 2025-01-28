cmake_minimum_required(VERSION 3.15)
project(TrafficMonitoringSystem VERSION 1.0 LANGUAGES CXX)

# Enable C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories (binaries + libraries go here)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Enable testing framework
enable_testing()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)
