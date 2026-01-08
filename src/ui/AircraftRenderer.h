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

#include <vector>

#include "core/AircraftList.h"
#include "ui/RenderContext.h"

class Aircraft;

namespace viz1090 {

class MapView;

/// Info for a single off-map plane bucket
struct OffMapBucket {
  int count{0};
  float avgX{0.0f};  // Average screen X position of planes in bucket
  float avgY{0.0f};  // Average screen Y position of planes in bucket
  SDL_Color color{0, 0, 0, 255};
  Aircraft* singleAircraft{nullptr};  // Set when count == 1, for label drawing
};

/// Info for a single on-map plane cluster (greedy distance-based)
struct OnMapCluster {
  int count{0};
  float centerX{0.0f};  // Cluster center X (first plane position)
  float centerY{0.0f};  // Cluster center Y (first plane position)
  float avgHeading{0.0f};  // Average heading for icon drawing
  SDL_Color color{0, 0, 0, 255};
  Aircraft* singleAircraft{nullptr};  // Set when count == 1, for normal drawing
};

/// Renders aircraft icons, trails, and labels
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
  void moveLabels(AircraftList& aircraftList, float dx, float dy);

  // Configuration
  void setMetric(bool metric) { this->metric = metric; }
  [[nodiscard]] bool getMetric() const { return metric; }

  // Check if any animation needs high framerate
  [[nodiscard]] bool needsHighFramerate() const { return highFramerate; }
  void resetHighFramerate() { highFramerate = false; }

private:
  void drawPlaneIcon(const RenderContext& ctx, int x, int y, float heading,
                     SDL_Color planeColor);
  void drawPlaneText(const RenderContext& ctx, Aircraft* p, Aircraft* selectedAircraft);

  // Off-map bucketing helpers
  void clearOffMapBuckets();
  void addToOffMapBucket(const RenderContext& ctx, int x, int y, SDL_Color planeColor,
                         Aircraft* aircraft);
  void drawOffMapBuckets(const RenderContext& ctx, Aircraft* selectedAircraft);
  int calculateBucketIndex(const RenderContext& ctx, int x, int y) const;
  void drawOffMapArrow(const RenderContext& ctx, float edgeX, float edgeY,
                       float dirX, float dirY, SDL_Color planeColor, int count);

  // On-map clustering helpers (greedy distance-based)
  void clearOnMapClusters(const RenderContext& ctx);
  void addToOnMapCluster(const RenderContext& ctx, int x, int y, float heading,
                         SDL_Color planeColor, Aircraft* aircraft);
  void drawOnMapClusters(const RenderContext& ctx, Aircraft* selectedAircraft);

  bool metric{false};
  bool highFramerate{false};

  // Off-map plane buckets - sized based on screen perimeter and arrow size
  std::vector<OffMapBucket> offMapBuckets_;
  int numBuckets_{0};
  float bucketAngularSize_{0.0f};

  // On-map plane clusters - greedy distance-based
  std::vector<OnMapCluster> onMapClusters_;
  float clusterRadius_{0.0f};  // Distance threshold for clustering

  static constexpr float DISPLAY_ACTIVE = 30.0f;
  static constexpr int MIN_BUCKETS = 16;
  static constexpr int MAX_BUCKETS = 64;
};

}  // namespace viz1090

#endif  // AIRCRAFT_RENDERER_H
