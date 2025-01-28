# Define a static library for CrossroadTrafficMonitoring
add_library(CrossroadTrafficMonitoring STATIC
    CrossroadTrafficMonitoring.cpp
    CrossroadTrafficMonitoring.hpp
)

# Ensure the library can see its own headers
target_include_directories(CrossroadTrafficMonitoring
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Find and link Boost
find_package(Boost REQUIRED)
target_link_libraries(CrossroadTrafficMonitoring
    PUBLIC
        Boost::boost
        pthread
)

add_executable(interactive_main
    interactive_main.cpp
)

# Link interactive_main with the libraries
target_link_libraries(interactive_main
    PRIVATE
        CrossroadTrafficMonitoring
)