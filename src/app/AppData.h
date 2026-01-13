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

#ifndef APPDATA_H
#define APPDATA_H

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include "core/AircraftList.h"
#include "viz1090/network/ConnectionManager.h"

/// Application data and network management
///
/// Manages the connection to dump1090 and aircraft state.
class AppData {
public:
  AppData();
  ~AppData();

  // Non-copyable
  AppData(const AppData&) = delete;
  AppData& operator=(const AppData&) = delete;

  /// Initialize internal state
  void initialize();

  /// Start connection to server
  void connect();

  /// Disconnect from server
  void disconnect();

  /// Process pending updates (call each frame)
  void update();

  /// Check if connected
  [[nodiscard]] bool isConnected() const;

  // Configuration (set before connect())
  std::string server = "127.0.0.1";
  uint16_t port = 30005;
  double userLat = 0.0;
  double userLon = 0.0;

  // Aircraft list (thread-safe access via lockAircraftList())
  AircraftList aircraftList;

  /// Lock the aircraft list for thread-safe iteration
  /// Returns a lock guard that releases when destroyed
  [[nodiscard]] std::unique_lock<std::mutex> lockAircraftList() {
    return std::unique_lock<std::mutex>(mMessageMutex);
  }

  // Statistics
  int numVisiblePlanes = 0;
  int numPlanes = 0;
  double maxDist = 0.0;
  double avgSig = 0.0;
  double msgRate = 0.0;

  // For backwards compatibility with View
  [[nodiscard]] bool connected() const { return isConnected(); }

private:
  void handleMessage(const viz1090::ModesMessage& aMsg);
  void updateStatus();
  void removeStaleAircraft();

  std::unique_ptr<viz1090::network::ConnectionManager> mConnectionManager;
  std::mutex mMessageMutex;

  // Timing for stale aircraft removal
  std::chrono::steady_clock::time_point mLastCleanup;
  static constexpr std::chrono::seconds kCleanupInterval{1};
  static constexpr std::chrono::seconds kAircraftTtl{60};
};

#endif
