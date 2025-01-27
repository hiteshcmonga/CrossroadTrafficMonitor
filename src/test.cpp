#include "CrossroadTrafficMonitoring.hpp"
#include <iostream>
#include <cassert>

using namespace ctm;

int main()
{
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(2000));
    assert(monitor.GetCurrentState() == State::Init);

    monitor.OnSignal(Bicycle("ABC-012"));
    assert(monitor.GetStatistics().empty());

    monitor.Start();
    assert(monitor.GetCurrentState() == State::Active);

    monitor.OnSignal(Bicycle("ABC-011"));
    monitor.OnSignal(Car("ABC-012"));
    monitor.OnSignal(Scooter("ABC-014"));

    for (const auto& stat : monitor.GetStatistics()) {
        std::cout << stat << "\n";
    }

    std::cout << "All tests passed!\n";
    return 0;
}
