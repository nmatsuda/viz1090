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

#ifndef INPUT_FEEDBACK_H
#define INPUT_FEEDBACK_H

#include <chrono>

#include "core/AircraftList.h"
#include "ui/RenderContext.h"

class Aircraft;

namespace viz1090 {

class MapView;

/// Handles visual feedback for user input (clicks, mouse, selection)
class InputFeedback {
public:
  InputFeedback() = default;

  /// Draw all input feedback (click ripple, selection brackets)
  void draw(const RenderContext& ctx, Aircraft* selectedAircraft);

  /// Register a click event
  void registerClick(int tapcount, int x, int y, AircraftList& aircraftList,
                     Aircraft** selectedAircraft, MapView& mapView);

  /// Register mouse movement
  void registerMouseMove(int x, int y);

  /// Check if high framerate is needed for animations
  [[nodiscard]] bool needsHighFramerate() const { return highFramerate; }
  void resetHighFramerate() { highFramerate = false; }

private:
  void drawClickRipple(const RenderContext& ctx);
  void drawSelectionBrackets(const RenderContext& ctx, Aircraft* selectedAircraft);
  void drawMouse(const RenderContext& ctx);

  // Click state
  int clickx{0};
  int clicky{0};
  std::chrono::high_resolution_clock::time_point clickTime;

  // Mouse state
  int mousex{0};
  int mousey{0};
  bool mouseActive{false};  // True when mouse is actively moving
  bool mouseWasActive{false};  // Previous frame's active state (for edge detection)
  std::chrono::high_resolution_clock::time_point mouseLastMoveTime;  // Last movement time
  std::chrono::high_resolution_clock::time_point mouseFadeStartTime;  // When fade animation started
  float mouseOpacity{0.0f};  // Current opacity (0.0 to 1.0)

  bool highFramerate{false};
};

}  // namespace viz1090

#endif  // INPUT_FEEDBACK_H
