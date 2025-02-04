#include "CrossroadTrafficMonitoring.hpp"
#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

using namespace ctm;

void displayMenu() {
  std::cout << "\n--- Crossroad Traffic Monitoring ---\n";
  std::cout << "1. Start Monitoring\n";
  std::cout << "2. Stop Monitoring\n";
  std::cout << "3. Reset Monitoring\n";
  std::cout << "4. Signal Vehicle (Bicycle/Car/Scooter)\n";
  std::cout << "5. Signal Error\n";
  std::cout << "6. Display Statistics\n";
  std::cout << "7. Display Error Count\n";
  std::cout << "0. Exit\n";
  std::cout << "Select an option: ";
}
// Helper function for robust input
template<typename T>
T getValidInput(const std::string& prompt) {
  T value;
  while(true) {
    std::cout << prompt;
    if(std::cin >> value) break;
    std::cout << "Invalid input! Please try again.\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return value;
}

int main() {
  CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(
      600000)); // 10-minute period for auto-reset, can be changed as desired.
  int choice;

  do {
    monitor.CheckAndHandlePeriodicReset();
    displayMenu();
    choice = getValidInput<int>("");

    // oldState/newState pattern for Start/Stop/Reset
    State oldState = monitor.GetCurrentState();

    switch(choice) {
    case 1: {
      // Attempt to start monitoring
      monitor.Start();
      State newState = monitor.GetCurrentState();

      if (oldState == newState) {
        // No state change
        if (newState == State::Active) {
          std::cout << "Monitoring is already active.\n";
        } else if (newState == State::Error) {
          std::cout << "Cannot start from Error state.\n";
        } else if (newState == State::Stopped) {
          std::cout << "Cannot start from Stopped state.\n";
        } else {
          // oldState == State::Init but the transition code didn't switch to
          // Active
          std::cout << "Cannot start. Already in Init or invalid state.\n";
        }
      } else {
        // oldState -> newState
        if (newState == State::Active && oldState == State::Init) {
          std::cout << "Monitoring started.\n";
        } else {
          // Possibly from a Reset or an unexpected transition
          std::cout << "Monitoring started (transitioned to Active).\n";
        }
      }
      break;
    }

    case 2: {
      // Attempt to stop monitoring
      monitor.Stop();
      State newState = monitor.GetCurrentState();

      if (oldState == newState) {
        // No state change
        if (newState == State::Stopped) {
          std::cout << "Monitoring is already stopped.\n";
        } else if (newState == State::Init) {
          std::cout << "Monitoring not started yet. Cannot stop.\n";
        } else if (newState == State::Error) {
          std::cout << "Cannot stop from Error state.\n";
        } else if (newState == State::Active) {
          std::cout << "Cannot stop (Unexpected)\n";
        }
      } else {
        // oldState -> newState
        if (oldState == State::Active && newState == State::Stopped) {
          std::cout << "Monitoring stopped.\n";
        } else {
          std::cout << "Unexpected transition to Stopped.\n";
        }
      }
      break;
    }

    case 3: {
      // Reset from any state => Active
      State oldState = monitor.GetCurrentState();
      monitor.Reset();
      State newState = monitor.GetCurrentState();
      // By spec, newState should always be Active
      if (newState == State::Active) {
        std::cout << "Monitoring reset. System is now Active. Error count and "
                     "stats cleared.\n";
      } else {
        // Reset should always result in Active per the specification.
        std::cout
            << "[Warning] Reset failed to transition to Active. Current state: "
            << static_cast<int>(newState) << "\n";
      }
      break;
    }

    case 4: {
      // Signal a vehicle
      std::string type;
      while(true) {
        type = getValidInput<std::string>("Enter Vehicle Type (Bicycle/Car/Scooter): ");
        if(type == "Bicycle" || type == "Car" || type == "Scooter") break;
        std::cout << "Invalid vehicle type! Try again.\n";
      }
      std::string id = getValidInput<std::string>("Enter Vehicle ID: ");

      State beforeSignal = monitor.GetCurrentState();
      if (type == "Bicycle") {
        monitor.OnSignal(Bicycle(id));
      } else if (type == "Car") {
        monitor.OnSignal(Car(id));
      } else if (type == "Scooter") {
        monitor.OnSignal(Scooter(id));
      } else {
        std::cout << "Invalid vehicle type.\n";
        break;
      }
      State afterSignal = monitor.GetCurrentState();

      if (beforeSignal == State::Init || beforeSignal == State::Stopped) {
        // The code ignores signals in these states
        std::cout << "Signal ignored (system not Active).\n";
      } else if (beforeSignal == State::Error) {
        std::cout << "Vehicle signal counted as an error (Error state). Not "
                     "added to stats.\n";
      } else if (beforeSignal == State::Active &&
                 afterSignal == State::Active) {
        // Actually added or incremented the vehicle in stats
        std::cout << "Vehicle signal processed.\n";
      } else if (beforeSignal == State::Active && afterSignal == State::Error) {
        // ran out of memory and it sets state=Error
        std::cout << "Vehicle signal triggered an error.\n";
      }
      break;
    }

    case 5: {
      // Empty signal => camera error
      State beforeSignal = monitor.GetCurrentState();
      monitor.OnSignal();
      State afterSignal = monitor.GetCurrentState();

      if (beforeSignal == State::Init || beforeSignal == State::Stopped) {
        std::cout << "Error signal ignored (system not Active).\n";
      } else if (beforeSignal == State::Active && afterSignal == State::Error) {
        std::cout << "Error signaled: system now in Error state.\n";
      } else if (beforeSignal == State::Error && afterSignal == State::Error) {
        std::cout << "Error signaled again while already in Error state.\n";
      }
      break;
    }

    case 6: {
      // Display statistics
      std::cout << "\n--- Display Statistics ---\n";
      std::cout << "1. All vehicles (alphabetical)\n";
      std::cout << "2. By category (Bicycle, Car, Scooter)\n";
      std::cout << "Select an option: ";
      int subChoice;
      std::cin >> subChoice;

      if (!std::cin) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input.\n";
        break;
      }

      if (subChoice == 1) {
        auto stats = monitor.GetStatistics();
        std::cout << "\n--- Statistics (Alphabetical) ---\n";
        if (stats.empty()) {
          std::cout << "(No vehicles recorded)\n";
        } else {
          for (const auto &entry : stats) {
            std::cout << entry << "\n";
          }
        }
      } else if (subChoice == 2) {
        std::cout << "Enter Vehicle Category (Bicycle/Car/Scooter): ";
        std::string category;
        std::cin >> category;

        if (category == "Bicycle") {
          auto stats = monitor.GetStatistics(VehicleCategory::Bicycle);
          std::cout << "\n--- Bicycle Statistics ---\n";
          if (stats.empty()) {
            std::cout << "(No bicycles recorded)\n";
          } else {
            for (const auto &entry : stats) {
              std::cout << entry << "\n";
            }
          }
        } else if (category == "Car") {
          auto stats = monitor.GetStatistics(VehicleCategory::Car);
          std::cout << "\n--- Car Statistics ---\n";
          if (stats.empty()) {
            std::cout << "(No cars recorded)\n";
          } else {
            for (const auto &entry : stats) {
              std::cout << entry << "\n";
            }
          }
        } else if (category == "Scooter") {
          auto stats = monitor.GetStatistics(VehicleCategory::Scooter);
          std::cout << "\n--- Scooter Statistics ---\n";
          if (stats.empty()) {
            std::cout << "(No scooters recorded)\n";
          } else {
            for (const auto &entry : stats) {
              std::cout << entry << "\n";
            }
          }
        } else {
          std::cout << "Invalid category.\n";
        }
      } else {
        std::cout << "Invalid choice.\n";
      }
      break;
    }

    case 7:
      std::cout << "Error Count: " << monitor.GetErrorCount() << "\n";
      break;

    case 0:
      std::cout << "Exiting program.\n";
      break;

    default:
      std::cout << "Invalid choice. Please try again.\n";
      break;
    }
  } while (choice != 0);

  return 0;
}