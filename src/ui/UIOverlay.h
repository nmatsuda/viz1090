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

#include <functional>
#include <string>
#include <vector>

#include "ui/Button.h"
#include "ui/MenuPanel.h"
#include "ui/RenderContext.h"

class AppData;

namespace viz1090 {

/// UI overlay for status boxes, buttons, and menus
/// Renders HUD elements on top of the map view
class UIOverlay {
public:
  using FrameAllCallback = std::function<void()>;
  using ThemeSelectedCallback = std::function<void(const std::string&)>;
  using ThemeListProvider = std::function<std::vector<std::string>()>;
  using CurrentThemeProvider = std::function<std::string()>;

  UIOverlay();

  /// Draw status bar elements (status boxes and menu button)
  void draw(const RenderContext& ctx, const AppData& appData, float lastFrameTime,
            float centerLat, float centerLon, int mapLoadPercent);

  /// Draw the menu panel (should be called after other UI elements, before input feedback)
  void drawMenuPanel(const RenderContext& ctx);

  /// Draw a status box at the specified position
  /// Updates left/top to position for the next box
  void drawStatusBox(const RenderContext& ctx, int* left, int* top,
                     const std::string& label, const std::string& message,
                     SDL_Color color);

  /// Draw a button at the specified position (follows status box style)
  /// Updates left/top to position for the next element
  void drawButton(const RenderContext& ctx, int* left, int* top, Button& button);

  /// Draw a centered status box
  void drawCenteredStatusBox(const RenderContext& ctx,
                             const std::string& label, const std::string& message,
                             SDL_Color color);

  /// Handle click events - returns true if a UI element was clicked
  bool handleClick(int x, int y);

  /// Get the bounding rectangles of status bar elements
  /// Returns rects in screen coordinates for collision avoidance
  struct StatusBarBounds {
    int topY{0};      // Y coordinate of top of highest row (smallest Y value)
    int bottomY{0};   // Y coordinate of bottom of lowest row (largest Y value, typically screen bottom)
    int rightX{0};    // X coordinate of rightmost element in bottom row
  };
  [[nodiscard]] StatusBarBounds calculateStatusBarBounds(const RenderContext& ctx,
                                                          const AppData& appData,
                                                          int mapLoadPercent) const;

  // Configuration
  void setShowFps(bool show) { showFps = show; }
  [[nodiscard]] bool getShowFps() const { return showFps; }

  /// Set callback for frame-all button
  void setFrameAllCallback(FrameAllCallback callback);

  /// Set up theme selection support
  void setThemeSupport(ThemeListProvider listProvider,
                       CurrentThemeProvider currentProvider,
                       ThemeSelectedCallback selectedCallback);

private:
  bool showFps{false};
  Button menuButton_;
  MenuPanel menuPanel_;
  FrameAllCallback frameAllCallback_;
};

}  // namespace viz1090

#endif  // UI_OVERLAY_H
