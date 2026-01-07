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
//

#ifndef AIRCRAFT_LIST_H
#define AIRCRAFT_LIST_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "core/Aircraft.h"
#include "viz1090/ModesMessage.h"

/// Manages a collection of aircraft with efficient lookup and iteration
class AircraftList {
public:
  using AircraftPtr = std::unique_ptr<Aircraft>;
  using Container = std::vector<AircraftPtr>;
  using iterator = Container::iterator;
  using const_iterator = Container::const_iterator;

  /// Find an aircraft by ICAO address (returns nullptr if not found)
  Aircraft* find(uint32_t aAddr);

  /// Find or create an aircraft
  Aircraft* findOrCreate(uint32_t aAddr);

  /// Update aircraft from a decoded message
  void updateFromMessage(const viz1090::ModesMessage& aMsg);

  /// Remove aircraft not seen within TTL
  void removeStale(std::chrono::seconds aTtl);

  /// Iterator support for range-based for loops
  iterator begin() { return mAircraft.begin(); }
  iterator end() { return mAircraft.end(); }
  const_iterator begin() const { return mAircraft.begin(); }
  const_iterator end() const { return mAircraft.end(); }
  const_iterator cbegin() const { return mAircraft.cbegin(); }
  const_iterator cend() const { return mAircraft.cend(); }

  /// Size access
  [[nodiscard]] size_t size() const { return mAircraft.size(); }
  [[nodiscard]] bool empty() const { return mAircraft.empty(); }

  AircraftList() = default;
  ~AircraftList() = default;

  // Non-copyable but movable
  AircraftList(const AircraftList&) = delete;
  AircraftList& operator=(const AircraftList&) = delete;
  AircraftList(AircraftList&&) = default;
  AircraftList& operator=(AircraftList&&) = default;

private:
  Container mAircraft;
};

#endif
