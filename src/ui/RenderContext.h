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

#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cstring>
#include <string>

#include "viz1090/StyleManager.h"

namespace viz1090 {

/// Font with associated dimensions
struct FontInfo {
  TTF_Font* font{nullptr};
  int width{0};
  int height{0};
};

/// Collection of fonts used for rendering
struct FontSet {
  FontInfo map;        // Map place names
  FontInfo mapBold;    // Map bold text
  FontInfo label;      // Aircraft labels
  FontInfo message;    // Status messages
  FontInfo list;       // Aircraft list
};

/// UI overlay bounds for avoidance calculations
/// Used by off-map arrows and label positioning
struct UIBounds {
  // Status bar (bottom of screen)
  int statusBarTopY{0};     // Top Y coordinate of status bar area (0 = use screen edge)
  int statusBarRightX{0};   // Right X coordinate of status bar elements

  // Scale bar (top of screen)
  int scaleBarBottomY{0};   // Bottom Y coordinate of scale bar area
  int scaleBarRightX{0};    // Right X coordinate of scale bar elements

  // Screen dimensions
  int screenWidth{0};
  int screenHeight{0};

  /// Check if a point is in the status bar region
  [[nodiscard]] bool isInStatusBar(int x, int y) const {
    return statusBarTopY > 0 && y > statusBarTopY && x < statusBarRightX;
  }

  /// Check if a point is in the scale bar region
  [[nodiscard]] bool isInScaleBar(int x, int y) const {
    return scaleBarBottomY > 0 && y < scaleBarBottomY && x < scaleBarRightX;
  }

  /// Check if a point is off-map (outside screen or in UI overlay regions)
  [[nodiscard]] bool isOffMap(int x, int y) const {
    if (x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) {
      return true;
    }
    return isInStatusBar(x, y) || isInScaleBar(x, y);
  }
};

/// Shared rendering context passed to all drawing components
/// Bundles commonly needed rendering state to avoid passing many parameters
struct RenderContext {
  // SDL rendering
  SDL_Renderer* renderer{nullptr};

  // Screen dimensions
  int screenWidth{0};
  int screenHeight{0};
  int uiScale{1};

  // Theme reference for consistent theming
  const Theme* style{nullptr};

  // Fonts (pointer to FontSet owned by View)
  const FontSet* fonts{nullptr};

  // Helper accessors
  [[nodiscard]] int padding() const { return 5; }
  [[nodiscard]] int cornerRadius() const { return 3; }

  // Convenience font accessors (for compatibility during transition)
  [[nodiscard]] TTF_Font* mapFont() const { return fonts ? fonts->map.font : nullptr; }
  [[nodiscard]] TTF_Font* mapBoldFont() const { return fonts ? fonts->mapBold.font : nullptr; }
  [[nodiscard]] TTF_Font* labelFont() const { return fonts ? fonts->label.font : nullptr; }
  [[nodiscard]] TTF_Font* messageFont() const { return fonts ? fonts->message.font : nullptr; }
  [[nodiscard]] TTF_Font* listFont() const { return fonts ? fonts->list.font : nullptr; }
  [[nodiscard]] int mapFontWidth() const { return fonts ? fonts->map.width : 5; }
  [[nodiscard]] int mapFontHeight() const { return fonts ? fonts->map.height : 12; }
  [[nodiscard]] int labelFontWidth() const { return fonts ? fonts->label.width : 6; }
  [[nodiscard]] int labelFontHeight() const { return fonts ? fonts->label.height : 12; }
  [[nodiscard]] int messageFontWidth() const { return fonts ? fonts->message.width : 6; }
  [[nodiscard]] int messageFontHeight() const { return fonts ? fonts->message.height : 12; }

  // Text width helpers (includes padding for button-style labels)
  [[nodiscard]] int labelTextWidth(const std::string& text) const {
    return static_cast<int>((text.length() + 1) * labelFontWidth());
  }
  [[nodiscard]] int labelTextWidth(const char* text) const {
    return static_cast<int>((std::strlen(text) + 1) * labelFontWidth());
  }
  [[nodiscard]] int messageTextWidth(const std::string& text) const {
    return static_cast<int>((text.length() + 1) * messageFontWidth());
  }
  [[nodiscard]] int messageTextWidth(const char* text) const {
    return static_cast<int>((std::strlen(text) + 1) * messageFontWidth());
  }
  [[nodiscard]] int mapTextWidth(const char* text) const {
    return static_cast<int>(std::strlen(text) * mapFontWidth());
  }
};

}  // namespace viz1090

#endif  // RENDER_CONTEXT_H
