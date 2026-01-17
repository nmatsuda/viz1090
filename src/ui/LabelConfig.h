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

#ifndef LABEL_CONFIG_H
#define LABEL_CONFIG_H

#include <SDL2/SDL.h>

namespace viz1090 {
namespace ui {

/// Configuration for aircraft label rendering and physics
/// Replaces static global state in AircraftLabel
struct LabelConfig {
  // Display settings
  float densityMultiplier{0.15f};  // Controls how aggressively labels are hidden
  bool showLabels{true};           // Global toggle for showing/hiding all labels

  // UI bounds for label avoidance
  int uiStatusBarTopY{0};          // Top Y coordinate of status bar area
  int uiStatusBarRightX{0};        // Right X coordinate of status bar area

  // Screen dimensions (needed for boundary calculations)
  int screenWidth{0};
  int screenHeight{0};

  // Flag to force immediate density recalculation
  bool densityChanged{false};

  /// Adjust density multiplier by delta, clamped to [0, 1]
  void adjustDensityMult(float delta) {
    densityMultiplier += delta;
    if (densityMultiplier < 0.0f) densityMultiplier = 0.0f;
    if (densityMultiplier > 1.0f) densityMultiplier = 1.0f;
    densityChanged = true;
  }

  /// Set density multiplier, clamped to [0, 1]
  void setDensityMult(float value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    densityMultiplier = value;
  }

  /// Clear the density changed flag
  void clearDensityChanged() { densityChanged = false; }
};

}  // namespace ui
}  // namespace viz1090

#endif  // LABEL_CONFIG_H
