#include <gtest/gtest.h>
#include "CrossroadTrafficMonitoring.hpp"
#include <chrono>
#include <thread>
#include <iostream>

using namespace ctm;

// A helper to simulate time passage by calling CheckAndHandlePeriodicReset()
static void simulateTimePassing(CrossroadTrafficMonitoring& monitor, std::chrono::milliseconds ms)
{
    // Simulate time passing by manipulating nextResetTime or using
    // std::this_thread::sleep_for to trigger a reset for testing.
    std::this_thread::sleep_for(ms);
    monitor.CheckAndHandlePeriodicReset();
}

TEST(CrossroadTrafficMonitoringTest, InitialStateIsInit)
{
    std::cout << "\n[TEST] InitialStateIsInit\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));

    // Check state
    std::cout << "  Expecting state=Init, Actual=" 
              << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    EXPECT_EQ(monitor.GetCurrentState(), State::Init);

    // Check error count
    std::cout << "  Expecting errorCount=0, Actual=" << monitor.GetErrorCount() << std::endl;
    EXPECT_EQ(monitor.GetErrorCount(), 0u);

    // Check stats empty
    auto stats = monitor.GetStatistics();
    std::cout << "  Expecting empty stats, Actual size=" << stats.size() << std::endl;
    EXPECT_TRUE(stats.empty());
}

TEST(CrossroadTrafficMonitoringTest, StartTransitionsInitToActive)
{
    std::cout << "\n[TEST] StartTransitionsInitToActive\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));
    std::cout << "  Initial state (expect Init)=" << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    ASSERT_EQ(monitor.GetCurrentState(), State::Init);

    std::cout << "  Signaling Bicycle(\"INIT-BIKE\") in Init => should be ignored\n";
    monitor.OnSignal(Bicycle("INIT-BIKE"));

    auto stats = monitor.GetStatistics();
    std::cout << "  Expecting no stats, Actual size=" << stats.size() << std::endl;
    EXPECT_TRUE(stats.empty());

    std::cout << "  Calling monitor.Start() => expect state=Active\n";
    monitor.Start();
    std::cout << "  Actual state=" << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    EXPECT_EQ(monitor.GetCurrentState(), State::Active);
}

TEST(CrossroadTrafficMonitoringTest, StopTransitionsActiveToStopped)
{
    std::cout << "\n[TEST] StopTransitionsActiveToStopped\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(2000));
    monitor.Start();
    std::cout << "  After Start(): Expect Active, Actual=" 
              << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    ASSERT_EQ(monitor.GetCurrentState(), State::Active);

    std::cout << "  Calling monitor.Stop() => expect state=Stopped\n";
    monitor.Stop();
    std::cout << "  Actual state=" << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    EXPECT_EQ(monitor.GetCurrentState(), State::Stopped);

    std::cout << "  OnSignal(Car(\"STOPPED-CAR\")) in Stopped => ignored\n";
    monitor.OnSignal(Car("STOPPED-CAR"));

    auto stats = monitor.GetStatistics();
    std::cout << "  Expecting stats empty, Actual size=" << stats.size() << std::endl;
    EXPECT_TRUE(stats.empty());

    std::cout << "  simulateTimePassing(2500 ms) => should remain Stopped\n";
    simulateTimePassing(monitor, std::chrono::milliseconds(2500));
    std::cout << "  Actual state=" << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    EXPECT_EQ(monitor.GetCurrentState(), State::Stopped);
}

TEST(CrossroadTrafficMonitoringTest, ResetClearsStatsAndForcesActive)
{
    std::cout << "\n[TEST] ResetClearsStatsAndForcesActive\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(2000));
    monitor.Start();

    std::cout << "  Signaling vehicles...\n";
    monitor.OnSignal(Bicycle("B1"));
    monitor.OnSignal(Bicycle("B1")); // same ID => increments count
    monitor.OnSignal(Car("C1"));

    std::cout << "  Expect state=Active, Actual=" 
              << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    ASSERT_EQ(monitor.GetCurrentState(), State::Active);

    auto beforeReset = monitor.GetStatistics();
    std::cout << "  Stats before Reset, size=" << beforeReset.size() << " (expect >0)\n";
    EXPECT_FALSE(beforeReset.empty());

    std::cout << "  Calling monitor.Reset() => expect state=Active, empty stats, errorCount=0\n";
    monitor.Reset();
    std::cout << "  Actual state=" << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    EXPECT_EQ(monitor.GetCurrentState(), State::Active);

    auto afterReset = monitor.GetStatistics();
    std::cout << "  Stats after Reset, size=" << afterReset.size() << std::endl;
    EXPECT_TRUE(afterReset.empty());

    std::cout << "  ErrorCount=" << monitor.GetErrorCount() << std::endl;
    EXPECT_EQ(monitor.GetErrorCount(), 0u);
}

TEST(CrossroadTrafficMonitoringTest, EmptySignalCausesErrorStateFromActive)
{
    std::cout << "\n[TEST] EmptySignalCausesErrorStateFromActive\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));
    monitor.Start();

    std::cout << "  Expect initial state=Active, Actual=" 
              << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    ASSERT_EQ(monitor.GetCurrentState(), State::Active);

    std::cout << "  OnSignal() => empty => transition to Error\n";
    monitor.OnSignal();
    std::cout << "  Actual state=" << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    EXPECT_EQ(monitor.GetCurrentState(), State::Error);

    std::cout << "  Expect errorCount=1, Actual=" << monitor.GetErrorCount() << std::endl;
    EXPECT_EQ(monitor.GetErrorCount(), 1u);

    std::cout << "  Another empty OnSignal() => increments errorCount\n";
    monitor.OnSignal();
    std::cout << "  errorCount=" << monitor.GetErrorCount() << std::endl;
    EXPECT_EQ(monitor.GetErrorCount(), 2u);

    std::cout << "  OnSignal(Car(\"E-CAR\")) => still in Error => increments errorCount again\n";
    monitor.OnSignal(Car("E-CAR"));
    std::cout << "  errorCount=" << monitor.GetErrorCount() << std::endl;
    EXPECT_EQ(monitor.GetErrorCount(), 3u);

    auto stats = monitor.GetStatistics();
    std::cout << "  Expect stats empty, Actual size=" << stats.size() << std::endl;
    EXPECT_TRUE(stats.empty());

    std::cout << "  simulateTimePassing(1500 ms) => triggers periodic reset => back to Active, error=0\n";
    simulateTimePassing(monitor, std::chrono::milliseconds(1500));
    std::cout << "  Actual state=" << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    EXPECT_EQ(monitor.GetCurrentState(), State::Active);
    std::cout << "  ErrorCount=" << monitor.GetErrorCount() << std::endl;
    EXPECT_EQ(monitor.GetErrorCount(), 0u);

    stats = monitor.GetStatistics();
    std::cout << "  Expect stats empty after reset, size=" << stats.size() << std::endl;
    EXPECT_TRUE(stats.empty());
}

TEST(CrossroadTrafficMonitoringTest, SignalsInInitOrStoppedAreIgnored)
{
    std::cout << "\n[TEST] SignalsInInitOrStoppedAreIgnored\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));
    std::cout << "  Expect state=Init, Actual=" 
              << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    ASSERT_EQ(monitor.GetCurrentState(), State::Init);

    std::cout << "  OnSignal(Scooter(\"NOOP\")) in Init => ignored\n";
    monitor.OnSignal(Scooter("NOOP"));
    auto stats = monitor.GetStatistics();
    std::cout << "  Expect stats empty, size=" << stats.size() << std::endl;
    EXPECT_TRUE(stats.empty());

    std::cout << "  Start() => becomes Active, then Stop() => becomes Stopped\n";
    monitor.Start();
    monitor.Stop();
    std::cout << "  Expect state=Stopped, Actual=" 
              << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    ASSERT_EQ(monitor.GetCurrentState(), State::Stopped);

    std::cout << "  OnSignal(Scooter(\"NOOP2\")) => ignored in Stopped\n";
    monitor.OnSignal(Scooter("NOOP2"));
    stats = monitor.GetStatistics();
    std::cout << "  Expect stats empty, size=" << stats.size() << std::endl;
    EXPECT_TRUE(stats.empty());
}

TEST(CrossroadTrafficMonitoringTest, VehicleCountingWorks)
{
    std::cout << "\n[TEST] VehicleCountingWorks\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(5000));
    monitor.Start();
    std::cout << "  Adding signals: Bicycle(ABC-011), Car(ABC-012), Scooter(ABC-014)\n";
    monitor.OnSignal(Bicycle("ABC-011"));
    monitor.OnSignal(Car("ABC-012"));
    monitor.OnSignal(Scooter("ABC-014"));

    std::cout << "  Re-signaling Car(ABC-012), plus Bicycle(ZZZ-999), Bicycle(ABC-011)\n";
    monitor.OnSignal(Car("ABC-012"));  // same car => increment count to 2
    monitor.OnSignal(Bicycle("ZZZ-999"));
    monitor.OnSignal(Bicycle("ABC-011")); // increment Bicycle ABC-011 => 2

    auto allStats = monitor.GetStatistics();
    std::cout << "  Expect 4 total entries, Actual size=" << allStats.size() << std::endl;
    ASSERT_EQ(allStats.size(), 4u);

    std::cout << "  Expect alphabetical: ABC-011 (Bicycle(2)), ABC-012 (Car(2)), ABC-014 (Scooter(1)), ZZZ-999 (Bicycle(1))\n";
    for (const auto& line : allStats)
        std::cout << "    " << line << std::endl;

    // check the order
    EXPECT_EQ(allStats[0], "ABC-011 - Bicycle (2)");
    EXPECT_EQ(allStats[1], "ABC-012 - Car (2)");
    EXPECT_EQ(allStats[2], "ABC-014 - Scooter (1)");
    EXPECT_EQ(allStats[3], "ZZZ-999 - Bicycle (1)");

    // Check category-specific
    auto bicycleStats = monitor.GetStatistics(VehicleCategory::Bicycle);
    std::cout << "  Bicycle stats size=" << bicycleStats.size() << std::endl;
    EXPECT_EQ(bicycleStats.size(), 2u);

    auto carStats = monitor.GetStatistics(VehicleCategory::Car);
    std::cout << "  Car stats size=" << carStats.size() << std::endl;
    EXPECT_EQ(carStats.size(), 1u);
    EXPECT_EQ(carStats[0], "ABC-012 - Car (2)");

    auto scooterStats = monitor.GetStatistics(VehicleCategory::Scooter);
    std::cout << "  Scooter stats size=" << scooterStats.size() << std::endl;
    EXPECT_EQ(scooterStats.size(), 1u);
    EXPECT_EQ(scooterStats[0], "ABC-014 - Scooter (1)");
}

TEST(CrossroadTrafficMonitoringTest, TestPeriodicResetManualCheck)
{
    std::cout << "\n[TEST] TestPeriodicResetManualCheck\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(1000));
    monitor.Start();

    std::cout << "  Signaling Bicycle(\"B1\") => expect stats not empty\n";
    monitor.OnSignal(Bicycle("B1"));
    auto stats = monitor.GetStatistics();
    std::cout << "  stats size=" << stats.size() << std::endl;
    ASSERT_FALSE(stats.empty());

    std::cout << "  simulateTimePassing(1200 ms) => auto-reset triggered => expect empty stats, Active state\n";
    simulateTimePassing(monitor, std::chrono::milliseconds(1200));
    stats = monitor.GetStatistics();
    std::cout << "  stats size=" << stats.size() << std::endl;
    EXPECT_TRUE(stats.empty());
    std::cout << "  state=" << static_cast<int>(monitor.GetCurrentState()) << std::endl;
    EXPECT_EQ(monitor.GetCurrentState(), State::Active);
}

TEST(CrossroadTrafficMonitoringTest, TestMaxVehiclesCapacity)
{
    std::cout << "\n[TEST] TestMaxVehiclesCapacity\n";
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(999999));
    monitor.Start();

    std::cout << "  Adding up to 1000 unique vehicles...\n";
    for (int i = 0; i < 1000; ++i)
    {
        if (i % 3 == 0)
            monitor.OnSignal(Bicycle("ID-" + std::to_string(i)));
        else if (i % 3 == 1)
            monitor.OnSignal(Car("ID-" + std::to_string(i)));
        else
            monitor.OnSignal(Scooter("ID-" + std::to_string(i)));
    }

    auto statsAll = monitor.GetStatistics();
    std::cout << "  Expect 1000 entries, Actual size=" << statsAll.size() << std::endl;
    EXPECT_EQ(statsAll.size(), 1000u);

    std::cout << "  Now adding one more unique ID => expect errorCount increment\n";
    monitor.OnSignal(Scooter("ID-1001"));
    std::cout << "  Expect errorCount=1, Actual=" << monitor.GetErrorCount() << std::endl;
    EXPECT_EQ(monitor.GetErrorCount(), 1u);

    // Check stats remain 1000
    statsAll = monitor.GetStatistics();
    std::cout << "  Expect size=1000, Actual size=" << statsAll.size() << std::endl;
    EXPECT_EQ(statsAll.size(), 1000u);

    std::cout << "  Try adding an existing ID => no new error\n";
    std::cout << "  i=3 => Bicycle(\"ID-3\"), so re-signal Bicycle(\"ID-3\")\n";
    monitor.OnSignal(Bicycle("ID-3"));

    std::cout << "  Expect errorCount still=1, Actual=" << monitor.GetErrorCount() << std::endl;
    EXPECT_EQ(monitor.GetErrorCount(), 1u);
}
