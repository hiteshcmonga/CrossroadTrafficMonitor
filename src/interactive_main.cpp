#include "CrossroadTrafficMonitoring.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

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

int main() {
    CrossroadTrafficMonitoring monitor(std::chrono::milliseconds(600000));  // monitoring seconds can be changed as desired.
    int choice;

    do {
        displayMenu();
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                monitor.Start();
                std::cout << "Monitoring started.\n";
                break;

            case 2:
                monitor.Stop();
                std::cout << "Monitoring stopped.\n";
                break;

            case 3:
                monitor.Reset();
                std::cout << "Monitoring reset.\n";
                break;

            case 4: {
                std::cout << "Enter Vehicle Type (Bicycle/Car/Scooter): ";
                std::string type;
                std::cin >> type;
                std::cout << "Enter Vehicle ID: ";
                std::string id;
                std::cin >> id;

                if (type == "Bicycle") {
                    monitor.OnSignal(Bicycle(id));
                } else if (type == "Car") {
                    monitor.OnSignal(Car(id));
                } else if (type == "Scooter") {
                    monitor.OnSignal(Scooter(id));
                } else {
                    std::cout << "Invalid vehicle type.\n";
                }
                break;
            }

            case 5:
                monitor.OnSignal();
                std::cout << "Error signaled.\n";
                break;

            case 6: {
                int subChoice;
                std::cout << "\n--- Display Statistics ---\n";
                std::cout << "1. All vehicles (alphabetical)\n";
                std::cout << "2. By category (Bicycle, Car, Scooter)\n";
                std::cout << "Select an option: ";
                std::cin >> subChoice;

                if (subChoice == 1) {
                    auto stats = monitor.GetStatistics();
                    std::cout << "\n--- Statistics (Alphabetical) ---\n";
                    for (const auto& entry : stats) {
                        std::cout << entry << "\n";
                    }
                } else if (subChoice == 2) {
                    std::cout << "Enter Vehicle Category (Bicycle/Car/Scooter): ";
                    std::string category;
                    std::cin >> category;

                    if (category == "Bicycle") {
                        auto stats = monitor.GetStatistics(VehicleCategory::Bicycle);
                        std::cout << "\n--- Bicycle Statistics ---\n";
                        for (const auto& entry : stats) {
                            std::cout << entry << "\n";
                        }
                    } else if (category == "Car") {
                        auto stats = monitor.GetStatistics(VehicleCategory::Car);
                        std::cout << "\n--- Car Statistics ---\n";
                        for (const auto& entry : stats) {
                            std::cout << entry << "\n";
                        }
                    } else if (category == "Scooter") {
                        auto stats = monitor.GetStatistics(VehicleCategory::Scooter);
                        std::cout << "\n--- Scooter Statistics ---\n";
                        for (const auto& entry : stats) {
                            std::cout << entry << "\n";
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
        }
    } while (choice != 0);

    return 0;
}