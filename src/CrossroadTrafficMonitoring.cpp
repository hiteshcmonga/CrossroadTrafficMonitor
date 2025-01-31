#include "CrossroadTrafficMonitoring.hpp"
#include <algorithm>
#include <boost/intrusive/list.hpp>
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

namespace ctm // ctm == CrossRoad Traffic Monitoring
{

// Constructor
CrossroadTrafficMonitoring::CrossroadTrafficMonitoring(
    std::chrono::milliseconds period)
    : period{period} {
  InitializeFreeList();
  scheduleNextReset();
}

// Free list initialization
void CrossroadTrafficMonitoring::InitializeFreeList() {
  // Create a singly-linked list out of vehiclePool.
  // freeListHead will be the head of linked structure.
  freeListHead = &vehiclePool[0];
  for (std::size_t i = 0; i < MAX_VEHICLES - 1; ++i) {
    vehiclePool[i].category_hook.unlink();
    vehiclePool[i].alphabetical_hook.unlink();
    // link i-th to (i+1)-th
    vehiclePool[i].nextFree = &vehiclePool[i + 1];
  }
  // Mark the tail
  vehiclePool[MAX_VEHICLES - 1].category_hook.unlink();
  vehiclePool[MAX_VEHICLES - 1].alphabetical_hook.unlink();
  vehiclePool[MAX_VEHICLES - 1].nextFree = nullptr;
}

// AllocateVehicle: push from free list
Vehicle *CrossroadTrafficMonitoring::AllocateVehicle() {
  if (!freeListHead) {
    return nullptr; // no more space
  }
  Vehicle *v = freeListHead;
  // Move head
  freeListHead = v->nextFree;
  // Clear out the data
  v->reset();
  return v;
}

// Free vehicle: pop from free list
void CrossroadTrafficMonitoring::FreeVehicle(Vehicle *v) {
  // unlink from any intrusive lists if it's linked
  if (v->category_hook.is_linked())
    v->category_hook.unlink();
  if (v->alphabetical_hook.is_linked())
    v->alphabetical_hook.unlink();

  // push front to free list
  v->nextFree = freeListHead;
  freeListHead = v;
}

// InsertVehicle: add to category & alphabetical lists
void CrossroadTrafficMonitoring::InsertVehicle(Vehicle *v) {
  // Insert into category-specific list
  switch (v->category) {
  case VehicleCategory::Bicycle:
    bicycleList.push_back(*v);
    break;
  case VehicleCategory::Car:
    carList.push_back(*v);
    break;
  case VehicleCategory::Scooter:
    scooterList.push_back(*v);
    break;
  }
  // Insert into alphabetical list
  InsertAlphaSorted(v);
}

// InsertAlphaSorted: maintain alphabetical order by v->id
void CrossroadTrafficMonitoring::InsertAlphaSorted(Vehicle *v) {
  // linear search for the correct insertion position.
  for (auto it = alphabeticalList.begin(); it != alphabeticalList.end(); ++it) {
    if (v->id < it->id) {
      alphabeticalList.insert(it, *v);
      return;
    }
  }
  // If we didn't insert before someone, push_back at the tail
  alphabeticalList.push_back(*v);
}

// FindVehicle in the category list
Vehicle *CrossroadTrafficMonitoring::FindVehicle(VehicleCategory cat,
                                                 const std::string &id) {
  switch (cat) {
  case VehicleCategory::Bicycle:
    for (auto &x : bicycleList) {
      if (x.id == id)
        return &x;
    }
    break;
  case VehicleCategory::Car:
    for (auto &x : carList) {
      if (x.id == id)
        return &x;
    }
    break;
  case VehicleCategory::Scooter:
    for (auto &x : scooterList) {
      if (x.id == id)
        return &x;
    }
    break;
  }
  return nullptr;
}

void CrossroadTrafficMonitoring::scheduleNextReset() {
  nextResetTime = std::chrono::steady_clock::now() + period;
}

void CrossroadTrafficMonitoring::CheckAndHandlePeriodicReset() {
  // If we're in Stopped state, do not reset.
  if (state == State::Stopped)
    return;

  const auto now = std::chrono::steady_clock::now();
  if (now >= nextResetTime) {
    std::cout << "Periodic reset triggered!\n";
    // Perform reset and become Active
    Reset();
  }
}

// State management
void CrossroadTrafficMonitoring::Start() {
  // Start() transitions from Init -> Active
  std::lock_guard<std::mutex> lock(monitorMutex);
  if (state == State::Init) {
    state = State::Active;
    scheduleNextReset();
  }
}

void CrossroadTrafficMonitoring::Stop() {
  std::lock_guard<std::mutex> lock(monitorMutex);
  // Stop(): Active -> Stopped
  if (state == State::Active) {
    state = State::Stopped;
  }
}

void CrossroadTrafficMonitoring::Reset() {
  // Reset(): Transitions to Active from any state, per the spec ("any ->
  // Active"). Even if Stopped, Reset() forces Active and clears stats and error
  // counters.
  state = State::Active;
  errorCount = 0;

  // Free all vehicles in each category
  for (auto it = bicycleList.begin(); it != bicycleList.end();) {
    auto *v = &(*it);
    ++it;
    FreeVehicle(v);
  }
  for (auto it = carList.begin(); it != carList.end();) {
    auto *v = &(*it);
    ++it;
    FreeVehicle(v);
  }
  for (auto it = scooterList.begin(); it != scooterList.end();) {
    auto *v = &(*it);
    ++it;
    FreeVehicle(v);
  }

  alphabeticalList.clear();
  scheduleNextReset();
}

// OnSignal(ResetSignal)
void CrossroadTrafficMonitoring::OnSignal(ResetSignal) {
  std::lock_guard<std::mutex> lock(monitorMutex);
  Reset();
}

// OnSignal() => camera error
void CrossroadTrafficMonitoring::OnSignal() {
  // check for periodic reset first
  std::lock_guard<std::mutex> lock(monitorMutex);
  CheckAndHandlePeriodicReset();
  // If in Init or Stopped => ignore
  if (state == State::Init || state == State::Stopped) {
    return;
  }

  // If in Active => switch to Error state
  if (state == State::Active) {
    ++errorCount; // increment for first error signal
    state = State::Error;
    return;
  }

  // If in Error => increment error count and log
  if (state == State::Error) {
    ++errorCount;
    std::cerr << "[CameraError]: Received empty signal while in Error state\n";
  }
}

// Category deduction
static VehicleCategory deduceCategory(const Bicycle &) {
  return VehicleCategory::Bicycle;
}
static VehicleCategory deduceCategory(const Car &) {
  return VehicleCategory::Car;
}
static VehicleCategory deduceCategory(const Scooter &) {
  return VehicleCategory::Scooter;
}

// Declared as a friend of CrossroadTrafficMonitoring so it can access private
// members without changing the core logic.
template <typename T>
void CrossroadTrafficMonitoring_OnSignal_Helper(
    CrossroadTrafficMonitoring *self, const T &vehicle) {
  self->CheckAndHandlePeriodicReset();

  std::lock_guard<std::mutex> lock(self->monitorMutex);

  if (self->GetCurrentState() == State::Init ||
      self->GetCurrentState() == State::Stopped) {
    return;
  }

  // if in Error => increment errorCount, log, do not count the vehicle
  if (self->GetCurrentState() == State::Error) {
    ++(self->errorCount);
    std::cerr << "Vehicle signal received in Error state. Not counted.\n";
    return;
  }

  // Otherwise (Active):
  // find or create a vehicle
  VehicleCategory cat = deduceCategory(vehicle);
  Vehicle *existing = self->FindVehicle(cat, vehicle.id);
  if (existing) {
    existing->count += 1;
  } else {
    // allocate from free list
    Vehicle *v = self->AllocateVehicle();
    if (!v) {
      // no more space, increment errorCount, go to error state
      ++(self->errorCount);
      std::cerr << "[AllocationError] No space left for new vehicle.\n";
      return;
    }
    v->category = cat;
    v->id = vehicle.id;
    v->count = 1;
    self->InsertVehicle(v);
  }
}

// OnSignal(Bicycle), OnSignal(Car), OnSignal(Scooter)
void CrossroadTrafficMonitoring::OnSignal(const Bicycle &b) {
  CrossroadTrafficMonitoring_OnSignal_Helper(this, b);
}
void CrossroadTrafficMonitoring::OnSignal(const Car &c) {
  CrossroadTrafficMonitoring_OnSignal_Helper(this, c);
}
void CrossroadTrafficMonitoring::OnSignal(const Scooter &s) {
  CrossroadTrafficMonitoring_OnSignal_Helper(this, s);
}

// Getters
unsigned CrossroadTrafficMonitoring::GetErrorCount() const {
  std::lock_guard<std::mutex> lock(monitorMutex);
  return errorCount;
}

std::vector<std::string>
CrossroadTrafficMonitoring::GetStatistics(VehicleCategory cat) const {
  std::lock_guard<std::mutex> lock(monitorMutex);
  std::vector<std::string> result;
  switch (cat) {
  case VehicleCategory::Bicycle:
    for (auto &x : bicycleList) {
      std::string line = x.id + " - " + ToString(x.category) + " (" +
                         std::to_string(x.count) + ")";
      result.push_back(line);
    }
    break;
  case VehicleCategory::Car:
    for (auto &x : carList) {
      std::string line = x.id + " - " + ToString(x.category) + " (" +
                         std::to_string(x.count) + ")";
      result.push_back(line);
    }
    break;
  case VehicleCategory::Scooter:
    for (auto &x : scooterList) {
      std::string line = x.id + " - " + ToString(x.category) + " (" +
                         std::to_string(x.count) + ")";
      result.push_back(line);
    }
    break;
  }
  return result;
}

// GetStatistics() => alphabetical
std::vector<std::string> CrossroadTrafficMonitoring::GetStatistics() const {
  std::lock_guard<std::mutex> lock(monitorMutex);
  std::vector<std::string> result;
  for (auto &x : alphabeticalList) {
    std::string line = x.id + " - " + ToString(x.category) + " (" +
                       std::to_string(x.count) + ")";
    result.push_back(line);
  }
  return result;
}

} // namespace ctm
