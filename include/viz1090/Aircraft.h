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

#ifndef VIZ1090_AIRCRAFT_H
#define VIZ1090_AIRCRAFT_H

#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "Types.h"

namespace viz1090 {

// Forward declaration - AircraftLabel is defined in the legacy code for now
class AircraftLabel;

/// Aircraft tracked by the system
///
/// Represents a single aircraft with its current state and position history.
/// Uses modern C++17 features including std::optional for nullable fields.
class Aircraft {
public:
  /// Maximum number of position history records to keep
  static constexpr size_t kMaxHistorySize = 100;

  /// Signal history size
  static constexpr size_t kSignalHistorySize = 8;

  explicit Aircraft(IcaoAddress aAddr);
  ~Aircraft();

  // Non-copyable but movable
  Aircraft(const Aircraft&) = delete;
  Aircraft& operator=(const Aircraft&) = delete;
  Aircraft(Aircraft&&) = default;
  Aircraft& operator=(Aircraft&&) = default;

  // Identity
  [[nodiscard]] IcaoAddress addr() const { return mAddr; }

  [[nodiscard]] std::string_view flight() const {
    return std::string_view(mFlight.data());
  }
  void setFlight(const char* aFlight);
  void setFlight(std::string_view aFlight);

  // Position
  [[nodiscard]] std::optional<Position> position() const { return mPosition; }
  void setPosition(double aLat, double aLon);
  void setPosition(const Position& aPos);

  [[nodiscard]] std::optional<Position> lastPosition() const;
  [[nodiscard]] std::optional<float> lastHeading() const;

  // Flight parameters
  [[nodiscard]] std::optional<int> altitude() const { return mAltitude; }
  void setAltitude(int aAlt) { mAltitude = aAlt; }

  [[nodiscard]] std::optional<int> speed() const { return mSpeed; }
  void setSpeed(int aSpeed) { mSpeed = aSpeed; }

  [[nodiscard]] std::optional<int> track() const { return mTrack; }
  void setTrack(int aTrack) { mTrack = aTrack; }

  [[nodiscard]] std::optional<int> vertRate() const { return mVertRate; }
  void setVertRate(int aRate) { mVertRate = aRate; }

  [[nodiscard]] std::optional<SquawkCode> squawk() const { return mSquawk; }
  void setSquawk(SquawkCode aSquawk) { mSquawk = aSquawk; }

  // Signal
  [[nodiscard]] const std::array<SignalLevel, kSignalHistorySize>&
  signalLevels() const {
    return mSignalLevels;
  }
  void addSignalLevel(SignalLevel aLevel);
  [[nodiscard]] float averageSignalLevel() const;

  [[nodiscard]] float messageRate() const { return mMessageRate; }
  void setMessageRate(float aRate) { mMessageRate = aRate; }

  // Timestamps
  [[nodiscard]] TimePoint seen() const { return mSeen; }
  void updateSeen();

  [[nodiscard]] TimePoint seenLatLon() const { return mSeenLatLon; }
  void updateSeenLatLon();

  [[nodiscard]] TimePoint created() const { return mCreated; }

  // Message count
  [[nodiscard]] long messageCount() const { return mMessageCount; }
  void incrementMessageCount() { ++mMessageCount; }

  // Position history for trail drawing
  [[nodiscard]] const std::deque<PositionRecord>& positionHistory() const {
    return mPositionHistory;
  }
  void addPositionToHistory(const Position& aPos, float aHeading);
  void trimHistory();

  // CPR (Compact Position Reporting) state
  struct CprState {
    int oddLat = 0;
    int oddLon = 0;
    int evenLat = 0;
    int evenLon = 0;
    uint64_t oddTime = 0;
    uint64_t evenTime = 0;
  };

  [[nodiscard]] const CprState& cprState() const { return mCprState; }
  [[nodiscard]] CprState& cprState() { return mCprState; }

  // Flags
  [[nodiscard]] AircraftFlags flags() const { return mFlags; }
  void setFlags(AircraftFlags aFlags) { mFlags = aFlags; }
  void addFlags(AircraftFlags aFlags) { mFlags |= aFlags; }
  [[nodiscard]] bool hasFlag(AircraftFlags aFlag) const {
    return viz1090::hasFlag(mFlags, aFlag);
  }

  // Display state (for UI layer)
  int displayX = 0;
  int displayY = 0;
  // Note: label is managed by UI layer, not owned by Aircraft
  AircraftLabel* label = nullptr;

  // Utility methods
  [[nodiscard]] bool isStale(Seconds aTtl) const;
  [[nodiscard]] Milliseconds timeSinceSeen() const;

private:
  // Identity
  IcaoAddress mAddr;
  std::array<char, 16> mFlight{};

  // Position
  std::optional<Position> mPosition;

  // Flight parameters
  std::optional<int> mAltitude;
  std::optional<int> mSpeed;
  std::optional<int> mTrack;
  std::optional<int> mVertRate;
  std::optional<SquawkCode> mSquawk;

  // Signal
  std::array<SignalLevel, kSignalHistorySize> mSignalLevels{};
  size_t mSignalIndex = 0;
  float mMessageRate = 0.0f;

  // Timestamps
  TimePoint mCreated;
  TimePoint mSeen;
  TimePoint mSeenLatLon;

  // Message count
  long mMessageCount = 0;

  // Position history
  std::deque<PositionRecord> mPositionHistory;

  // CPR state for position decoding
  CprState mCprState;

  // Flags
  AircraftFlags mFlags = AircraftFlags::None;
};

// Inline implementations
inline void
Aircraft::setFlight(const char* aFlight) {
  if (aFlight) {
    size_t len = std::min(strlen(aFlight), mFlight.size() - 1);
    std::copy(aFlight, aFlight + len, mFlight.begin());
    mFlight[len] = '\0';
  }
}

inline void
Aircraft::setFlight(std::string_view aFlight) {
  size_t len = std::min(aFlight.size(), mFlight.size() - 1);
  std::copy(aFlight.begin(), aFlight.begin() + len, mFlight.begin());
  mFlight[len] = '\0';
}

inline void
Aircraft::setPosition(double aLat, double aLon) {
  mPosition = Position{aLat, aLon};
}

inline void
Aircraft::setPosition(const Position& aPos) {
  mPosition = aPos;
}

inline std::optional<Position>
Aircraft::lastPosition() const {
  if (mPositionHistory.size() > 1) {
    return mPositionHistory[mPositionHistory.size() - 2].position;
  }
  return std::nullopt;
}

inline std::optional<float>
Aircraft::lastHeading() const {
  if (mPositionHistory.size() > 1) {
    return mPositionHistory[mPositionHistory.size() - 2].heading;
  }
  return std::nullopt;
}

inline void
Aircraft::addSignalLevel(SignalLevel aLevel) {
  mSignalLevels[mSignalIndex] = aLevel;
  mSignalIndex = (mSignalIndex + 1) % kSignalHistorySize;
}

inline float
Aircraft::averageSignalLevel() const {
  int sum = 0;
  for (auto level : mSignalLevels) {
    sum += level;
  }
  return static_cast<float>(sum) / kSignalHistorySize;
}

inline void
Aircraft::updateSeen() {
  mSeen = Clock::now();
}

inline void
Aircraft::updateSeenLatLon() {
  mSeenLatLon = Clock::now();
}

inline void
Aircraft::addPositionToHistory(const Position& aPos, float aHeading) {
  mPositionHistory.push_back({aPos, aHeading, Clock::now()});
  if (mPositionHistory.size() > kMaxHistorySize) {
    mPositionHistory.pop_front();
  }
}

inline void
Aircraft::trimHistory() {
  while (mPositionHistory.size() > kMaxHistorySize) {
    mPositionHistory.pop_front();
  }
}

inline bool
Aircraft::isStale(Seconds aTtl) const {
  auto elapsed = Clock::now() - mSeen;
  return elapsed > aTtl;
}

inline Milliseconds
Aircraft::timeSinceSeen() const {
  return std::chrono::duration_cast<Milliseconds>(Clock::now() - mSeen);
}

}  // namespace viz1090

#endif  // VIZ1090_AIRCRAFT_H
