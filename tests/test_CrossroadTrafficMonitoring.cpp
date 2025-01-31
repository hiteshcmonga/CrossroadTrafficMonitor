#include "CrossroadTrafficMonitoring.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>

using namespace ctm;

// Helper function to simulate time passage
static void simulateTimePassing(CrossroadTrafficMonitoring &monitor,
                                std::chrono::milliseconds ms) {
    std::this_thread::sleep_for(ms);
    monitor.CheckAndHandlePeriodicReset();
}

//-----------------------------------------------------------------------------
// Test Suite: State Transitions
//-----------------------------------------------------------------------------


TEST(StateTransitions, InitialStateIsInit) {
    std::cout << "\n[TEST] InitialStateIsInit\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));

    std::cout << "  Checking initial state...\n";
    std::cout << "  Expected state: Init (0), Actual: " 
              << static_cast<int>(monitor.GetCurrentState()) << "\n";
    EXPECT_EQ(monitor.GetCurrentState(), State::Init);
    
    std::cout << "  Expected error count: 0, Actual: " 
              << monitor.GetErrorCount() << "\n";
    EXPECT_EQ(monitor.GetErrorCount(), 0u);
    
    std::cout << "  Expected empty statistics: true, Actual: " 
              << std::boolalpha << monitor.GetStatistics().empty() << "\n";
    EXPECT_TRUE(monitor.GetStatistics().empty());
}


TEST(StateTransitions, StartTransitionsInitToActive) {
    std::cout << "\n[TEST] StartTransitionsInitToActive\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));
    
    std::cout << "  Initial state: " 
              << static_cast<int>(monitor.GetCurrentState()) 
              << " (Expected: 0 - Init)\n";
    ASSERT_EQ(monitor.GetCurrentState(), State::Init);

    std::cout << "  Sending signal in Init state...\n";
    monitor.OnSignal(Bicycle("INIT-BIKE"));
    std::cout << "  Expected statistics empty: true, Actual: " 
              << std::boolalpha << monitor.GetStatistics().empty() << "\n";
    EXPECT_TRUE(monitor.GetStatistics().empty());

    std::cout << "  Calling Start()...\n";
    monitor.Start();
    std::cout << "  Post-Start state: " 
              << static_cast<int>(monitor.GetCurrentState()) 
              << " (Expected: 1 - Active)\n";
    EXPECT_EQ(monitor.GetCurrentState(), State::Active);
}

TEST(StateTransitions, StopTransitionsActiveToStopped) {
    std::cout << "\n[TEST] StopTransitionsActiveToStopped\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(2000));
    monitor.Start();
    
    std::cout << "  Pre-Stop state: " 
              << static_cast<int>(monitor.GetCurrentState()) 
              << " (Expected: 1 - Active)\n";
    ASSERT_EQ(monitor.GetCurrentState(), State::Active);

    std::cout << "  Calling Stop()...\n";
    monitor.Stop();
    std::cout << "  Post-Stop state: " 
              << static_cast<int>(monitor.GetCurrentState()) 
              << " (Expected: 3 - Stopped)\n";
    EXPECT_EQ(monitor.GetCurrentState(), State::Stopped);

    std::cout << "  Testing signal in Stopped state...\n";
    monitor.OnSignal(Car("STOPPED-CAR"));
    std::cout << "  Expected statistics empty: true, Actual: " 
              << std::boolalpha << monitor.GetStatistics().empty() << "\n";
    EXPECT_TRUE(monitor.GetStatistics().empty());

    std::cout << "  Testing periodic reset in Stopped state...\n";
    simulateTimePassing(monitor, std::chrono::milliseconds(2500));
    std::cout << "  Expected state remains Stopped: 3, Actual: " 
              << static_cast<int>(monitor.GetCurrentState()) << "\n";
    EXPECT_EQ(monitor.GetCurrentState(), State::Stopped);
}

//-----------------------------------------------------------------------------
// Test Suite: Reset Functionality
//-----------------------------------------------------------------------------

TEST(ResetFunctionality, ManualResetClearsAllData) {
    std::cout << "\n[TEST] ManualResetClearsAllData\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(2000));
    monitor.Start();

    std::cout << "  Adding test vehicles:\n";
    std::cout << "  - Bicycle B1 (x2)\n  - Car C1\n";
    monitor.OnSignal(Bicycle("B1"));
    monitor.OnSignal(Bicycle("B1"));
    monitor.OnSignal(Car("C1"));
    
    std::cout << "  Pre-reset statistics count: " 
              << monitor.GetStatistics().size() << "\n";
    ASSERT_FALSE(monitor.GetStatistics().empty());

    std::cout << "  Performing manual reset...\n";
    monitor.Reset();
    
    std::cout << "  Post-Reset state: " 
              << static_cast<int>(monitor.GetCurrentState()) 
              << " (Expected: 1 - Active)\n";
    EXPECT_EQ(monitor.GetCurrentState(), State::Active);
    
    std::cout << "  Expected empty statistics: true, Actual: " 
              << std::boolalpha << monitor.GetStatistics().empty() << "\n";
    EXPECT_TRUE(monitor.GetStatistics().empty());
    
    std::cout << "  Expected error count: 0, Actual: " 
              << monitor.GetErrorCount() << "\n";
    EXPECT_EQ(monitor.GetErrorCount(), 0u);
}



TEST(ResetFunctionality, PeriodicAutoReset) {
    std::cout << "\n[TEST] PeriodicAutoReset\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));
    monitor.Start();

    std::cout << "  Adding test vehicle B1...\n";
    monitor.OnSignal(Bicycle("B1"));
    std::cout << "  Pre-reset statistics count: " 
              << monitor.GetStatistics().size() << "\n";
    ASSERT_FALSE(monitor.GetStatistics().empty());

    std::cout << "  Simulating 1200ms delay...\n";
    simulateTimePassing(monitor, std::chrono::milliseconds(1200));
    
    std::cout << "  Post-reset statistics count: " 
              << monitor.GetStatistics().size() 
              << " (Expected: 0)\n";
    EXPECT_TRUE(monitor.GetStatistics().empty());
    
    std::cout << "  Expected state: Active (1), Actual: " 
              << static_cast<int>(monitor.GetCurrentState()) << "\n";
    EXPECT_EQ(monitor.GetCurrentState(), State::Active);
}


TEST(ResetFunctionality, PeriodicResetTransitionsErrorToActive) {
    std::cout << "\n[TEST] PeriodicResetTransitionsErrorToActive\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(100));
    monitor.Start();

    std::cout << "  Initial state: " 
              << static_cast<int>(monitor.GetCurrentState()) 
              << " (Expected: 1 - Active)\n";
    ASSERT_EQ(monitor.GetCurrentState(), State::Active);

    std::cout << "  Triggering error with empty signal...\n";
    monitor.OnSignal();
    std::cout << "  Post-error state: " 
              << static_cast<int>(monitor.GetCurrentState()) 
              << " (Expected: 2 - Error)\n";
    ASSERT_EQ(monitor.GetCurrentState(), State::Error);

    std::cout << "  Simulating 150ms delay (period=100ms)...\n";
    simulateTimePassing(monitor, std::chrono::milliseconds(150));
    
    std::cout << "  Post-reset checks:\n";
    std::cout << "  - Expected state: Active (1), Actual: " 
              << static_cast<int>(monitor.GetCurrentState()) << "\n";
    EXPECT_EQ(monitor.GetCurrentState(), State::Active);
    
    std::cout << "  - Expected statistics empty: true, Actual: " 
              << std::boolalpha << monitor.GetStatistics().empty() << "\n";
    EXPECT_TRUE(monitor.GetStatistics().empty());
    
    std::cout << "  - Expected error count reset: 0, Actual: " 
              << monitor.GetErrorCount() << "\n";
    EXPECT_EQ(monitor.GetErrorCount(), 0u);
}

//-----------------------------------------------------------------------------
// Test Suite: Error Handling
//-----------------------------------------------------------------------------

TEST(ErrorHandling, EmptySignalsTriggerErrorState) {
    std::cout << "\n[TEST] EmptySignalsTriggerErrorState\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));
    monitor.Start();

    std::cout << "  Sending empty signal...\n";
    monitor.OnSignal();
    std::cout << "  Expected state: Error (2), Actual: " 
              << static_cast<int>(monitor.GetCurrentState()) << "\n";
    EXPECT_EQ(monitor.GetCurrentState(), State::Error);
    
    std::cout << "  Expected error count: 1, Actual: " 
              << monitor.GetErrorCount() << "\n";
    EXPECT_EQ(monitor.GetErrorCount(), 1u);

    std::cout << "  Sending subsequent signals in Error state:\n";
    std::cout << "  - Empty signal\n  - Car E-CAR\n";
    monitor.OnSignal();
    monitor.OnSignal(Car("E-CAR"));
    
    std::cout << "  Expected error count: 3, Actual: " 
              << monitor.GetErrorCount() << "\n";
    EXPECT_EQ(monitor.GetErrorCount(), 3u);
    
    std::cout << "  Expected statistics empty: true, Actual: " 
              << std::boolalpha << monitor.GetStatistics().empty() << "\n";
    EXPECT_TRUE(monitor.GetStatistics().empty());

    std::cout << "  Simulating 1500ms delay for periodic reset...\n";
    simulateTimePassing(monitor, std::chrono::milliseconds(1500));
    
    std::cout << "  Post-reset state: Active (1), Actual: " 
              << static_cast<int>(monitor.GetCurrentState()) << "\n";
    EXPECT_EQ(monitor.GetCurrentState(), State::Active);
    
    std::cout << "  Expected error count reset: 0, Actual: " 
              << monitor.GetErrorCount() << "\n";
    EXPECT_EQ(monitor.GetErrorCount(), 0u);
}


//-----------------------------------------------------------------------------
// Test Suite: Data Validation
//-----------------------------------------------------------------------------

TEST(DataValidation, VehicleCountingAndOrder) {
    std::cout << "\n[TEST] VehicleCountingAndOrder\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(5000));
    monitor.Start();

    std::cout << "  Adding vehicles:\n";
    std::cout << "  - Bicycle ABC-011 (x2)\n";
    std::cout << "  - Car ABC-012 (x2)\n";
    std::cout << "  - Scooter ABC-014\n";
    std::cout << "  - Bicycle ZZZ-999\n";
    
    monitor.OnSignal(Bicycle("ABC-011"));
    monitor.OnSignal(Car("ABC-012"));
    monitor.OnSignal(Scooter("ABC-014"));
    monitor.OnSignal(Car("ABC-012"));
    monitor.OnSignal(Bicycle("ZZZ-999"));
    monitor.OnSignal(Bicycle("ABC-011"));

    const auto stats = monitor.GetStatistics();
    std::cout << "  Total entries: " << stats.size() << " (Expected: 4)\n";
    ASSERT_EQ(stats.size(), 4u);

    std::cout << "  Verifying order and counts:\n";
    std::cout << "  [0] Expected: ABC-011 - Bicycle (2), Actual: " << stats[0] << "\n";
    EXPECT_EQ(stats[0], "ABC-011 - Bicycle (2)");
    
    std::cout << "  [1] Expected: ABC-012 - Car (2), Actual: " << stats[1] << "\n";
    EXPECT_EQ(stats[1], "ABC-012 - Car (2)");
    
    std::cout << "  [2] Expected: ABC-014 - Scooter (1), Actual: " << stats[2] << "\n";
    EXPECT_EQ(stats[2], "ABC-014 - Scooter (1)");
    
    std::cout << "  [3] Expected: ZZZ-999 - Bicycle (1), Actual: " << stats[3] << "\n";
    EXPECT_EQ(stats[3], "ZZZ-999 - Bicycle (1)");

    std::cout << "  Verifying category-specific counts:\n";
    std::cout << "  Bicycles: " << monitor.GetStatistics(VehicleCategory::Bicycle).size() 
              << " (Expected: 2)\n";
    EXPECT_EQ(monitor.GetStatistics(VehicleCategory::Bicycle).size(), 2u);
    
    std::cout << "  Cars: " << monitor.GetStatistics(VehicleCategory::Car).size() 
              << " (Expected: 1)\n";
    EXPECT_EQ(monitor.GetStatistics(VehicleCategory::Car).size(), 1u);
    
    std::cout << "  Scooters: " << monitor.GetStatistics(VehicleCategory::Scooter).size() 
              << " (Expected: 1)\n";
    EXPECT_EQ(monitor.GetStatistics(VehicleCategory::Scooter).size(), 1u);
}

TEST(DataValidation, SignalHandlingInInvalidStates) {
    std::cout << "\n[TEST] SignalHandlingInInvalidStates\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));
    
    std::cout << "  Testing Init state...\n";
    monitor.OnSignal(Scooter("NOOP"));
    std::cout << "  Expected statistics empty: true, Actual: " 
              << std::boolalpha << monitor.GetStatistics().empty() << "\n";
    EXPECT_TRUE(monitor.GetStatistics().empty());

    std::cout << "  Starting and stopping monitor...\n";
    monitor.Start();
    monitor.Stop();
    
    std::cout << "  Testing Stopped state...\n";
    monitor.OnSignal(Scooter("NOOP2"));
    std::cout << "  Expected statistics empty: true, Actual: " 
              << std::boolalpha << monitor.GetStatistics().empty() << "\n";
    EXPECT_TRUE(monitor.GetStatistics().empty());
}

TEST(DataValidation, SameIdDifferentCategories) {
    std::cout << "\n[TEST] SameIdDifferentCategories\n";
    CrossroadTrafficMonitoring monitor(std::chrono::hours(24));
    monitor.Start();

    std::cout << "  Adding vehicles with same ID across categories:\n";
    std::cout << "  - Bicycle ID-123\n";
    monitor.OnSignal(Bicycle("ID-123"));
    std::cout << "  - Car ID-123\n";
    monitor.OnSignal(Car("ID-123"));
    std::cout << "  - Scooter ID-123\n";
    monitor.OnSignal(Scooter("ID-123"));

    const auto allStats = monitor.GetStatistics();
    std::cout << "  Total unique entries: " << allStats.size() 
              << " (Expected: 3)\n";
    ASSERT_EQ(allStats.size(), 3u);

    std::cout << "  Verifying alphabetical order and counts:\n";
    std::cout << "  [0] Expected: ID-123 - Bicycle (1), Actual: " 
              << allStats[0] << "\n";
    EXPECT_EQ(allStats[0], "ID-123 - Bicycle (1)");
    
    std::cout << "  [1] Expected: ID-123 - Car (1), Actual: " 
              << allStats[1] << "\n";
    EXPECT_EQ(allStats[1], "ID-123 - Car (1)");
    
    std::cout << "  [2] Expected: ID-123 - Scooter (1), Actual: " 
              << allStats[2] << "\n";
    EXPECT_EQ(allStats[2], "ID-123 - Scooter (1)");

    std::cout << "  Verifying category separation:\n";
    std::cout << "  Bicycle list size: " 
              << monitor.GetStatistics(VehicleCategory::Bicycle).size() 
              << " (Expected: 1)\n";
    EXPECT_EQ(monitor.GetStatistics(VehicleCategory::Bicycle).size(), 1u);
    
    std::cout << "  Car list size: " 
              << monitor.GetStatistics(VehicleCategory::Car).size() 
              << " (Expected: 1)\n";
    EXPECT_EQ(monitor.GetStatistics(VehicleCategory::Car).size(), 1u);
    
    std::cout << "  Scooter list size: " 
              << monitor.GetStatistics(VehicleCategory::Scooter).size() 
              << " (Expected: 1)\n";
    EXPECT_EQ(monitor.GetStatistics(VehicleCategory::Scooter).size(), 1u);
}

TEST(DataValidation, TestMaxVehiclesCapacity) {
  std::cout << "\n[TEST] TestMaxVehiclesCapacity\n";
  CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(999999));
  monitor.Start();

  std::cout << "  Adding up to 1000 unique vehicles...\n";
  for (int i = 0; i < 1000; ++i) {
    if (i % 3 == 0)
      monitor.OnSignal(Bicycle("ID-" + std::to_string(i)));
    else if (i % 3 == 1)
      monitor.OnSignal(Car("ID-" + std::to_string(i)));
    else
      monitor.OnSignal(Scooter("ID-" + std::to_string(i)));
  }

  auto statsAll = monitor.GetStatistics();
  std::cout << "  Expect 1000 entries, Actual size=" << statsAll.size()
            << std::endl;
  EXPECT_EQ(statsAll.size(), 1000u);

  std::cout
      << "  Now adding one more unique ID => expect errorCount increment\n";
  monitor.OnSignal(Scooter("ID-1001"));
  std::cout << "  Expect errorCount=1, Actual=" << monitor.GetErrorCount()
            << std::endl;
  EXPECT_EQ(monitor.GetErrorCount(), 1u);

  // Check stats remain 1000
  statsAll = monitor.GetStatistics();
  std::cout << "  Expect size=1000, Actual size=" << statsAll.size()
            << std::endl;
  EXPECT_EQ(statsAll.size(), 1000u);

  std::cout << "  Try adding an existing ID => no new error\n";
  std::cout << "  i=3 => Bicycle(\"ID-3\"), so re-signal Bicycle(\"ID-3\")\n";
  monitor.OnSignal(Bicycle("ID-3"));

  std::cout << "  Expect errorCount still=1, Actual=" << monitor.GetErrorCount()
            << std::endl;
  EXPECT_EQ(monitor.GetErrorCount(), 1u);
}