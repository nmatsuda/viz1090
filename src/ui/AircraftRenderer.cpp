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

#include "ui/AircraftRenderer.h"

#include <cmath>
#include <memory>

#include "SDL2/SDL2_gfxPrimitives.h"
#include "core/Aircraft.h"
#include "ui/AircraftLabel.h"
#include "ui/Label.h"
#include "ui/MapView.h"
#include "ui/MathUtils.h"
#include "viz1090/Profiler.h"

namespace viz1090 {

void AircraftRenderer::draw(const RenderContext& ctx, AircraftList& aircraftList,
                            Aircraft* selectedAircraft, MapView& mapView) {
  PROFILE_SCOPE("drawPlanes");
  SDL_Color planeColor;

  if (selectedAircraft) {
    mapView.setTarget(selectedAircraft->lon, selectedAircraft->lat);
  }

  // Clear clusters for this frame
  clearOffMapClusters(ctx);
  clearOnMapClusters(ctx);

  for (const auto& p : aircraftList) {
    if (p->lon && p->lat) {

      // if lon lat arguments were not provided, start by snapping to the first plane we see
      if (mapView.centerLon == 0 && mapView.centerLat == 0) {
        mapView.setTarget(p->lon, p->lat);
      }

      int x, y;

      float dx, dy;
      mapView.pxFromLonLat(&dx, &dy, p->lon, p->lat);
      mapView.screenCoords(&x, &y, dx, dy, ctx.screenWidth, ctx.screenHeight);

      float age_ms = elapsed(p->created);
      if (age_ms < 500) {
        float ratio = age_ms / 500.0f;
        float radius = (1.0f - ratio * ratio) * ctx.screenWidth / 8;
        for (float theta = 0; theta < 2 * M_PI; theta += M_PI / 4) {
          pixelRGBA(ctx.renderer, static_cast<int>(x + radius * std::cos(theta)),
                    static_cast<int>(y + radius * std::sin(theta)),
                    ctx.style->planeColor.r, ctx.style->planeColor.g,
                    ctx.style->planeColor.b, static_cast<Uint8>(255 * ratio));
        }
      } else if (1000 * DISPLAY_ACTIVE - elapsed(p->msSeen) > 500) {
        int usex = x;
        int usey = y;
        float useHeading = static_cast<float>(p->track);

        p->x = usex;
        p->y = usey;

        planeColor = lerpColor(ctx.style->planeColor, ctx.style->planeGoneColor,
                               elapsed_s(p->msSeen) / DISPLAY_ACTIVE);

        if (elapsed_s(p->msSeen) > DISPLAY_ACTIVE / 2) {
          arcRGBA(ctx.renderer, x, y, 8, 0,
                  static_cast<int>(360 * 2.0 * (elapsed_s(p->msSeen) / DISPLAY_ACTIVE - 0.5)),
                  planeColor.r, planeColor.g, planeColor.b, 255);
        }

        if (p.get() == selectedAircraft) {
          planeColor = ctx.style->selectedColor;
        }

        bool outOfBounds = (x < 0 || x >= ctx.screenWidth || y < 0 || y >= ctx.screenHeight);
        if (outOfBounds) {
          // Add to cluster instead of drawing immediately
          addToOffMapCluster(ctx, x, y, planeColor, p.get());
        } else {
          if (elapsed(p->msSeenLatLon) < 500) {
            circleRGBA(ctx.renderer, p->x, p->y,
                       static_cast<int>(elapsed(p->msSeenLatLon) * ctx.screenWidth / 8192),
                       127, 127, 127,
                       static_cast<Uint8>(255 - 255.0 * elapsed(p->msSeenLatLon) / 500.0));

            mapView.pxFromLonLat(&dx, &dy, p->getLastLon(), p->getLastLat());
            mapView.screenCoords(&x, &y, dx, dy, ctx.screenWidth, ctx.screenHeight);

            usex = static_cast<int>(lerp(static_cast<float>(x), static_cast<float>(usex),
                                         elapsed(p->msSeenLatLon) / 500.0f));
            usey = static_cast<int>(lerp(static_cast<float>(y), static_cast<float>(usey),
                                         elapsed(p->msSeenLatLon) / 500.0f));
            useHeading = lerpAngle(p->getLastHeading(), useHeading,
                                   elapsed(p->msSeenLatLon) / 500.0f);
          }

          // Add to on-map cluster instead of drawing immediately
          addToOnMapCluster(ctx, usex, usey, useHeading, planeColor, p.get());
        }
      } else {
        circleRGBA(ctx.renderer, x, y,
                   static_cast<int>(8 * (1000 * DISPLAY_ACTIVE - elapsed(p->msSeen)) / 500),
                   ctx.style->planeGoneColor.r, ctx.style->planeGoneColor.g,
                   ctx.style->planeGoneColor.b, 255);
      }
    }
  }

  // Draw all on-map clusters first (so labels are on top)
  drawOnMapClusters(ctx, selectedAircraft);

  // Draw all off-map cluster arrows
  drawOffMapClusters(ctx, selectedAircraft);
}

void AircraftRenderer::drawPlaneIcon(const RenderContext& ctx, int x, int y, float heading,
                                     SDL_Color planeColor) {
  float body = 8.0f * ctx.uiScale;
  float wing = 6.0f * ctx.uiScale;
  float wingThick = 0.5f;
  float tail = 3.0f * ctx.uiScale;
  float tailThick = 0.35f;
  float bodyWidth = static_cast<float>(ctx.uiScale);

  float vec[3];
  vec[0] = std::sin(heading * static_cast<float>(M_PI) / 180.0f);
  vec[1] = -std::cos(heading * static_cast<float>(M_PI) / 180.0f);
  vec[2] = 0;

  float up[] = {0, 0, 1};
  float out[3];
  crossProduct(out, vec, up);

  int x1, x2, y1, y2;

  // body
  x1 = x + static_cast<int>(std::round(-bodyWidth * out[0]));
  y1 = y + static_cast<int>(std::round(-bodyWidth * out[1]));
  x2 = x + static_cast<int>(std::round(bodyWidth * out[0]));
  y2 = y + static_cast<int>(std::round(bodyWidth * out[1]));

  filledTrigonRGBA(ctx.renderer, x1, y1, x2, y2,
                   x + static_cast<int>(std::round(-body * vec[0])),
                   y + static_cast<int>(std::round(-body * vec[1])),
                   planeColor.r, planeColor.g, planeColor.b, SDL_ALPHA_OPAQUE);
  filledTrigonRGBA(ctx.renderer, x1, y1, x2, y2,
                   x + static_cast<int>(std::round(body * vec[0])),
                   y + static_cast<int>(std::round(body * vec[1])),
                   planeColor.r, planeColor.g, planeColor.b, SDL_ALPHA_OPAQUE);

  // wing
  x1 = x + static_cast<int>(std::round(-wing * out[0]));
  y1 = y + static_cast<int>(std::round(-wing * out[1]));
  x2 = x + static_cast<int>(std::round(wing * out[0]));
  y2 = y + static_cast<int>(std::round(wing * out[1]));

  filledTrigonRGBA(ctx.renderer, x1, y1, x2, y2,
                   x + static_cast<int>(std::round(body * wingThick * vec[0])),
                   y + static_cast<int>(std::round(body * wingThick * vec[1])),
                   planeColor.r, planeColor.g, planeColor.b, SDL_ALPHA_OPAQUE);

  // tail
  x1 = x + static_cast<int>(std::round(-body * 0.75f * vec[0] - tail * out[0]));
  y1 = y + static_cast<int>(std::round(-body * 0.75f * vec[1] - tail * out[1]));
  x2 = x + static_cast<int>(std::round(-body * 0.75f * vec[0] + tail * out[0]));
  y2 = y + static_cast<int>(std::round(-body * 0.75f * vec[1] + tail * out[1]));

  filledTrigonRGBA(ctx.renderer, x1, y1, x2, y2,
                   x + static_cast<int>(std::round(-body * tailThick * vec[0])),
                   y + static_cast<int>(std::round(-body * tailThick * vec[1])),
                   planeColor.r, planeColor.g, planeColor.b, SDL_ALPHA_OPAQUE);
}

// Off-map plane clustering implementation (greedy distance-based)

void AircraftRenderer::clearOffMapClusters(const RenderContext& ctx) {
  // Cluster radius based on arrow footprint on screen edge
  // Arrows are ~4x arrow width, so use that as clustering distance
  offMapClusterRadius_ = 24.0f * ctx.uiScale;

  offMapClusters_.clear();
}

void AircraftRenderer::addToOffMapCluster(const RenderContext& ctx, int x, int y,
                                           SDL_Color planeColor, Aircraft* aircraft) {
  int centerX = ctx.screenWidth >> 1;
  int centerY = ctx.screenHeight >> 1;

  // Get direction from screen center to plane
  float inx = static_cast<float>(x - centerX);
  float iny = static_cast<float>(y - centerY);

  // Calculate distance from center
  float dist = std::sqrt(inx * inx + iny * iny);

  // Normalize direction
  float dirX = (dist > 0.001f) ? inx / dist : 0.0f;
  float dirY = (dist > 0.001f) ? iny / dist : 1.0f;

  // Project to screen edge to get edge position
  float outx = inx;
  float outy = iny;

  if (std::abs(inx) > std::abs(iny) *
                          static_cast<float>(centerX) /
                          static_cast<float>(centerY)) {
    outx = static_cast<float>(centerX) * ((inx > 0) ? 1.0f : -1.0f);
    outy = (outx) * iny / (inx);
  } else {
    outy = static_cast<float>(ctx.screenHeight) * ((iny > 0) ? 0.5f : -0.5f);
    outx = (outy) * inx / (iny);
  }

  // Find nearest existing cluster within radius (based on edge position)
  float radiusSq = offMapClusterRadius_ * offMapClusterRadius_;
  OffMapCluster* nearest = nullptr;
  float nearestDistSq = radiusSq;

  for (auto& cluster : offMapClusters_) {
    float cdx = outx - cluster.edgeX;
    float cdy = outy - cluster.edgeY;
    float cdistSq = cdx * cdx + cdy * cdy;

    if (cdistSq < nearestDistSq) {
      nearestDistSq = cdistSq;
      nearest = &cluster;
    }
  }

  if (nearest) {
    // Add to existing cluster - update average distance
    float newCount = static_cast<float>(nearest->count + 1);
    nearest->avgDistance = (nearest->avgDistance * static_cast<float>(nearest->count) + dist) / newCount;
    nearest->color = lerpColor(nearest->color, planeColor, 1.0f / newCount);
    nearest->singleAircraft = nullptr;  // Multiple planes
    nearest->count++;
  } else {
    // Create new cluster
    OffMapCluster newCluster;
    newCluster.count = 1;
    newCluster.edgeX = outx;
    newCluster.edgeY = outy;
    newCluster.dirX = dirX;
    newCluster.dirY = dirY;
    newCluster.avgDistance = dist;
    newCluster.color = planeColor;
    newCluster.singleAircraft = aircraft;
    offMapClusters_.push_back(newCluster);
  }
}

void AircraftRenderer::drawOffMapClusters(const RenderContext& ctx, Aircraft* selectedAircraft) {
  // Calculate distance thresholds for color lerping
  // At screen edge (half diagonal) = full plane color
  // At 2x view width beyond edge = grey_dark
  float halfWidth = static_cast<float>(ctx.screenWidth >> 1);
  float halfHeight = static_cast<float>(ctx.screenHeight >> 1);
  float edgeDist = std::sqrt(halfWidth * halfWidth + halfHeight * halfHeight);
  float fadeStartDist = edgeDist;
  float fadeEndDist = edgeDist + 2.0f * static_cast<float>(ctx.screenWidth);

  for (const auto& cluster : offMapClusters_) {
    if (cluster.count == 0) {
      continue;
    }

    // Calculate color based on distance
    float distRatio = clamp((cluster.avgDistance - fadeStartDist) / (fadeEndDist - fadeStartDist),
                            0.0f, 1.0f);
    SDL_Color drawColor = lerpColor(cluster.color, ctx.style->grey_dark, distRatio);

    drawOffMapArrow(ctx, cluster.edgeX, cluster.edgeY, cluster.dirX, cluster.dirY,
                    drawColor, cluster.count);

    // For single planes, update position and draw normal label
    if (cluster.count == 1 && cluster.singleAircraft) {
      float arrowWidth = 6.0f * ctx.uiScale;
      int centerX = ctx.screenWidth >> 1;
      int centerY = ctx.screenHeight >> 1;
      cluster.singleAircraft->x = static_cast<int>(centerX + cluster.edgeX -
                                                    2.0f * arrowWidth * cluster.dirX);
      cluster.singleAircraft->y = static_cast<int>(centerY + cluster.edgeY -
                                                    2.0f * arrowWidth * cluster.dirY);
      drawPlaneText(ctx, cluster.singleAircraft, selectedAircraft);
    }
  }
}

void AircraftRenderer::drawOffMapArrow(const RenderContext& ctx, float edgeX, float edgeY,
                                        float dirX, float dirY, SDL_Color planeColor, int count) {
  float arrowWidth = 6.0f * ctx.uiScale;

  float vec[3] = {dirX, dirY, 0.0f};
  float up[] = {0, 0, 1};
  float out[3];
  crossProduct(out, vec, up);

  int centerX = ctx.screenWidth >> 1;
  int centerY = ctx.screenHeight >> 1;

  int x1, x2, x3, y1, y2, y3;

  // arrow 1
  x1 = static_cast<int>(centerX + edgeX - 2.0f * arrowWidth * vec[0] +
                        std::round(-arrowWidth * out[0]));
  y1 = static_cast<int>(centerY + edgeY - 2.0f * arrowWidth * vec[1] +
                        std::round(-arrowWidth * out[1]));
  x2 = static_cast<int>(centerX + edgeX - 2.0f * arrowWidth * vec[0] +
                        std::round(arrowWidth * out[0]));
  y2 = static_cast<int>(centerY + edgeY - 2.0f * arrowWidth * vec[1] +
                        std::round(arrowWidth * out[1]));
  x3 = static_cast<int>(centerX + edgeX - arrowWidth * vec[0]);
  y3 = static_cast<int>(centerY + edgeY - arrowWidth * vec[1]);
  filledTrigonRGBA(ctx.renderer, x1, y1, x2, y2, x3, y3,
                   planeColor.r, planeColor.g, planeColor.b, SDL_ALPHA_OPAQUE);

  // arrow 2
  x1 = static_cast<int>(centerX + edgeX - 3.0f * arrowWidth * vec[0] +
                        std::round(-arrowWidth * out[0]));
  y1 = static_cast<int>(centerY + edgeY - 3.0f * arrowWidth * vec[1] +
                        std::round(-arrowWidth * out[1]));
  x2 = static_cast<int>(centerX + edgeX - 3.0f * arrowWidth * vec[0] +
                        std::round(arrowWidth * out[0]));
  y2 = static_cast<int>(centerY + edgeY - 3.0f * arrowWidth * vec[1] +
                        std::round(arrowWidth * out[1]));
  x3 = static_cast<int>(centerX + edgeX - 2.0f * arrowWidth * vec[0]);
  y3 = static_cast<int>(centerY + edgeY - 2.0f * arrowWidth * vec[1]);
  filledTrigonRGBA(ctx.renderer, x1, y1, x2, y2, x3, y3,
                   planeColor.r, planeColor.g, planeColor.b, SDL_ALPHA_OPAQUE);

  // Draw count label if more than 1 plane
  if (count > 1) {
    // Position label slightly inward from arrow
    int labelX = static_cast<int>(centerX + edgeX - 5.0f * arrowWidth * vec[0]);
    int labelY = static_cast<int>(centerY + edgeY - 5.0f * arrowWidth * vec[1]);

    // Draw count as label
    Label countLabel;
    countLabel.setFont(ctx.labelFont);
    countLabel.setColor(planeColor);
    countLabel.setPosition(labelX - ctx.labelFontWidth, labelY - ctx.labelFontHeight / 2);
    countLabel.setText(std::to_string(count));
    countLabel.draw(ctx.renderer);
  }
}

// On-map plane clustering implementation (greedy distance-based)

void AircraftRenderer::clearOnMapClusters(const RenderContext& ctx) {
  // Cluster radius based on plane icon size (body + wing span)
  // Body is 8 * uiScale, wing is 6 * uiScale on each side
  clusterRadius_ = 20.0f * ctx.uiScale;

  onMapClusters_.clear();
}

void AircraftRenderer::addToOnMapCluster(const RenderContext& /* ctx */, int x, int y, float heading,
                                          SDL_Color planeColor, Aircraft* aircraft) {
  float fx = static_cast<float>(x);
  float fy = static_cast<float>(y);
  float radiusSq = clusterRadius_ * clusterRadius_;

  // Find nearest existing cluster within radius
  OnMapCluster* nearest = nullptr;
  float nearestDistSq = radiusSq;

  for (auto& cluster : onMapClusters_) {
    float dx = fx - cluster.centerX;
    float dy = fy - cluster.centerY;
    float distSq = dx * dx + dy * dy;

    if (distSq < nearestDistSq) {
      nearestDistSq = distSq;
      nearest = &cluster;
    }
  }

  if (nearest) {
    // Add to existing cluster
    float newCount = static_cast<float>(nearest->count + 1);
    nearest->avgHeading = lerpAngle(nearest->avgHeading, heading, 1.0f / newCount);
    nearest->color = lerpColor(nearest->color, planeColor, 1.0f / newCount);
    nearest->singleAircraft = nullptr;  // Multiple planes
    nearest->count++;
  } else {
    // Create new cluster centered on this plane
    OnMapCluster newCluster;
    newCluster.count = 1;
    newCluster.centerX = fx;
    newCluster.centerY = fy;
    newCluster.avgHeading = heading;
    newCluster.color = planeColor;
    newCluster.singleAircraft = aircraft;
    onMapClusters_.push_back(newCluster);
  }
}

void AircraftRenderer::drawOnMapClusters(const RenderContext& ctx, Aircraft* selectedAircraft) {
  for (const auto& cluster : onMapClusters_) {
    if (cluster.count == 0) {
      continue;
    }

    int x = static_cast<int>(cluster.centerX);
    int y = static_cast<int>(cluster.centerY);

    if (cluster.count == 1 && cluster.singleAircraft) {
      // Single plane - draw normal icon and label
      drawPlaneIcon(ctx, x, y, cluster.avgHeading, cluster.color);
      cluster.singleAircraft->x = x;
      cluster.singleAircraft->y = y;
      drawPlaneText(ctx, cluster.singleAircraft, selectedAircraft);
    } else if (cluster.count > 1) {
      // Multiple planes - draw circle instead of directional icon
      int radius = static_cast<int>(8.0f * ctx.uiScale);
      circleRGBA(ctx.renderer, x, y, radius,
                 cluster.color.r, cluster.color.g, cluster.color.b, SDL_ALPHA_OPAQUE);

      // Draw count label centered inside the circle
      std::string countText = std::to_string(cluster.count);
      int textWidth = static_cast<int>(countText.length() * ctx.labelFontWidth);
      int textHeight = ctx.labelFontHeight;
      int labelX = x - textWidth / 2;
      int labelY = y - textHeight / 2;

      Label countLabel;
      countLabel.setFont(ctx.labelFont);
      countLabel.setColor(cluster.color);
      countLabel.setPosition(labelX, labelY);
      countLabel.setText(countText);
      countLabel.draw(ctx.renderer);
    }
  }
}

void AircraftRenderer::drawPlaneText(const RenderContext& ctx, Aircraft* p,
                                     Aircraft* selectedAircraft) {
  if (!p->label) {
    p->label = std::make_unique<AircraftLabel>(p, metric, ctx.screenWidth, ctx.screenHeight,
                                               ctx.mapFont, *ctx.style);
  }

  p->label->update();
  p->label->draw(ctx.renderer, (p == selectedAircraft));
}

void AircraftRenderer::drawTrails(const RenderContext& ctx, const AircraftList& aircraftList,
                                  const MapView& mapView, int left, int top, int right, int bottom) {
  PROFILE_SCOPE("drawTrails");
  int currentX, currentY, prevX, prevY;
  float dx, dy;

  for (const auto& aircraft : aircraftList) {
    if (aircraft->positionHistory.empty()) {
      continue;
    }

    const auto& history = aircraft->positionHistory;
    float historySize = static_cast<float>(history.size());

    for (size_t i = 0; i + 1 < history.size(); ++i) {
      float age = static_cast<float>(i);

      mapView.pxFromLonLat(&dx, &dy, history[i + 1].lon, history[i + 1].lat);
      mapView.screenCoords(&currentX, &currentY, dx, dy, ctx.screenWidth, ctx.screenHeight);

      mapView.pxFromLonLat(&dx, &dy, history[i].lon, history[i].lat);
      mapView.screenCoords(&prevX, &prevY, dx, dy, ctx.screenWidth, ctx.screenHeight);

      bool currentOOB = (currentX < left || currentX >= right ||
                         currentY < top || currentY >= bottom);
      bool prevOOB = (prevX < left || prevX >= right || prevY < top || prevY >= bottom);
      if (currentOOB && prevOOB) {
        continue;
      }

      SDL_Color color = lerpColor(makeColor(255, 0, 0), makeColor(255, 200, 0), age / historySize);

      color = lerpColor(color, ctx.style->planeGoneColor, elapsed_s(aircraft->msSeen) / DISPLAY_ACTIVE);
      color = lerpColor(color, ctx.style->black, -1.0f + (elapsed_s(aircraft->msSeen) / DISPLAY_ACTIVE));

      int colorVal = static_cast<int>(clamp(512.0f * (age / historySize), 0.0f, 255.0f));

      lineRGBA(ctx.renderer, prevX, prevY, currentX, currentY,
               color.r, color.g, color.b, static_cast<Uint8>(colorVal));
    }

    if (elapsed_s(aircraft->msSeen) > DISPLAY_ACTIVE) {
      SDL_Color color = lerpColor(ctx.style->planeGoneColor, ctx.style->black,
                                  -1.0f + (elapsed_s(aircraft->msSeen) / DISPLAY_ACTIVE));
      int colorVal = static_cast<int>(clamp(512.0f * (static_cast<float>(history.size() - 1) / historySize),
                                            0.0f, 255.0f));
      circleRGBA(ctx.renderer, currentX, currentY, 5, color.r, color.g, color.b,
                 static_cast<Uint8>(colorVal));
    }
  }
}

void AircraftRenderer::resolveLabelConflicts(AircraftList& aircraftList) {
  PROFILE_SCOPE("resolveLabelConflicts");

  for (const auto& aircraft : aircraftList) {
    if (aircraft->label) {
      aircraft->label->clearAcceleration();
    }
  }

  for (const auto& aircraft : aircraftList) {
    if (aircraft->label) {
      aircraft->label->calculateForces(aircraftList);
    }
  }

  for (const auto& aircraft : aircraftList) {
    if (aircraft->label) {
      aircraft->label->applyForces();
    }
  }
}

void AircraftRenderer::moveLabels(AircraftList& aircraftList, float dx, float dy) {
  for (const auto& aircraft : aircraftList) {
    if (aircraft->label) {
      aircraft->label->move(dx, dy);
    }
  }
}

}  // namespace viz1090
