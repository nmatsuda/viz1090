// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// Copyright (C) 2014, Malcolm Robb <Support@ATTAvionics.com>
// Copyright (C) 2012, Salvatore Sanfilippo <antirez at gmail dot com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef VIZ1090_AIRCRAFT_LIST_H
#define VIZ1090_AIRCRAFT_LIST_H

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "Aircraft.h"
#include "Types.h"

namespace viz1090 {

/// Thread-safe container for tracked aircraft
///
/// Uses std::unordered_map for O(1) lookup by ICAO address and
/// std::shared_mutex for thread-safe read/write access.
class AircraftList {
public:
  using AircraftPtr = std::shared_ptr<Aircraft>;
  using AircraftMap = std::unordered_map<IcaoAddress, AircraftPtr>;

  AircraftList() = default;
  ~AircraftList() = default;

  // Non-copyable
  AircraftList(const AircraftList&) = delete;
  AircraftList& operator=(const AircraftList&) = delete;

  /// Find an aircraft by ICAO address
  /// @param aAddr The ICAO 24-bit address
  /// @return Shared pointer to aircraft, or nullptr if not found
  [[nodiscard]] AircraftPtr find(IcaoAddress aAddr) const;

  /// Find or create an aircraft by ICAO address
  /// @param aAddr The ICAO 24-bit address
  /// @return Shared pointer to existing or newly created aircraft
  [[nodiscard]] AircraftPtr findOrCreate(IcaoAddress aAddr);

  /// Remove stale aircraft that haven't been seen recently
  /// @param aTtl Time-to-live before removal
  /// @return Number of aircraft removed
  size_t removeStale(Seconds aTtl);

  /// Get the number of tracked aircraft
  [[nodiscard]] size_t count() const;

  /// Check if empty
  [[nodiscard]] bool empty() const;

  /// Clear all aircraft
  void clear();

  /// Execute a function for each aircraft (read-only)
  /// @param aFunc Function to execute, receives const AircraftPtr&
  template <typename Func>
  void forEach(Func&& aFunc) const {
    std::shared_lock lock(mMutex);
    for (const auto& [addr, aircraft] : mAircraft) {
      aFunc(aircraft);
    }
  }

  /// Execute a function for each aircraft (mutable)
  /// @param aFunc Function to execute, receives AircraftPtr&
  template <typename Func>
  void forEachMut(Func&& aFunc) {
    std::unique_lock lock(mMutex);
    for (auto& [addr, aircraft] : mAircraft) {
      aFunc(aircraft);
    }
  }

  /// Get a snapshot of all aircraft (for iteration without holding lock)
  /// @return Vector of shared pointers to all aircraft
  [[nodiscard]] std::vector<AircraftPtr> snapshot() const;

  /// Get all aircraft addresses
  /// @return Vector of ICAO addresses
  [[nodiscard]] std::vector<IcaoAddress> addresses() const;

  // Statistics
  struct Stats {
    size_t totalAircraft = 0;
    size_t withPosition = 0;
    size_t withCallsign = 0;
    double avgSignalLevel = 0.0;
  };

  [[nodiscard]] Stats computeStats() const;

private:
  mutable std::shared_mutex mMutex;
  AircraftMap mAircraft;
};

// Inline implementations
inline AircraftList::AircraftPtr
AircraftList::find(IcaoAddress aAddr) const {
  std::shared_lock lock(mMutex);
  auto it = mAircraft.find(aAddr);
  if (it != mAircraft.end()) {
    return it->second;
  }
  return nullptr;
}

inline AircraftList::AircraftPtr
AircraftList::findOrCreate(IcaoAddress aAddr) {
  // First try with shared lock (read-only)
  {
    std::shared_lock lock(mMutex);
    auto it = mAircraft.find(aAddr);
    if (it != mAircraft.end()) {
      return it->second;
    }
  }

  // Need to create - acquire exclusive lock
  std::unique_lock lock(mMutex);

  // Double-check after acquiring exclusive lock
  auto it = mAircraft.find(aAddr);
  if (it != mAircraft.end()) {
    return it->second;
  }

  // Create new aircraft
  auto aircraft = std::make_shared<Aircraft>(aAddr);
  mAircraft[aAddr] = aircraft;
  return aircraft;
}

inline size_t
AircraftList::removeStale(Seconds aTtl) {
  std::unique_lock lock(mMutex);
  size_t removed = 0;

  for (auto it = mAircraft.begin(); it != mAircraft.end();) {
    if (it->second->isStale(aTtl)) {
      it = mAircraft.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }

  return removed;
}

inline size_t
AircraftList::count() const {
  std::shared_lock lock(mMutex);
  return mAircraft.size();
}

inline bool
AircraftList::empty() const {
  std::shared_lock lock(mMutex);
  return mAircraft.empty();
}

inline void
AircraftList::clear() {
  std::unique_lock lock(mMutex);
  mAircraft.clear();
}

inline std::vector<AircraftList::AircraftPtr>
AircraftList::snapshot() const {
  std::shared_lock lock(mMutex);
  std::vector<AircraftPtr> result;
  result.reserve(mAircraft.size());
  for (const auto& [addr, aircraft] : mAircraft) {
    result.push_back(aircraft);
  }
  return result;
}

inline std::vector<IcaoAddress>
AircraftList::addresses() const {
  std::shared_lock lock(mMutex);
  std::vector<IcaoAddress> result;
  result.reserve(mAircraft.size());
  for (const auto& [addr, aircraft] : mAircraft) {
    result.push_back(addr);
  }
  return result;
}

inline AircraftList::Stats
AircraftList::computeStats() const {
  std::shared_lock lock(mMutex);
  Stats stats;
  stats.totalAircraft = mAircraft.size();

  double signalSum = 0.0;

  for (const auto& [addr, aircraft] : mAircraft) {
    if (aircraft->position().has_value()) {
      ++stats.withPosition;
    }
    if (!aircraft->flight().empty()) {
      ++stats.withCallsign;
    }
    signalSum += aircraft->averageSignalLevel();
  }

  if (stats.totalAircraft > 0) {
    stats.avgSignalLevel = signalSum / stats.totalAircraft;
  }

  return stats;
}

}  // namespace viz1090

#endif  // VIZ1090_AIRCRAFT_LIST_H
