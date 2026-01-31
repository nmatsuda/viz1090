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

#include "ui/MapView.h"

#include <cmath>
#include <cstring>
#include <vector>

#include "SDL2/SDL2_gfxPrimitives.h"
#include "ui/Label.h"
#include "ui/MathUtils.h"

namespace viz1090 {

MapView::MapView(bool& metric) : metric{metric} {
  lastRedraw = now();
}

void MapView::initTexture(SDL_Renderer* renderer, int width, int height) {
  mapTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_TARGET, width, height);
}

int MapView::screenDist(float d, int screenWidth, int screenHeight) const {
  float scale_factor = (screenWidth > screenHeight) ? screenWidth : screenHeight;
  return static_cast<int>(std::round(scale_factor * 0.5f * std::fabs(d) / maxDist));
}

void MapView::pxFromLonLat(float* dx, float* dy, float lon, float lat) const {
  if (!lon || !lat) {
    *dx = 0;
    *dy = 0;
    return;
  }

  *dx = LATLONMULT * (lon - centerLon) * std::cos(((lat + centerLat) / 2.0f) * M_PI / 180.0f);
  *dy = LATLONMULT * (lat - centerLat);
}

void MapView::latLonFromScreenCoords(float* lat, float* lon, int x, int y,
                                     int screenWidth, int screenHeight) const {
  float scale_factor = (screenWidth > screenHeight) ? screenWidth : screenHeight;

  float dx = maxDist * (x - (screenWidth >> 1)) / (0.95f * scale_factor * 0.5f);
  float dy = maxDist * (y - (screenHeight >> 1)) / (0.95f * scale_factor * 0.5f);

  *lat = 180.0f * dy / (6371.0f * static_cast<float>(M_PI)) + centerLat;
  *lon = 180.0f * dx / (std::cos(((*lat + centerLat) / 2.0f) * static_cast<float>(M_PI) / 180.0f) *
                        6371.0f * static_cast<float>(M_PI)) +
         centerLon;
}

void MapView::screenCoords(int* outX, int* outY, float dx, float dy,
                           int screenWidth, int screenHeight) const {
  *outX = (screenWidth >> 1) + ((dx > 0) ? 1 : -1) * screenDist(dx, screenWidth, screenHeight);
  *outY = (screenHeight >> 1) + ((dy > 0) ? -1 : 1) * screenDist(dy, screenWidth, screenHeight);
}

void MapView::update() {
  moveMapToTarget();
  zoomMapToTarget();
}

void MapView::moveMapToTarget() {
  if (mapTargetLon && mapTargetLat) {
    if (std::fabs(mapTargetLon - centerLon) > 0.0001f ||
        std::fabs(mapTargetLat - centerLat) > 0.0001f) {
      centerLon += 0.1f * (mapTargetLon - centerLon);
      centerLat += 0.1f * (mapTargetLat - centerLat);

      renderState_ = MapRenderState::VIEWPORT_DIRTY;
      highFramerate = true;
    } else {
      mapTargetLon = 0;
      mapTargetLat = 0;
    }
  }
}

void MapView::zoomMapToTarget() {
  if (mapTargetMaxDist) {
    if (std::fabs(mapTargetMaxDist - maxDist) > 0.0001f) {
      maxDist += 0.1f * (mapTargetMaxDist - maxDist);
      renderState_ = MapRenderState::VIEWPORT_DIRTY;
      highFramerate = true;
    } else {
      mapTargetMaxDist = 0;
    }
  }
}

void MapView::moveCenterRelative(float dx, float dy, int screenWidth, int screenHeight) {
  float scale_factor = (screenWidth > screenHeight) ? screenWidth : screenHeight;

  dx = -1.0f * dx * maxDist / (0.95f * scale_factor * 0.5f);
  dy = 1.0f * dy * maxDist / (0.95f * scale_factor * 0.5f);

  float outLat = dy * (1.0f / 6371.0f) * (180.0f / static_cast<float>(M_PI));
  float outLon = dx * (1.0f / 6371.0f) * (180.0f / static_cast<float>(M_PI)) /
                 std::cos((centerLat / 2.0f) * static_cast<float>(M_PI) / 180.0f);

  centerLon += outLon;
  centerLat += outLat;

  mapTargetLon = 0;
  mapTargetLat = 0;

  renderState_ = MapRenderState::VIEWPORT_DIRTY;
  highFramerate = true;
}

void MapView::moveCenterAbsolute(float x, float y, int screenWidth, int screenHeight) {
  float scale_factor = (screenWidth > screenHeight) ? screenWidth : screenHeight;

  float dx = -1.0f * (0.75f * static_cast<float>(screenWidth) / static_cast<float>(screenHeight)) *
             (x - screenWidth / 2) * maxDist / (0.95f * scale_factor * 0.5f);
  float dy = 1.0f * (y - screenHeight / 2) * maxDist / (0.95f * scale_factor * 0.5f);

  float outLat = dy * (1.0f / 6371.0f) * (180.0f / static_cast<float>(M_PI));
  float outLon = dx * (1.0f / 6371.0f) * (180.0f / static_cast<float>(M_PI)) /
                 std::cos((centerLat / 2.0f) * static_cast<float>(M_PI) / 180.0f);

  centerLon += outLon;
  centerLat += outLat;

  mapTargetLon = 0;
  mapTargetLat = 0;

  renderState_ = MapRenderState::VIEWPORT_DIRTY;
  highFramerate = true;
}

void MapView::animateCenterAbsolute(float x, float y, int screenWidth, int screenHeight) {
  float scale_factor = (screenWidth > screenHeight) ? screenWidth : screenHeight;

  float dx = -1.0f * (0.75f * static_cast<float>(screenWidth) / static_cast<float>(screenHeight)) *
             (x - screenWidth / 2) * maxDist / (0.95f * scale_factor * 0.5f);
  float dy = 1.0f * (y - screenHeight / 2) * maxDist / (0.95f * scale_factor * 0.5f);

  float outLat = dy * (1.0f / 6371.0f) * (180.0f / static_cast<float>(M_PI));
  float outLon = dx * (1.0f / 6371.0f) * (180.0f / static_cast<float>(M_PI)) /
                 std::cos((centerLat / 2.0f) * static_cast<float>(M_PI) / 180.0f);

  mapTargetLon = centerLon - outLon;
  mapTargetLat = centerLat - outLat;

  mapTargetMaxDist = 0.25f * maxDist;

  renderState_ = MapRenderState::VIEWPORT_DIRTY;
  highFramerate = true;
}

void MapView::setTarget(float lon, float lat) {
  mapTargetLon = lon;
  mapTargetLat = lat;
}

void MapView::setTargetZoom(float zoom) {
  mapTargetMaxDist = zoom;
}

void MapView::animateCenterRelative(float dx, float dy, int screenWidth, int screenHeight) {
  float scale_factor = (screenWidth > screenHeight) ? screenWidth : screenHeight;

  // Convert screen pixels to lat/lon delta
  float dxKm = -1.0f * dx * maxDist / (0.95f * scale_factor * 0.5f);
  float dyKm = 1.0f * dy * maxDist / (0.95f * scale_factor * 0.5f);

  float deltaLat = dyKm * (1.0f / 6371.0f) * (180.0f / static_cast<float>(M_PI));
  float deltaLon = dxKm * (1.0f / 6371.0f) * (180.0f / static_cast<float>(M_PI)) /
                   std::cos((centerLat / 2.0f) * static_cast<float>(M_PI) / 180.0f);

  // If already animating, add to current target; otherwise start from current position
  if (mapTargetLon != 0.0f || mapTargetLat != 0.0f) {
    mapTargetLon += deltaLon;
    mapTargetLat += deltaLat;
  } else {
    mapTargetLon = centerLon + deltaLon;
    mapTargetLat = centerLat + deltaLat;
  }

  renderState_ = MapRenderState::VIEWPORT_DIRTY;
  highFramerate = true;
}

void MapView::animateZoomRelative(float factor) {
  // Calculate new target zoom level
  float newMaxDist = maxDist * factor;
  if (newMaxDist < 0.001f) {
    newMaxDist = 0.001f;
  }

  // If already animating zoom, multiply the target; otherwise start from current
  if (mapTargetMaxDist != 0.0f) {
    mapTargetMaxDist *= factor;
    if (mapTargetMaxDist < 0.001f) {
      mapTargetMaxDist = 0.001f;
    }
  } else {
    mapTargetMaxDist = newMaxDist;
  }

  renderState_ = MapRenderState::VIEWPORT_DIRTY;
  highFramerate = true;
}

void MapView::drawGeography(const RenderContext& ctx) {
  // Determine if we need a full texture redraw
  bool needsTextureRedraw =
      (renderState_ == MapRenderState::TEXTURE_DIRTY) ||
      (isAnimating() && elapsed(lastRedraw) > 8 * FRAMETIME) ||
      (elapsed(lastRedraw) > 2000) ||
      (map.loaded < 100 && elapsed(lastRedraw) > 250);

  if (needsTextureRedraw) {
    SDL_SetRenderTarget(ctx.renderer, mapTexture);

    SDL_SetRenderDrawColor(ctx.renderer, ctx.style->backgroundColor.r,
                           ctx.style->backgroundColor.g, ctx.style->backgroundColor.b, 255);

    SDL_RenderClear(ctx.renderer);

    drawLines(ctx, 0, 0, ctx.screenWidth, ctx.screenHeight);
    drawPlaceNames(ctx);

    SDL_SetRenderTarget(ctx.renderer, NULL);

    renderState_ = MapRenderState::CLEAN;
    lastRedraw = now();

    currentLon = centerLon;
    currentLat = centerLat;
    currentMaxDist = maxDist;
  }

  SDL_SetRenderDrawColor(ctx.renderer, ctx.style->backgroundColor.r,
                         ctx.style->backgroundColor.g, ctx.style->backgroundColor.b, 255);

  SDL_RenderClear(ctx.renderer);

  if (renderState_ == MapRenderState::VIEWPORT_DIRTY) {
    float dx, dy;
    int x1, y1, x2, y2;
    pxFromLonLat(&dx, &dy, currentLon, currentLat);
    screenCoords(&x1, &y1, dx, dy, ctx.screenWidth, ctx.screenHeight);
    pxFromLonLat(&dx, &dy, centerLon, centerLat);
    screenCoords(&x2, &y2, dx, dy, ctx.screenWidth, ctx.screenHeight);

    int shiftx = x1 - x2;
    int shifty = y1 - y2;

    SDL_Rect dest;

    dest.x = shiftx + (ctx.screenWidth / 2) * (1 - currentMaxDist / maxDist);
    dest.y = shifty + (ctx.screenHeight / 2) * (1 - currentMaxDist / maxDist);
    dest.w = static_cast<int>(ctx.screenWidth * currentMaxDist / maxDist);
    dest.h = static_cast<int>(ctx.screenHeight * currentMaxDist / maxDist);

    // left
    if (dest.x > 0) {
      drawLines(ctx, 0, 0, dest.x, ctx.screenHeight);
    }

    // top
    if (dest.y > 0) {
      drawLines(ctx, 0, ctx.screenHeight - dest.y, ctx.screenWidth, ctx.screenHeight);
    }

    // right
    if (dest.x + dest.w < ctx.screenWidth) {
      drawLines(ctx, dest.x + dest.w, 0, ctx.screenWidth, ctx.screenHeight);
    }

    // bottom
    if (dest.y + dest.h < ctx.screenHeight) {
      drawLines(ctx, 0, 0, ctx.screenWidth, ctx.screenHeight - dest.y - dest.h);
    }

    SDL_RenderCopy(ctx.renderer, mapTexture, NULL, &dest);

    renderState_ = MapRenderState::TEXTURE_DIRTY;
  } else {
    SDL_RenderCopy(ctx.renderer, mapTexture, NULL, NULL);
  }
}

void MapView::drawLines(const RenderContext& ctx, int left, int top, int right, int bottom) {
  float screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max;

  latLonFromScreenCoords(&screen_lat_min, &screen_lon_min, left, top,
                         ctx.screenWidth, ctx.screenHeight);
  latLonFromScreenCoords(&screen_lat_max, &screen_lon_max, right, bottom,
                         ctx.screenWidth, ctx.screenHeight);

  // Draw map lines (state/province boundaries) - collect into buffer then batch draw each segment
  lineBuffer_.clear();
  collectLinesRecursive(&(map.root), screen_lat_min, screen_lat_max, screen_lon_min,
                        screen_lon_max, ctx.screenWidth, ctx.screenHeight);
  if (!lineBuffer_.empty()) {
    SDL_SetRenderDrawColor(ctx.renderer, ctx.style->geoColor.r, ctx.style->geoColor.g,
                           ctx.style->geoColor.b, 255);
    // Draw each line segment (pairs of points)
    for (size_t i = 0; i + 1 < lineBuffer_.size(); i += 2) {
      SDL_RenderDrawLine(ctx.renderer,
                         lineBuffer_[i].x, lineBuffer_[i].y,
                         lineBuffer_[i + 1].x, lineBuffer_[i + 1].y);
    }
  }

  // Draw coastlines (land-sea boundaries)
  lineBuffer_.clear();
  collectLinesRecursive(&(map.coastline_root), screen_lat_min, screen_lat_max, screen_lon_min,
                        screen_lon_max, ctx.screenWidth, ctx.screenHeight);
  if (!lineBuffer_.empty()) {
    SDL_SetRenderDrawColor(ctx.renderer, ctx.style->coastlineColor.r, ctx.style->coastlineColor.g,
                           ctx.style->coastlineColor.b, 255);
    for (size_t i = 0; i + 1 < lineBuffer_.size(); i += 2) {
      SDL_RenderDrawLine(ctx.renderer,
                         lineBuffer_[i].x, lineBuffer_[i].y,
                         lineBuffer_[i + 1].x, lineBuffer_[i + 1].y);
    }
  }

  // Draw country boundary lines (national borders) - on top of other features
  lineBuffer_.clear();
  collectLinesRecursive(&(map.country_root), screen_lat_min, screen_lat_max, screen_lon_min,
                        screen_lon_max, ctx.screenWidth, ctx.screenHeight);
  if (!lineBuffer_.empty()) {
    SDL_SetRenderDrawColor(ctx.renderer, ctx.style->countryBorderColor.r,
                           ctx.style->countryBorderColor.g, ctx.style->countryBorderColor.b, 255);
    // Draw each line segment (pairs of points)
    for (size_t i = 0; i + 1 < lineBuffer_.size(); i += 2) {
      SDL_RenderDrawLine(ctx.renderer,
                         lineBuffer_[i].x, lineBuffer_[i].y,
                         lineBuffer_[i + 1].x, lineBuffer_[i + 1].y);
    }
  }

  // Draw airport lines - collect into buffer then batch draw each segment
  lineBuffer_.clear();
  collectLinesRecursive(&(map.airport_root), screen_lat_min, screen_lat_max, screen_lon_min,
                        screen_lon_max, ctx.screenWidth, ctx.screenHeight);
  if (!lineBuffer_.empty()) {
    SDL_SetRenderDrawColor(ctx.renderer, ctx.style->airportColor.r, ctx.style->airportColor.g,
                           ctx.style->airportColor.b, 255);
    // Draw each line segment (pairs of points)
    for (size_t i = 0; i + 1 < lineBuffer_.size(); i += 2) {
      SDL_RenderDrawLine(ctx.renderer,
                         lineBuffer_[i].x, lineBuffer_[i].y,
                         lineBuffer_[i + 1].x, lineBuffer_[i + 1].y);
    }
  }
}

void MapView::drawLinesRecursive(const RenderContext& ctx, QuadTree* tree,
                                 float screen_lat_min, float screen_lat_max,
                                 float screen_lon_min, float screen_lon_max, SDL_Color color) {
  if (tree == NULL) {
    return;
  }

  if (tree->lat_min > screen_lat_max || screen_lat_min > tree->lat_max) {
    return;
  }

  if (tree->lon_min > screen_lon_max || screen_lon_min > tree->lon_max) {
    return;
  }

  drawLinesRecursive(ctx, tree->nw, screen_lat_min, screen_lat_max, screen_lon_min,
                     screen_lon_max, color);
  drawLinesRecursive(ctx, tree->sw, screen_lat_min, screen_lat_max, screen_lon_min,
                     screen_lon_max, color);
  drawLinesRecursive(ctx, tree->ne, screen_lat_min, screen_lat_max, screen_lon_min,
                     screen_lon_max, color);
  drawLinesRecursive(ctx, tree->se, screen_lat_min, screen_lat_max, screen_lon_min,
                     screen_lon_max, color);

  for (auto currentLine = tree->lines.begin(); currentLine != tree->lines.end(); ++currentLine) {
    int x1, y1, x2, y2;
    float dx, dy;

    pxFromLonLat(&dx, &dy, (*currentLine)->start.lon, (*currentLine)->start.lat);
    screenCoords(&x1, &y1, dx, dy, ctx.screenWidth, ctx.screenHeight);

    pxFromLonLat(&dx, &dy, (*currentLine)->end.lon, (*currentLine)->end.lat);
    screenCoords(&x2, &y2, dx, dy, ctx.screenWidth, ctx.screenHeight);

    // Check if out of bounds
    if ((x1 < 0 || x1 >= ctx.screenWidth || y1 < 0 || y1 >= ctx.screenHeight) &&
        (x2 < 0 || x2 >= ctx.screenWidth || y2 < 0 || y2 >= ctx.screenHeight)) {
      continue;
    }

    if (x1 == x2 && y1 == y2) {
      continue;
    }

    lineRGBA(ctx.renderer, x1, y1, x2, y2, color.r, color.g, color.b, 255);
  }
}

void MapView::collectLinesRecursive(QuadTree* tree,
                                    float screen_lat_min, float screen_lat_max,
                                    float screen_lon_min, float screen_lon_max,
                                    int screenWidth, int screenHeight) {
  if (tree == NULL) {
    return;
  }

  // Quadtree bounding box culling
  if (tree->lat_min > screen_lat_max || screen_lat_min > tree->lat_max) {
    return;
  }

  if (tree->lon_min > screen_lon_max || screen_lon_min > tree->lon_max) {
    return;
  }

  // Recurse into children
  collectLinesRecursive(tree->nw, screen_lat_min, screen_lat_max, screen_lon_min,
                        screen_lon_max, screenWidth, screenHeight);
  collectLinesRecursive(tree->sw, screen_lat_min, screen_lat_max, screen_lon_min,
                        screen_lon_max, screenWidth, screenHeight);
  collectLinesRecursive(tree->ne, screen_lat_min, screen_lat_max, screen_lon_min,
                        screen_lon_max, screenWidth, screenHeight);
  collectLinesRecursive(tree->se, screen_lat_min, screen_lat_max, screen_lon_min,
                        screen_lon_max, screenWidth, screenHeight);

  // Collect lines from this node
  for (const auto& line : tree->lines) {
    int x1, y1, x2, y2;
    float dx, dy;

    pxFromLonLat(&dx, &dy, line->start.lon, line->start.lat);
    screenCoords(&x1, &y1, dx, dy, screenWidth, screenHeight);

    pxFromLonLat(&dx, &dy, line->end.lon, line->end.lat);
    screenCoords(&x2, &y2, dx, dy, screenWidth, screenHeight);

    // Skip if both endpoints are out of bounds
    bool p1OutOfBounds = (x1 < 0 || x1 >= screenWidth || y1 < 0 || y1 >= screenHeight);
    bool p2OutOfBounds = (x2 < 0 || x2 >= screenWidth || y2 < 0 || y2 >= screenHeight);
    if (p1OutOfBounds && p2OutOfBounds) {
      continue;
    }

    // Skip sub-pixel lines (both points map to same pixel)
    if (x1 == x2 && y1 == y2) {
      continue;
    }

    // Add line segment to buffer (two points per line)
    lineBuffer_.push_back({x1, y1});
    lineBuffer_.push_back({x2, y2});
  }
}

void MapView::drawPlaceNames(const RenderContext& ctx) {
  // Collect all visible labels with their screen positions and bounding boxes
  struct VisibleLabel {
    std::string text;
    int x, y;
    int width, height;
    Uint8 alpha{255};
  };

  std::vector<VisibleLabel> visibleLabels;
  visibleLabels.reserve(map.mapnames.size() + map.airportnames.size());

  // Estimate text dimensions based on font metrics
  int charWidth = ctx.mapFontWidth();
  int charHeight = ctx.mapFontHeight();

  // Collect map place names
  for (const auto& label : map.mapnames) {
    float dx, dy;
    int x, y;

    pxFromLonLat(&dx, &dy, label->location.lon, label->location.lat);
    screenCoords(&x, &y, dx, dy, ctx.screenWidth, ctx.screenHeight);

    if (x < 0 || x >= ctx.screenWidth || y < 0 || y >= ctx.screenHeight) {
      continue;
    }

    VisibleLabel vl;
    vl.text = label->text;
    vl.x = x;
    vl.y = y;
    vl.width = static_cast<int>(label->text.length()) * charWidth;
    vl.height = charHeight;
    visibleLabels.push_back(vl);
  }

  // Collect airport names
  for (const auto& label : map.airportnames) {
    float dx, dy;
    int x, y;

    pxFromLonLat(&dx, &dy, label->location.lon, label->location.lat);
    screenCoords(&x, &y, dx, dy, ctx.screenWidth, ctx.screenHeight);

    if (x < 0 || x >= ctx.screenWidth || y < 0 || y >= ctx.screenHeight) {
      continue;
    }

    VisibleLabel vl;
    vl.text = label->text;
    vl.x = x;
    vl.y = y;
    vl.width = static_cast<int>(label->text.length()) * charWidth;
    vl.height = charHeight;
    visibleLabels.push_back(vl);
  }

  // Detect overlaps and assign alpha values
  // Use greedy approach: first label stays visible, overlapping ones fade out
  // Add padding around labels for overlap detection
  int padding = charWidth;

  for (size_t i = 0; i < visibleLabels.size(); ++i) {
    auto& labelA = visibleLabels[i];

    // Skip already faded labels
    if (labelA.alpha == 0) {
      continue;
    }

    // Check against all subsequent labels
    for (size_t j = i + 1; j < visibleLabels.size(); ++j) {
      auto& labelB = visibleLabels[j];

      // Skip already faded labels
      if (labelB.alpha == 0) {
        continue;
      }

      // Check bounding box overlap with padding
      int aLeft = labelA.x - padding;
      int aRight = labelA.x + labelA.width + padding;
      int aTop = labelA.y - padding;
      int aBottom = labelA.y + labelA.height + padding;

      int bLeft = labelB.x - padding;
      int bRight = labelB.x + labelB.width + padding;
      int bTop = labelB.y - padding;
      int bBottom = labelB.y + labelB.height + padding;

      bool overlaps = !(aRight < bLeft || bRight < aLeft ||
                        aBottom < bTop || bBottom < aTop);

      if (overlaps) {
        // Fade out the later label (labelB)
        labelB.alpha = 0;
      }
    }
  }

  // Draw all labels with their computed alpha
  Label currentLabel;
  currentLabel.setFont(ctx.mapFont());
  currentLabel.setColor(ctx.style->geoColor);

  for (const auto& vl : visibleLabels) {
    if (vl.alpha == 0) {
      continue;
    }

    currentLabel.setText(vl.text);
    currentLabel.setPosition(vl.x, vl.y);
    currentLabel.draw(ctx.renderer, vl.alpha);
  }
}

void MapView::drawScaleBars(const RenderContext& ctx) {
  int scalePower = 0;
  int scaleBarDist = screenDist(static_cast<float>(std::pow(10, scalePower)),
                                ctx.screenWidth, ctx.screenHeight);

  const float baseUnit = metric ? 1.0f : 1.852f;

  char scaleLabel[13] = "";

  // Collect all scale bar tick positions and labels first
  struct ScaleBarTick {
    int xPos;
    int labelWidth;
    int power;
  };
  std::vector<ScaleBarTick> ticks;

  while (baseUnit * scaleBarDist < ctx.screenWidth) {
    ScaleBarTick tick;
    tick.xPos = static_cast<int>(baseUnit * (10 + scaleBarDist));
    tick.power = scalePower;

    // Calculate label width
    if (metric) {
      snprintf(scaleLabel, 13, "%d km", static_cast<int>(std::pow(10, scalePower)));
    } else {
      snprintf(scaleLabel, 13, "%d Mm", static_cast<int>(std::pow(10, scalePower)));
    }
    tick.labelWidth = ctx.mapTextWidth(scaleLabel);

    ticks.push_back(tick);

    scalePower++;
    scaleBarDist = screenDist(static_cast<float>(std::pow(10, scalePower)),
                              ctx.screenWidth, ctx.screenHeight);
  }

  // Draw tick marks for all scale bars
  for (const auto& tick : ticks) {
    lineRGBA(ctx.renderer, tick.xPos, 8, tick.xPos, 16 * ctx.uiScale,
             ctx.style->scaleBarColor.r, ctx.style->scaleBarColor.g,
             ctx.style->scaleBarColor.b, 255);
  }

  // Draw labels, skipping those that would overlap with the next (larger) label
  for (size_t i = 0; i < ticks.size(); i++) {
    const auto& tick = ticks[i];

    // Check if this label would overlap with the next one
    bool wouldOverlap = false;
    if (i + 1 < ticks.size()) {
      int labelEnd = tick.xPos + tick.labelWidth;
      int nextLabelStart = ticks[i + 1].xPos;
      if (labelEnd >= nextLabelStart) {
        wouldOverlap = true;
      }
    }

    // Skip this label if it would overlap (prefer the larger scale label)
    if (wouldOverlap) {
      continue;
    }

    if (metric) {
      snprintf(scaleLabel, 13, "%d km", static_cast<int>(std::pow(10, tick.power)));
    } else {
      snprintf(scaleLabel, 13, "%d Mm", static_cast<int>(std::pow(10, tick.power)));
    }

    Label currentLabel;
    currentLabel.setFont(ctx.mapFont());
    currentLabel.setColor(ctx.style->scaleBarColor);
    currentLabel.setPosition(tick.xPos, 15 * ctx.uiScale);
    currentLabel.setText(scaleLabel);
    currentLabel.draw(ctx.renderer);
  }

  // Draw the horizontal scale bar line
  scalePower--;
  scaleBarDist = screenDist(static_cast<float>(std::pow(10, scalePower)),
                            ctx.screenWidth, ctx.screenHeight);

  lineRGBA(ctx.renderer, 0, 10 + 5 * ctx.uiScale, baseUnit * (10 + scaleBarDist), 10 + 5 * ctx.uiScale,
           ctx.style->scaleBarColor.r, ctx.style->scaleBarColor.g,
           ctx.style->scaleBarColor.b, 255);

  // Only draw origin marker if enabled AND lat/lon was actually specified
  if (drawCenterOrigin && originSet) {
    drawCenterOriginPoint(ctx);
  }
}

void MapView::drawCenterOriginPoint(const RenderContext& ctx) {
  const int length = 8;
  const int radius = 5;

  float dx, dy;
  int x, y;

  pxFromLonLat(&dx, &dy, originLon, originLat);
  screenCoords(&x, &y, dx, dy, ctx.screenWidth, ctx.screenHeight);

  SDL_RenderDrawLine(ctx.renderer, x - length, y - length, x + length, y + length);
  SDL_RenderDrawLine(ctx.renderer, x + length, y - length, x - length, y + length);
  const SDL_Rect rect = {x - radius, y - radius, 2 * radius, 2 * radius};
  SDL_RenderDrawRect(ctx.renderer, &rect);
}

MapView::ScaleBarBounds MapView::calculateScaleBarBounds(const RenderContext& ctx) const {
  ScaleBarBounds bounds;

  // Calculate the same way as drawScaleBars
  int scalePower = 0;
  int scaleBarDist = screenDist(static_cast<float>(std::pow(10, scalePower)),
                                ctx.screenWidth, ctx.screenHeight);

  const float baseUnit = metric ? 1.0f : 1.852f;

  // Find the largest scale bar that fits on screen (same logic as drawScaleBars)
  while (baseUnit * scaleBarDist < ctx.screenWidth) {
    scalePower++;
    scaleBarDist = screenDist(static_cast<float>(std::pow(10, scalePower)),
                              ctx.screenWidth, ctx.screenHeight);
  }

  // Step back to the last one that fit
  scalePower--;
  scaleBarDist = screenDist(static_cast<float>(std::pow(10, scalePower)),
                            ctx.screenWidth, ctx.screenHeight);

  // The rightmost extent of the scale bar
  bounds.rightX = static_cast<int>(baseUnit * (10 + scaleBarDist));

  // Add some padding for the label text that appears after the last tick
  // Label format is "X km" or "X Mm" - estimate width
  int labelChars = scalePower + 4;  // digits + " km" or " Mm"
  bounds.rightX += labelChars * ctx.mapFontWidth();

  // Bottom Y is the lowest element - the label at 15 * uiScale plus font height
  bounds.bottomY = static_cast<int>(15 * ctx.uiScale) + ctx.mapFontHeight();

  return bounds;
}

}  // namespace viz1090
