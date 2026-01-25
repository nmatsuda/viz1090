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

/// Info for a single off-map plane cluster (spatial only)
struct OffMapCluster {
  int count{0};
  float edgeX{0.0f};       // Screen edge position X (relative to center)
  float edgeY{0.0f};       // Screen edge position Y (relative to center)
  float dirX{0.0f};        // Normalized direction X from center
  float dirY{0.0f};        // Normalized direction Y from center
  float avgDistance{0.0f}; // Average distance from screen center (in pixels)
  SDL_Color color{0, 0, 0, 255};
  Aircraft* singleAircraft{nullptr};  // Set when count == 1, for label drawing
  std::unordered_set<uint32_t> memberAddrs;  // Aircraft addresses in this cluster
};

/// Info for a single on-map plane cluster (spatial only)
struct OnMapCluster {
  int count{0};
  float centerX{0.0f};  // Cluster center X (first plane position)
  float centerY{0.0f};  // Cluster center Y (first plane position)
  float avgHeading{0.0f};  // Average heading for icon drawing
  SDL_Color color{0, 0, 0, 255};
  Aircraft* singleAircraft{nullptr};  // Set when count == 1, for normal drawing
  std::unordered_set<uint32_t> memberAddrs;  // Aircraft addresses in this cluster
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
    uiBounds_.statusBarTopY = statusBarTopY;
    uiBounds_.statusBarRightX = statusBarRightX;
    labelConfig_.uiStatusBarTopY = statusBarTopY;
    labelConfig_.uiStatusBarRightX = statusBarRightX;
  }

  /// Set the scale bar bounds for off-map arrow avoidance
  void setScaleBarBounds(int scaleBarBottomY, int scaleBarRightX) {
    uiBounds_.scaleBarBottomY = scaleBarBottomY;
    uiBounds_.scaleBarRightX = scaleBarRightX;
  }

  /// Update screen dimensions in UI bounds
  void setScreenSize(int width, int height) {
    uiBounds_.screenWidth = width;
    uiBounds_.screenHeight = height;
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

  // Cluster state machine helpers
  void updateClusterStates(const RenderContext& ctx, const AircraftList& aircraftList);
  void drawClusterAnimations(const RenderContext& ctx, const AircraftList& aircraftList);
  bool findClusterCenter(const std::vector<OnMapCluster>& clusters, uint32_t memberAddr,
                         float& outX, float& outY) const;
  bool findOffMapClusterPosition(uint32_t memberAddr, float& outEdgeX, float& outEdgeY,
                                  float& outDirX, float& outDirY) const;
  bool isInMultiPlaneCluster(uint32_t addr) const;

  // Check if a screen position is off-map (outside screen bounds or in UI overlay region)
  bool isOffMap(int x, int y, int screenWidth, int screenHeight) const;

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

  // Previous frame cluster state (for cluster-mate stickiness)
  std::vector<OnMapCluster> prevOnMapClusters_;
  std::vector<OffMapCluster> prevOffMapClusters_;

  // Cluster timing constants
  static constexpr int HYSTERESIS_FRAMES = 30;        // Frames to wait before state transition
  static constexpr int ANIMATION_FRAMES = 8;          // Frames for merge/unmerge animation
  static constexpr float DISPLAY_ACTIVE = 30.0f;

  // UI overlay bounds for off-map arrow and isOffMap checks
  UIBounds uiBounds_;

  // Spatial grid for efficient label neighbor queries
  mutable ui::LabelSpatialGrid labelSpatialGrid_;
  mutable std::vector<const ui::LabelNeighbor*> nearbyNeighborsTemp_;
};

}  // namespace viz1090

#endif  // AIRCRAFT_RENDERER_H
