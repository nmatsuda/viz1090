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

#ifndef UI_OVERLAY_H
#define UI_OVERLAY_H

#include <SDL2/SDL.h>
#include <string>

#include "ui/RenderContext.h"

class AppData;

namespace viz1090 {

/// UI overlay for status boxes, buttons, and menus
/// Renders HUD elements on top of the map view
class UIOverlay {
public:
  UIOverlay() = default;

  /// Draw all UI overlay elements
  void draw(const RenderContext& ctx, const AppData& appData, float lastFrameTime,
            float centerLat, float centerLon, int mapLoadPercent);

  /// Draw a status box at the specified position
  /// Updates left/top to position for the next box
  void drawStatusBox(const RenderContext& ctx, int* left, int* top,
                     const std::string& label, const std::string& message,
                     SDL_Color color);

  /// Draw a centered status box
  void drawCenteredStatusBox(const RenderContext& ctx,
                             const std::string& label, const std::string& message,
                             SDL_Color color);

  // Configuration
  void setShowFps(bool show) { showFps = show; }
  [[nodiscard]] bool getShowFps() const { return showFps; }

private:
  bool showFps{false};
};

}  // namespace viz1090

#endif  // UI_OVERLAY_H
