# Locate Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Gather test source files
file(GLOB TEST_SOURCES *.cpp)

# Create the test executable
add_executable(TrafficMonitoringTests
    ${TEST_SOURCES}
)

# Link the test executable with libraries
target_link_libraries(TrafficMonitoringTests
    PRIVATE
        CrossroadTrafficMonitoring
        GTest::GTest
        GTest::Main
        pthread
)

# Register the test with CTest
add_test(NAME TrafficMonitoringTests COMMAND TrafficMonitoringTests)
