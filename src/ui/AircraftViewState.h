// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
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

#ifndef AIRCRAFT_VIEW_STATE_H
#define AIRCRAFT_VIEW_STATE_H

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <SDL2/SDL.h>

namespace viz1090 {
namespace ui {

class AircraftLabel;

/// Cluster membership state machine
/// SOLO: Aircraft is not part of any multi-aircraft cluster
/// MERGE_PENDING: Aircraft should join cluster, waiting for hysteresis
/// CLUSTERED: Aircraft is part of a multi-aircraft cluster
/// UNMERGE_PENDING: Aircraft should leave cluster, waiting for hysteresis
enum class ClusterState {
  SOLO,
  MERGE_PENDING,
  CLUSTERED,
  UNMERGE_PENDING
};

/// Per-aircraft rendering state (screen coordinates, label, clustering, etc.)
/// This separates UI concerns from the domain Aircraft model
struct AircraftViewState {
  // Screen coordinates (computed from lat/lon each frame)
  int screenX{0};
  int screenY{0};

  // Label (created on first render)
  std::unique_ptr<AircraftLabel> label;

  // Cluster state machine
  ClusterState clusterState{ClusterState::SOLO};
  uint32_t clusterAnchorAddr{0};  // Address of a cluster-mate (for cluster identity)
  int stateFrameCount{0};         // Frames in current state (for hysteresis)
  float animProgress{0.0f};       // 0.0-1.0 for merge/unmerge animations
  float heading{0.0f};            // Cached heading for animation
  SDL_Color color{255, 255, 255, 255};  // Cached color for animation
  bool isOffMap{false};           // Whether this aircraft is off-screen

  AircraftViewState() = default;
  ~AircraftViewState();

  // Movable but not copyable (due to unique_ptr)
  AircraftViewState(const AircraftViewState&) = delete;
  AircraftViewState& operator=(const AircraftViewState&) = delete;
  AircraftViewState(AircraftViewState&&) = default;
  AircraftViewState& operator=(AircraftViewState&&) = default;
};

/// Maps aircraft ICAO address to view state
/// Owned by AircraftRenderer, provides screen position and label for each aircraft
class AircraftViewStateMap {
public:
  /// Get or create view state for an aircraft
  AircraftViewState& getOrCreate(uint32_t addr);

  /// Get view state (returns nullptr if not found)
  AircraftViewState* get(uint32_t addr);
  const AircraftViewState* get(uint32_t addr) const;

  /// Remove view state for aircraft that no longer exist
  void removeStale(const std::unordered_map<uint32_t, bool>& activeAddrs);

  /// Clear all view states
  void clear();

  /// Iteration support
  auto begin() { return states_.begin(); }
  auto end() { return states_.end(); }
  auto begin() const { return states_.begin(); }
  auto end() const { return states_.end(); }

private:
  std::unordered_map<uint32_t, AircraftViewState> states_;
};

}  // namespace ui
}  // namespace viz1090

#endif  // AIRCRAFT_VIEW_STATE_H
