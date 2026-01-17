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

#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <stdint.h>

#include <chrono>
#include <ctime>
#include <vector>

namespace viz1090 {
namespace core {

/// Position history record for efficient trail rendering
struct PositionHistory {
  float lon;
  float lat;
  float heading;
  std::chrono::high_resolution_clock::time_point timestamp;
};

/// Pure domain model representing an aircraft's state
/// Contains only flight data - no UI/rendering concerns
class Aircraft {
public:
  explicit Aircraft(uint32_t addr);
  ~Aircraft();

  // Non-copyable but movable
  Aircraft(const Aircraft&) = delete;
  Aircraft& operator=(const Aircraft&) = delete;
  Aircraft(Aircraft&&) = default;
  Aircraft& operator=(Aircraft&&) = default;

  /// Get last recorded position (for trail rendering)
  [[nodiscard]] float getLastLon() const;
  [[nodiscard]] float getLastLat() const;
  [[nodiscard]] float getLastHeading() const;

  // Identity
  uint32_t addr;         // ICAO address
  char flight[16];       // Flight number

  // Signal data
  unsigned char signalLevel[8];  // Last 8 Signal Amplitudes
  float messageRate;

  // Flight data
  int altitude;    // Altitude in feet
  int speed;       // Velocity in knots
  int track;       // Angle of flight (heading)
  int vert_rate;   // Vertical rate

  // Timing
  time_t seen;        // Time at which the last packet was received
  time_t seenLatLon;  // Time at which the last lat/lon was received
  time_t prev_seen;
  std::chrono::high_resolution_clock::time_point created;
  std::chrono::high_resolution_clock::time_point msSeen;
  std::chrono::high_resolution_clock::time_point msSeenLatLon;

  // Position
  float lat, lon;  // Coordinates obtained from CPR encoded data
  int live;

  // CPR decoding state
  int evenCprLat = 0;
  int evenCprLon = 0;
  int oddCprLat = 0;
  int oddCprLon = 0;
  uint64_t evenCprTime = 0;
  uint64_t oddCprTime = 0;
  bool cprOddValid = false;
  bool cprEvenValid = false;

  // Position history for trail rendering
  std::vector<PositionHistory> positionHistory;
};

}  // namespace core
}  // namespace viz1090

// Backwards compatibility alias
using Aircraft = viz1090::core::Aircraft;
using PositionHistory = viz1090::core::PositionHistory;

#endif  // AIRCRAFT_H
