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

#ifndef VIZ1090_TYPES_H
#define VIZ1090_TYPES_H

#include <chrono>
#include <cstdint>
#include <optional>

namespace viz1090 {

// Time types using chrono
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;

// Message size constants
inline constexpr int kLongMsgBytes = 14;
inline constexpr int kShortMsgBytes = 7;
inline constexpr int kLongMsgBits = kLongMsgBytes * 8;
inline constexpr int kShortMsgBits = kShortMsgBytes * 8;

// Error correction
inline constexpr int kMaxBitErrors = 2;

// Cache settings
inline constexpr int kIcaoCacheLen = 1024;
inline constexpr int kIcaoCacheTtl = 60;  // seconds

// Display settings
inline constexpr int kInteractiveDeleteTtl = 300;  // seconds
inline constexpr int kInteractiveDisplayTtl = 60;  // seconds

// Network settings
inline constexpr int kClientBufSize = 1024;
inline constexpr int kNetServicesNum = 6;
inline constexpr int kBeastOutputPort = 30005;

// Mode A/C constants
inline constexpr int kModeAcMsgBytes = 2;
inline constexpr uint16_t kModeAcMsgSquelchLevel = 0x07FF;

// Altitude units
enum class AltitudeUnit : uint8_t {
  Feet = 0,
  Meters = 1
};

// Downlink Format types
enum class DownlinkFormat : uint8_t {
  ShortAirSurveillance = 0,
  Altitude = 4,
  Identity = 5,
  AllCallReply = 11,
  LongAirSurveillance = 16,
  ExtendedSquitter = 17,
  ExtendedSquitterNonTransponder = 18,
  MilitaryExtendedSquitter = 19,
  CommBAltitude = 20,
  CommBIdentity = 21,
  CommD = 24
};

// Aircraft flags - indicates which fields are valid
enum class AircraftFlags : uint16_t {
  None = 0,
  LatLonValid = (1 << 0),
  AltitudeValid = (1 << 1),
  HeadingValid = (1 << 2),
  SpeedValid = (1 << 3),
  VertRateValid = (1 << 4),
  SquawkValid = (1 << 5),
  CallsignValid = (1 << 6),
  EwSpeedValid = (1 << 7),
  NsSpeedValid = (1 << 8),
  OnGround = (1 << 9),
  EvenLatLonValid = (1 << 10),
  OddLatLonValid = (1 << 11),
  OnGroundValid = (1 << 12),
  FlightStatusValid = (1 << 13),
  NsEwSpeedValid = (1 << 14),
  LatLonRelativeOk = (1 << 15)
};

// Bitwise operators for AircraftFlags
inline AircraftFlags
operator|(AircraftFlags aLhs, AircraftFlags aRhs) {
  return static_cast<AircraftFlags>(static_cast<uint16_t>(aLhs) |
                                    static_cast<uint16_t>(aRhs));
}

inline AircraftFlags
operator&(AircraftFlags aLhs, AircraftFlags aRhs) {
  return static_cast<AircraftFlags>(static_cast<uint16_t>(aLhs) &
                                    static_cast<uint16_t>(aRhs));
}

inline AircraftFlags&
operator|=(AircraftFlags& aLhs, AircraftFlags aRhs) {
  aLhs = aLhs | aRhs;
  return aLhs;
}

inline bool
hasFlag(AircraftFlags aFlags, AircraftFlags aFlag) {
  return (aFlags & aFlag) != AircraftFlags::None;
}

// Geographic position
struct Position {
  double latitude;
  double longitude;

  bool operator==(const Position& aOther) const {
    return latitude == aOther.latitude && longitude == aOther.longitude;
  }
};

// Position with timestamp for history tracking
struct PositionRecord {
  Position position;
  float heading;
  TimePoint timestamp;
};

// Signal level (0-255)
using SignalLevel = uint8_t;

// ICAO 24-bit address
using IcaoAddress = uint32_t;

// Squawk code (Mode A)
using SquawkCode = uint16_t;

}  // namespace viz1090

#endif  // VIZ1090_TYPES_H
