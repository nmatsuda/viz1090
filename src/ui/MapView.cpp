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

#include "SDL2/SDL2_gfxPrimitives.h"
#include "ui/Label.h"
#include "ui/MathUtils.h"
#include "viz1090/Profiler.h"

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

      mapAnimating = 1;
      mapMoved = 1;
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
      mapAnimating = 1;
      mapMoved = 1;
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

  mapMoved = 1;
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

  mapMoved = 1;
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

  mapMoved = 1;
  highFramerate = true;
}

void MapView::setTarget(float lon, float lat) {
  mapTargetLon = lon;
  mapTargetLat = lat;
}

void MapView::setTargetZoom(float zoom) {
  mapTargetMaxDist = zoom;
}

void MapView::drawGeography(const RenderContext& ctx) {
  PROFILE_SCOPE("drawGeography");

  if ((mapRedraw && !mapMoved) || (mapAnimating && elapsed(lastRedraw) > 8 * FRAMETIME) ||
      elapsed(lastRedraw) > 2000 || (map.loaded < 100 && elapsed(lastRedraw) > 250)) {

    SDL_SetRenderTarget(ctx.renderer, mapTexture);

    SDL_SetRenderDrawColor(ctx.renderer, ctx.style->backgroundColor.r,
                           ctx.style->backgroundColor.g, ctx.style->backgroundColor.b, 255);

    SDL_RenderClear(ctx.renderer);

    drawLines(ctx, 0, 0, ctx.screenWidth, ctx.screenHeight);
    drawPlaceNames(ctx);

    SDL_SetRenderTarget(ctx.renderer, NULL);

    mapMoved = 0;
    mapRedraw = 0;
    mapAnimating = 0;

    lastRedraw = now();

    currentLon = centerLon;
    currentLat = centerLat;
    currentMaxDist = maxDist;
  }

  SDL_SetRenderDrawColor(ctx.renderer, ctx.style->backgroundColor.r,
                         ctx.style->backgroundColor.g, ctx.style->backgroundColor.b, 255);

  SDL_RenderClear(ctx.renderer);

  if (mapMoved) {
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

    mapRedraw = 1;
    mapMoved = 0;
  } else {
    SDL_RenderCopy(ctx.renderer, mapTexture, NULL, NULL);
  }
}

void MapView::drawLines(const RenderContext& ctx, int left, int top, int right, int bottom) {
  PROFILE_SCOPE("drawLines");
  float screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max;

  latLonFromScreenCoords(&screen_lat_min, &screen_lon_min, left, top,
                         ctx.screenWidth, ctx.screenHeight);
  latLonFromScreenCoords(&screen_lat_max, &screen_lon_max, right, bottom,
                         ctx.screenWidth, ctx.screenHeight);

  drawLinesRecursive(ctx, &(map.root), screen_lat_min, screen_lat_max, screen_lon_min,
                     screen_lon_max, ctx.style->geoColor);

  drawLinesRecursive(ctx, &(map.airport_root), screen_lat_min, screen_lat_max, screen_lon_min,
                     screen_lon_max, ctx.style->airportColor);
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

void MapView::drawPlaceNames(const RenderContext& ctx) {
  PROFILE_SCOPE("drawPlaceNames");

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
  int charWidth = ctx.mapFontWidth;
  int charHeight = ctx.mapFontHeight;

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
  currentLabel.setFont(ctx.mapFont);
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
  PROFILE_SCOPE("drawScaleBars");
  int scalePower = 0;
  int scaleBarDist = screenDist(static_cast<float>(std::pow(10, scalePower)),
                                ctx.screenWidth, ctx.screenHeight);

  const float baseUnit = metric ? 1.0 : 1.852; // 1 Mn = 1852 m;

  char scaleLabel[13] = "";

  // not sure what is this supposed to draw?
  // lineRGBA(ctx.renderer, 10, 10, 10, 10 * ctx.uiScale, ctx.style->scaleBarColor.r,
  //          ctx.style->scaleBarColor.g, ctx.style->scaleBarColor.b, 255);

  while (baseUnit * scaleBarDist < ctx.screenWidth) {

    lineRGBA(ctx.renderer, baseUnit * (10 + scaleBarDist), 8, baseUnit * (10 + scaleBarDist), 16 * ctx.uiScale,
             ctx.style->scaleBarColor.r, ctx.style->scaleBarColor.g,
             ctx.style->scaleBarColor.b, 255);

    if (metric) {
      snprintf(scaleLabel, 13, "%d km", static_cast<int>(std::pow(10, scalePower)));
    } else {
      snprintf(scaleLabel, 13, "%d Mm", static_cast<int>(std::pow(10, scalePower)));
    }

    Label currentLabel;
    currentLabel.setFont(ctx.mapFont);
    currentLabel.setColor(ctx.style->scaleBarColor);
    currentLabel.setPosition(baseUnit * (10 + scaleBarDist), 15 * ctx.uiScale);
    currentLabel.setText(scaleLabel);
    currentLabel.draw(ctx.renderer);

    scalePower++;
    scaleBarDist = screenDist(static_cast<float>(std::pow(10, scalePower)),
                              ctx.screenWidth, ctx.screenHeight);
  }

  scalePower--;
  scaleBarDist = screenDist(static_cast<float>(std::pow(10, scalePower)),
                            ctx.screenWidth, ctx.screenHeight);

  lineRGBA(ctx.renderer, 0, 10 + 5 * ctx.uiScale, baseUnit * (10 + scaleBarDist), 10 + 5 * ctx.uiScale,
           ctx.style->scaleBarColor.r, ctx.style->scaleBarColor.g,
           ctx.style->scaleBarColor.b, 255);

  if (drawCenterOrigin) {
    drawCenterOriginPoint(ctx);
  }
}

void MapView::drawCenterOriginPoint(const RenderContext& ctx) {
  PROFILE_SCOPE("drawCenterOriginPoint");

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

}  // namespace viz1090
