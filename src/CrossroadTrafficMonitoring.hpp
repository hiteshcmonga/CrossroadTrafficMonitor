#ifndef CROSSROAD_TRAFFIC_MONITORING_HPP
#define CROSSROAD_TRAFFIC_MONITORING_HPP

#include <algorithm>
#include <boost/intrusive/list.hpp>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace ctm // ctm == CrossRoad Traffic Monitoring
{
// Vehicle Category enumeration
enum class VehicleCategory { Bicycle, Car, Scooter };

inline const char *ToString(VehicleCategory cat) {
  switch (cat) {
  case VehicleCategory::Bicycle:
    return "Bicycle";
  case VehicleCategory::Car:
    return "Car";
  case VehicleCategory::Scooter:
    return "Scooter";
  }
  return "Unknown";
}

struct ResetSignal {};

/*
Lightweight wrappers to signal OnSignal
*/
struct Bicycle {
  std::string id;
  explicit Bicycle(std::string id) : id(std::move(id)) {}
};

struct Car {
  std::string id;
  explicit Car(std::string id) : id(std::move(id)) {}
};

struct Scooter {
  std::string id;
  explicit Scooter(std::string id) : id(std::move(id)) {}
};

//-----------------------------------------------------------
// Represents a single vehicle stored in Boost.Intrusive lists.
// Contains:
//    - ID, category, and appearance count
//    - category_hook: for category-specific list
//    - alphabetical_hook: for alphabetical list
//-----------------------------------------------------------
class Vehicle {
public:
  Vehicle() = default;

  VehicleCategory category;
  std::string id;
  unsigned count{0};

  Vehicle *nextFree{nullptr}; // for free list

  // Intrusive hooks: one for category list, one for alphabetical list.
  // Use list_member_hook to store the hooks inside the object.
  typedef boost::intrusive::list_member_hook<
      boost::intrusive::link_mode<boost::intrusive::auto_unlink>>
      Hook;

  Hook category_hook;
  Hook alphabetical_hook;

  // For convenience in resetting this object
  void reset() {
    category = VehicleCategory::Bicycle;
    id.clear();
    count = 0;
    nextFree = nullptr;
  }
};

// CrossroadTrafficMonitoring states
enum class State {
  Init,   // Not started yet. Start() moves this to Active.
  Active, // Accepting signals and counting Vehicles.
  Error,  // Error state. signals increment error count.
  Stopped // Inactive, signals are ignored.
};

// declare the helper so we can make it a friend
template <typename T>
void CrossroadTrafficMonitoring_OnSignal_Helper(
    class CrossroadTrafficMonitoring *self, const T &vehicle);

// Main class: CrossroadTrafficMonitoring
class CrossroadTrafficMonitoring {
public:
  // Constructor with a configurable reset period.
  // The monitoring automatically resets after this period,
  // except if in Stopped state.
  explicit CrossroadTrafficMonitoring(std::chrono::milliseconds period);

  // State transitions:
  void Start();
  void Stop();
  void Reset(); // clearing stats + transitions to Active state

  // OnSignal overloads:
  void OnSignal(const Bicycle &b);
  void OnSignal(const Car &c);
  void OnSignal(const Scooter &s);
  void OnSignal();            // empty => error signal (camera error)
  void OnSignal(ResetSignal); // reset signal

  // Get the number of errors that occurred
  unsigned GetErrorCount() const;

  // Get statistics by category, as lines "ID - Category (count)"
  std::vector<std::string> GetStatistics(VehicleCategory cat) const;

  // Get *all* statistics in alphabetical order
  std::vector<std::string> GetStatistics() const;

  // Get current state
  State GetCurrentState() const { return state; }

  // helper for checking periodic reset status
  void CheckAndHandlePeriodicReset();

private:
  // Make helper function a friend, so it can access private members.
  template <typename T>
  friend void
  CrossroadTrafficMonitoring_OnSignal_Helper(CrossroadTrafficMonitoring *,
                                             const T &);

  // memory pool management
  static constexpr size_t MAX_VEHICLES = 1000;
  Vehicle vehiclePool[MAX_VEHICLES];
  Vehicle *freeListHead{nullptr};

  // Protect shared data
  mutable std::mutex monitorMutex;

  // Helpers to free list
  void InitializeFreeList();
  Vehicle *AllocateVehicle(); // get from free list
  void FreeVehicle(Vehicle *v);

  // Intrusive list definitions
  using CategoryMemberOption =
      boost::intrusive::member_hook<Vehicle, Vehicle::Hook,
                                    &Vehicle::category_hook>;

  // **Disable constant_time_size** because auto_unlink is being used.
  using CategoryList =
      boost::intrusive::list<Vehicle, CategoryMemberOption,
                             boost::intrusive::constant_time_size<false>>;

  CategoryList bicycleList;
  CategoryList carList;
  CategoryList scooterList;

  using AlphaMemberOption =
      boost::intrusive::member_hook<Vehicle, Vehicle::Hook,
                                    &Vehicle::alphabetical_hook>;

  using AlphabeticalList =
      boost::intrusive::list<Vehicle, AlphaMemberOption,
                             boost::intrusive::constant_time_size<false>>;

  AlphabeticalList alphabeticalList;

  // insert newly created Vehicle into both category and alphabetical lists
  void InsertVehicle(Vehicle *v);

  // Find a vehicle in a category list by ID. Return nullptr if not found.
  Vehicle *FindVehicle(VehicleCategory cat, const std::string &id);

  // Insert vehicle in alphabetical order (by v->id) into list
  void InsertAlphaSorted(Vehicle *v);

  // private members
  State state{State::Init};
  unsigned errorCount{0};
  std::chrono::milliseconds period{};
  std::chrono::steady_clock::time_point nextResetTime{};

  void scheduleNextReset();
};

} // namespace ctm

#endif // CROSSROAD_TRAFFIC_MONITORING_HPP
