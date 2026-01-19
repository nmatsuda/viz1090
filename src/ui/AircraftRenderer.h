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

#ifndef AIRCRAFT_RENDERER_H
#define AIRCRAFT_RENDERER_H

#include <SDL2/SDL.h>

#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/AircraftList.h"
#include "ui/AircraftLabel.h"
#include "ui/AircraftViewState.h"
#include "ui/LabelConfig.h"
#include "ui/LabelSpatialGrid.h"
#include "ui/RenderContext.h"

namespace viz1090 {

class MapView;

// Use same time_point type as MathUtils.h for compatibility with now()/elapsed()
using ClusterTimePoint = std::chrono::high_resolution_clock::time_point;

/// Per-aircraft cluster animation state
struct ClusterMemberState {
  uint32_t aircraftAddr{0};       // The aircraft being animated
  uint32_t clusterAnchorAddr{0};  // An aircraft to use as cluster position reference (0 if none)
  float heading{0.0f};            // Heading for icon drawing during animation
  SDL_Color color{255, 255, 255, 255};
  ClusterTimePoint animStartTime; // When this aircraft started its merge/unmerge animation
  bool isMerging{true};           // true = merging into cluster, false = unmerging out
};

/// Per-aircraft cluster membership tracking (for hysteresis)
struct ClusterMembershipState {
  ClusterTimePoint lastStateChange;  // When aircraft last merged or unmerged
  bool inCluster{false};             // Currently in a multi-plane cluster
};

/// Info for a single off-map plane cluster (greedy distance-based)
struct OffMapCluster {
  int count{0};
  float edgeX{0.0f};       // Screen edge position X (relative to center)
  float edgeY{0.0f};       // Screen edge position Y (relative to center)
  float dirX{0.0f};        // Normalized direction X from center
  float dirY{0.0f};        // Normalized direction Y from center
  float avgDistance{0.0f}; // Average distance from screen center (in pixels)
  SDL_Color color{0, 0, 0, 255};
  Aircraft* singleAircraft{nullptr};  // Set when count == 1, for label drawing

  // Animation state
  ClusterTimePoint lastStateChange;             // When cluster count last changed
  std::unordered_set<uint32_t> memberAddrs;     // Aircraft addresses currently in this cluster
};

/// Info for a single on-map plane cluster (greedy distance-based)
struct OnMapCluster {
  int count{0};
  float centerX{0.0f};  // Cluster center X (first plane position)
  float centerY{0.0f};  // Cluster center Y (first plane position)
  float avgHeading{0.0f};  // Average heading for icon drawing
  SDL_Color color{0, 0, 0, 255};
  Aircraft* singleAircraft{nullptr};  // Set when count == 1, for normal drawing

  // Animation state
  ClusterTimePoint lastStateChange;             // When cluster count last changed
  std::unordered_set<uint32_t> memberAddrs;     // Aircraft addresses currently in this cluster
};

/// Renders aircraft icons, trails, and labels
/// Manages per-aircraft view state (screen coordinates, labels)
class AircraftRenderer {
public:
  AircraftRenderer() = default;

  /// Draw all aircraft (icons, trails, labels)
  void draw(const RenderContext& ctx, AircraftList& aircraftList,
            Aircraft* selectedAircraft, MapView& mapView);

  /// Draw aircraft position trails
  void drawTrails(const RenderContext& ctx, const AircraftList& aircraftList,
                  const MapView& mapView, int left, int top, int right, int bottom);

  /// Resolve label conflicts using physics simulation
  void resolveLabelConflicts(AircraftList& aircraftList);

  /// Move all labels by offset (for viewport panning)
  void moveLabels(float dx, float dy);

  /// Sync all labels to their aircraft's current screen position
  void syncLabelsToAircraft(AircraftList& aircraftList);

  // Configuration
  void setMetric(bool* metric) { this->metric = metric; }

  /// Get screen coordinates for an aircraft (returns false if not found)
  bool getScreenCoords(uint32_t addr, int& outX, int& outY) const;

  /// Set the UI overlay bounds for off-map arrow and label avoidance
  void setUIBounds(int statusBarTopY, int statusBarRightX) {
    uiStatusBarTopY_ = statusBarTopY;
    uiStatusBarRightX_ = statusBarRightX;
    labelConfig_.uiStatusBarTopY = statusBarTopY;
    labelConfig_.uiStatusBarRightX = statusBarRightX;
  }

  /// Set the scale bar bounds for off-map arrow avoidance
  void setScaleBarBounds(int scaleBarBottomY, int scaleBarRightX) {
    scaleBarBottomY_ = scaleBarBottomY;
    scaleBarRightX_ = scaleBarRightX;
  }

  // Label configuration access
  ui::LabelConfig& labelConfig() { return labelConfig_; }
  const ui::LabelConfig& labelConfig() const { return labelConfig_; }

  // Check if any animation needs high framerate
  [[nodiscard]] bool needsHighFramerate() const { return highFramerate; }
  void resetHighFramerate() { highFramerate = false; }

private:
  void drawPlaneIcon(const RenderContext& ctx, int x, int y, float heading,
                     SDL_Color planeColor);
  void drawPlaneText(const RenderContext& ctx, Aircraft* p, Aircraft* selectedAircraft);

  // View state management
  ui::AircraftViewState& getOrCreateViewState(const RenderContext& ctx, Aircraft* p);
  void cleanupStaleViewStates(const AircraftList& aircraftList);

  // Off-map clustering helpers (greedy distance-based)
  void clearOffMapClusters(const RenderContext& ctx);
  void addToOffMapCluster(const RenderContext& ctx, int x, int y, SDL_Color planeColor,
                          Aircraft* aircraft);
  void drawOffMapClusterArrows(const RenderContext& ctx);
  void drawOffMapClusterLabels(const RenderContext& ctx, Aircraft* selectedAircraft);
  void drawOffMapArrow(const RenderContext& ctx, float edgeX, float edgeY,
                       float dirX, float dirY, SDL_Color planeColor, int count);

  // On-map clustering helpers (greedy distance-based)
  void clearOnMapClusters(const RenderContext& ctx);
  void addToOnMapCluster(const RenderContext& ctx, int x, int y, float heading,
                         SDL_Color planeColor, Aircraft* aircraft);
  void drawOnMapClusterIcons(const RenderContext& ctx);
  void drawOnMapClusterLabels(const RenderContext& ctx, Aircraft* selectedAircraft);

  // Cluster animation helpers
  void detectOnMapUnmergeEvents(const RenderContext& ctx, const AircraftList& aircraftList);
  void detectOffMapUnmergeEvents(const RenderContext& ctx, const AircraftList& aircraftList);
  void drawAnimatingClusterMembers(const RenderContext& ctx,
                                   std::unordered_map<uint32_t, ClusterMemberState>& animStates,
                                   const AircraftList& aircraftList);
  void cleanupFinishedAnimations(std::unordered_map<uint32_t, ClusterMemberState>& animStates);
  float getAnimProgress(ClusterTimePoint startTime) const;
  Aircraft* findAircraftByAddr(const AircraftList& aircraftList, uint32_t addr) const;
  bool findClusterCenter(const std::vector<OnMapCluster>& clusters, uint32_t memberAddr,
                         float& outX, float& outY) const;
  bool isInMultiPlaneCluster(uint32_t addr) const;

  // Build neighbor list for label physics
  std::vector<ui::LabelNeighbor> buildNeighborList(const AircraftList& aircraftList) const;

  bool* metric{nullptr};
  bool highFramerate{false};

  // Per-aircraft view state (screen coordinates, labels)
  ui::AircraftViewStateMap viewStates_;

  // Label configuration (replaces static state)
  ui::LabelConfig labelConfig_;

  // Off-map plane clusters - greedy distance-based
  std::vector<OffMapCluster> offMapClusters_;
  float offMapClusterRadius_{0.0f};  // Angular clustering threshold (in screen edge coords)

  // On-map plane clusters - greedy distance-based
  std::vector<OnMapCluster> onMapClusters_;
  float clusterRadius_{0.0f};  // Distance threshold for clustering

  // Previous frame cluster state (for detecting unmerge events)
  std::vector<OnMapCluster> prevOnMapClusters_;
  std::vector<OffMapCluster> prevOffMapClusters_;

  // Per-aircraft animation state (keyed by aircraft address)
  std::unordered_map<uint32_t, ClusterMemberState> onMapAnimStates_;
  std::unordered_map<uint32_t, ClusterMemberState> offMapAnimStates_;

  // Per-aircraft cluster membership tracking (for hysteresis)
  std::unordered_map<uint32_t, ClusterMembershipState> onMapMembership_;
  std::unordered_map<uint32_t, ClusterMembershipState> offMapMembership_;

  // Animation timing constants
  static constexpr float CLUSTER_ANIM_DURATION_MS = 250.0f;
  static constexpr float CLUSTER_HYSTERESIS_MS = 1000.0f;  // Min time between merge/unmerge
  static constexpr float DISPLAY_ACTIVE = 30.0f;

  // UI overlay bounds for off-map arrow avoidance
  int uiStatusBarTopY_{0};     // Top Y coordinate of status bar (0 = use screen edge)
  int uiStatusBarRightX_{0};   // Right X coordinate of bottom row elements

  // Scale bar bounds for off-map arrow avoidance
  int scaleBarBottomY_{0};     // Bottom Y coordinate of scale bar area
  int scaleBarRightX_{0};      // Right X coordinate of scale bar elements

  // Spatial grid for efficient label neighbor queries
  mutable ui::LabelSpatialGrid labelSpatialGrid_;
  mutable std::vector<const ui::LabelNeighbor*> nearbyNeighborsTemp_;
};

}  // namespace viz1090

#endif  // AIRCRAFT_RENDERER_H
