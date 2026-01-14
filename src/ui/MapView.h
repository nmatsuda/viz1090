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

#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include <SDL2/SDL.h>
#include <chrono>

#include "ui/Map.h"
#include "ui/RenderContext.h"

namespace viz1090 {

// Lat/lon to distance multiplier (km per degree)
constexpr float LATLONMULT = 111.195f;  // 6371.0 * M_PI / 180.0

/// Handles map viewport, coordinate transformations, and geography rendering
class MapView {
public:
  MapView(bool& metric);

  /// Initialize the map texture for caching
  void initTexture(SDL_Renderer* renderer, int width, int height);

  // Configuration
  void setDrawCenterOrigin(bool draw) { drawCenterOrigin = draw; }
  [[nodiscard]] bool getDrawCenterOrigin() const { return drawCenterOrigin; }

  /// Draw the geography (map lines, place names)
  void drawGeography(const RenderContext& ctx);

  /// Draw scale bars
  void drawScaleBars(const RenderContext& ctx);

  /// Get the bounding area of the scale bars for collision avoidance
  struct ScaleBarBounds {
    int bottomY{0};  // Y coordinate of bottom of scale bar area
    int rightX{0};   // X coordinate of rightmost scale bar element
  };
  [[nodiscard]] ScaleBarBounds calculateScaleBarBounds(const RenderContext& ctx) const;

  /// Update viewport animation (call each frame)
  void update();

  // Coordinate conversion methods
  [[nodiscard]] int screenDist(float d, int screenWidth, int screenHeight) const;
  void pxFromLonLat(float* dx, float* dy, float lon, float lat) const;
  void latLonFromScreenCoords(float* lat, float* lon, int x, int y,
                              int screenWidth, int screenHeight) const;
  void screenCoords(int* outX, int* outY, float dx, float dy,
                    int screenWidth, int screenHeight) const;

  // Viewport control
  void moveCenterRelative(float dx, float dy, int screenWidth, int screenHeight);
  void moveCenterAbsolute(float x, float y, int screenWidth, int screenHeight);
  void animateCenterAbsolute(float x, float y, int screenWidth, int screenHeight);
  void animateCenterRelative(float dx, float dy, int screenWidth, int screenHeight);
  void animateZoomRelative(float factor);
  void setTarget(float lon, float lat);
  void setTargetZoom(float zoom);

  // State queries
  [[nodiscard]] bool isAnimating() const { return mapAnimating; }
  [[nodiscard]] bool needsRedraw() const { return mapRedraw; }
  [[nodiscard]] bool hasMoved() const { return mapMoved; }
  void setMoved() { mapMoved = 1; }

  // Viewport state
  float centerLon{0.0f};
  float centerLat{0.0f};
  float originLon{0.0f};
  float originLat{0.0f};
  bool originSet{false};  // True if --lat/--lon was specified
  float maxDist{25.0f};

  float mapTargetLon{0.0f};
  float mapTargetLat{0.0f};
  float mapTargetMaxDist{0.0f};

  bool& metric;

  // Map data
  Map map;

private:
  void drawLines(const RenderContext& ctx, int left, int top, int right, int bottom);
  void drawLinesRecursive(const RenderContext& ctx, QuadTree* tree,
                          float screen_lat_min, float screen_lat_max,
                          float screen_lon_min, float screen_lon_max, SDL_Color color);
  void drawPlaceNames(const RenderContext& ctx);
  void moveMapToTarget();
  void zoomMapToTarget();

  /// Draw point at center lon/lat
  void drawCenterOriginPoint(const RenderContext& ctx);

  // Cached map texture
  SDL_Texture* mapTexture{nullptr};

  // Texture state
  float currentLon{0.0f};
  float currentLat{0.0f};
  float currentMaxDist{0.0f};

  // State flags
  bool drawCenterOrigin{true};
  int mapMoved{1};
  int mapRedraw{1};
  int mapAnimating{0};
  bool highFramerate{false};

  std::chrono::high_resolution_clock::time_point lastRedraw;

  static constexpr int FRAMETIME = 33;
};

}  // namespace viz1090

#endif  // MAP_VIEW_H
