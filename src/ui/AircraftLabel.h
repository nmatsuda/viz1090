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

#ifndef AIRCRAFT_LABEL_H
#define AIRCRAFT_LABEL_H

#include "SDL2/SDL_ttf.h"
#include <chrono>
#include <string>
#include <vector>

#include "ui/Label.h"
#include "ui/LabelConfig.h"
#include "style/Style.h"

namespace viz1090 {
namespace ui {

/// Info about a neighboring label for physics calculations
/// This breaks the circular dependency - AircraftLabel doesn't need to know about AircraftList
struct LabelNeighbor {
  float x;           // Label X position
  float y;           // Label Y position
  float w;           // Label width
  float h;           // Label height
  int aircraftX;     // Aircraft screen X
  int aircraftY;     // Aircraft screen Y
  uint32_t addr;     // Aircraft address (to skip self)
};

/// Aircraft label with physics-based positioning
class AircraftLabel {
public:
  /// Constructor
  /// @param aircraftAddr The ICAO address of the aircraft this label belongs to
  /// @param metric Reference to metric/imperial preference
  /// @param screenWidth Screen width in pixels
  /// @param screenHeight Screen height in pixels
  /// @param font Font for label text
  /// @param style Theme style reference
  AircraftLabel(uint32_t aircraftAddr, bool& metric, int screenWidth, int screenHeight,
                TTF_Font* font, const Style& style);

  /// Update label text from aircraft data
  void update(const char* flight, int altitude, int speed);

  /// Clear accumulated acceleration (call before force calculation)
  void clearAcceleration();

  /// Calculate physics forces from neighboring labels (full list - O(n) per call)
  /// @param neighbors List of neighboring labels for collision detection
  /// @param config Label configuration (density multiplier, bounds, etc.)
  /// @param aircraftScreenX Aircraft's current screen X
  /// @param aircraftScreenY Aircraft's current screen Y
  void calculateForces(const std::vector<LabelNeighbor>& neighbors,
                       const LabelConfig& config,
                       int aircraftScreenX, int aircraftScreenY);

  /// Calculate physics forces from pre-filtered nearby neighbors (optimized path)
  /// @param nearbyNeighbors Pointers to only the nearby neighbors (from spatial grid)
  /// @param allNeighbors Full list for density calculation
  /// @param config Label configuration (density multiplier, bounds, etc.)
  /// @param aircraftScreenX Aircraft's current screen X
  /// @param aircraftScreenY Aircraft's current screen Y
  void calculateForcesFromNearby(const std::vector<const LabelNeighbor*>& nearbyNeighbors,
                                 const std::vector<LabelNeighbor>& allNeighbors,
                                 const LabelConfig& config,
                                 int aircraftScreenX, int aircraftScreenY);

  /// Apply accumulated forces using Verlet integration
  void applyForces();

  /// Move label by delta (for viewport panning)
  void move(float dx, float dy);

  /// Sync label position to aircraft's current screen position
  void syncToAircraftPosition(int aircraftScreenX, int aircraftScreenY);

  /// Snap label directly to nominal position near aircraft
  void resetToAircraftPosition(int aircraftScreenX, int aircraftScreenY);

  /// Check if label is currently animating/changing
  [[nodiscard]] bool getIsChanging() const { return isChanging; }

  /// Draw the label
  /// @param renderer SDL renderer
  /// @param selected Whether this aircraft is selected
  /// @param showLabels Whether labels are globally visible
  /// @param aircraftScreenX Aircraft's current screen X
  /// @param aircraftScreenY Aircraft's current screen Y
  void draw(SDL_Renderer* renderer, bool selected, bool showLabels,
            int aircraftScreenX, int aircraftScreenY);

  /// Force label to collapse (for cluster merge)
  void forceCollapse();

  /// Force label to expand (for cluster unmerge)
  void forceExpand();

  /// Get label bounds for collision detection
  [[nodiscard]] float getX() const { return x; }
  [[nodiscard]] float getY() const { return y; }
  [[nodiscard]] float getWidth() const { return w; }
  [[nodiscard]] float getHeight() const { return h; }

  /// Get aircraft address this label belongs to
  [[nodiscard]] uint32_t getAircraftAddr() const { return aircraftAddr_; }

private:
  SDL_Rect getFullRect(int labelLevel);
  float calculateDensity(const std::vector<LabelNeighbor>& neighbors, int labelLevel);
  float calculateDensityFromNearby(const std::vector<const LabelNeighbor*>& nearbyNeighbors, int labelLevel);

  uint32_t aircraftAddr_;

  Label flightLabel;
  Label altitudeLabel;
  Label speedLabel;
  Label debugLabel;

  float labelLevel;

  bool& metric;

  float x;
  float y;
  float w;
  float h;

  float target_w;
  float target_h;

  // Verlet integration: previous position (replaces velocity dx/dy)
  float prev_x;
  float prev_y;

  float ddx;  // acceleration
  float ddy;

  float opacity;
  float target_opacity;

  float pressure;

  int screen_width;
  int screen_height;

  bool isChanging;

  // Last known aircraft screen position (for detecting view changes)
  float lastAircraftX;
  float lastAircraftY;

  std::chrono::high_resolution_clock::time_point lastLevelChange;

  // Physics constants
  float label_force = 0.01f;
  float label_dist = 2.0f;
  float density_force = 0.01f;
  float attachment_force = 0.01f;
  float attachment_dist = 10.0f;
  float icon_force = 0.01f;
  float icon_dist = 15.0f;
  float boundary_force = 0.01f;
  float damping_force = 0.65f;
  float velocity_limit = 1.0f;
  float edge_margin = 15.0f;
  float drag_force = 0.00f;

  const Style& style;
};

}  // namespace ui
}  // namespace viz1090

// Backwards compatibility
using AircraftLabel = viz1090::ui::AircraftLabel;

#endif  // AIRCRAFT_LABEL_H
