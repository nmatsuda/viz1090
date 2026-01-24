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

namespace viz1090 {

// Easing function for smooth animation
static float easeOutQuad(float t) {
  return t * (2.0f - t);
}

// View state management

ui::AircraftViewState& AircraftRenderer::getOrCreateViewState(const RenderContext& ctx, Aircraft* p) {
  auto& state = viewStates_.getOrCreate(p->addr);

  // Create label if needed
  if (!state.label) {
    state.label = std::make_unique<ui::AircraftLabel>(
        p->addr, *metric, ctx.screenWidth, ctx.screenHeight,
        ctx.mapFont, *ctx.style);
  }

  return state;
}

void AircraftRenderer::cleanupStaleViewStates(const AircraftList& aircraftList) {
  std::unordered_map<uint32_t, bool> activeAddrs;
  for (const auto& aircraft : aircraftList) {
    activeAddrs[aircraft->addr] = true;
  }
  viewStates_.removeStale(activeAddrs);
}

bool AircraftRenderer::getScreenCoords(uint32_t addr, int& outX, int& outY) const {
  const auto* state = viewStates_.get(addr);
  if (state) {
    outX = state->screenX;
    outY = state->screenY;
    return true;
  }
  return false;
}

std::vector<ui::LabelNeighbor> AircraftRenderer::buildNeighborList(const AircraftList& aircraftList) const {
  std::vector<ui::LabelNeighbor> neighbors;
  neighbors.reserve(aircraftList.size());

  for (const auto& aircraft : aircraftList) {
    const auto* state = viewStates_.get(aircraft->addr);
    if (state && state->label) {
      ui::LabelNeighbor neighbor;
      neighbor.x = state->label->getX();
      neighbor.y = state->label->getY();
      neighbor.w = state->label->getWidth();
      neighbor.h = state->label->getHeight();
      neighbor.aircraftX = state->screenX;
      neighbor.aircraftY = state->screenY;
      neighbor.addr = aircraft->addr;
      neighbors.push_back(neighbor);
    }
  }

  return neighbors;
}

void AircraftRenderer::draw(const RenderContext& ctx, AircraftList& aircraftList,
                            Aircraft* selectedAircraft, MapView& mapView) {
  SDL_Color planeColor;

  // Update label config with screen dimensions
  labelConfig_.screenWidth = ctx.screenWidth;
  labelConfig_.screenHeight = ctx.screenHeight;

  if (selectedAircraft) {
    mapView.setTarget(selectedAircraft->lon, selectedAircraft->lat);
  }

  // Store last frame's clusters as previous, then clear for this frame
  prevOnMapClusters_ = std::move(onMapClusters_);
  prevOffMapClusters_ = std::move(offMapClusters_);

  // Clear clusters for this frame
  clearOffMapClusters(ctx);
  clearOnMapClusters(ctx);

  // Phase 1: Compute spatial clusters (pure geometry)
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

      // Get or create view state for this aircraft
      auto& viewState = getOrCreateViewState(ctx, p.get());
      viewState.screenX = x;
      viewState.screenY = y;

      float age_ms = elapsed(p->created);
      if (age_ms < 500) {
        // Only draw appear animation if aircraft is in SOLO state
        if (viewState.clusterState == ui::ClusterState::SOLO) {
          float ratio = age_ms / 500.0f;
          float radius = (1.0f - ratio * ratio) * ctx.screenWidth / 8;
          for (float theta = 0; theta < 2 * M_PI; theta += M_PI / 4) {
            pixelRGBA(ctx.renderer, static_cast<int>(x + radius * std::cos(theta)),
                      static_cast<int>(y + radius * std::sin(theta)),
                      ctx.style->planeColor.r, ctx.style->planeColor.g,
                      ctx.style->planeColor.b, static_cast<Uint8>(255 * ratio));
          }
        }
      } else if (1000 * DISPLAY_ACTIVE - elapsed(p->msSeen) > 500) {
        int usex = x;
        int usey = y;
        float useHeading = static_cast<float>(p->track);

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

        // Cache heading and color for animations
        viewState.heading = useHeading;
        viewState.color = planeColor;

        // Check if plane is off-map (outside screen bounds or in UI overlay region)
        bool outOfBounds = isOffMap(x, y, ctx.screenWidth, ctx.screenHeight);
        viewState.isOffMap = outOfBounds;

        if (outOfBounds) {
          // Add to off-map cluster (pure spatial)
          addToOffMapCluster(ctx, x, y, planeColor, p.get());
        } else {
          // Draw ping circle only if SOLO and not animating
          if (elapsed(p->msSeenLatLon) < 500 &&
              viewState.clusterState == ui::ClusterState::SOLO &&
              viewState.animProgress >= 1.0f) {
            circleRGBA(ctx.renderer, viewState.screenX, viewState.screenY,
                       static_cast<int>(elapsed(p->msSeenLatLon) * ctx.screenWidth / 8192),
                       127, 127, 127,
                       static_cast<Uint8>(255 - 255.0 * elapsed(p->msSeenLatLon) / 500.0));
          }

          if (elapsed(p->msSeenLatLon) < 500) {
            mapView.pxFromLonLat(&dx, &dy, p->getLastLon(), p->getLastLat());
            mapView.screenCoords(&x, &y, dx, dy, ctx.screenWidth, ctx.screenHeight);

            usex = static_cast<int>(lerp(static_cast<float>(x), static_cast<float>(usex),
                                         elapsed(p->msSeenLatLon) / 500.0f));
            usey = static_cast<int>(lerp(static_cast<float>(y), static_cast<float>(usey),
                                         elapsed(p->msSeenLatLon) / 500.0f));
            useHeading = lerpAngle(p->getLastHeading(), useHeading,
                                   elapsed(p->msSeenLatLon) / 500.0f);
          }

          // Add to on-map cluster (pure spatial)
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

  // Phase 2: Update cluster state machine for each aircraft
  updateClusterStates(ctx, aircraftList);

  // Phase 3: Draw all icons/arrows first (before labels)
  drawOnMapClusterIcons(ctx);
  drawOffMapClusterArrows(ctx);

  // Phase 4: Draw cluster animations (merge/unmerge)
  drawClusterAnimations(ctx, aircraftList);

  // Phase 5: Draw all labels last (on top of icons)
  drawOnMapClusterLabels(ctx, selectedAircraft);
  drawOffMapClusterLabels(ctx, selectedAircraft);

  // Cleanup stale view states
  cleanupStaleViewStates(aircraftList);
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

// Off-map plane clustering implementation

void AircraftRenderer::clearOffMapClusters(const RenderContext& ctx) {
  offMapClusterRadius_ = 24.0f * ctx.uiScale;
  offMapClusters_.clear();
}

void AircraftRenderer::addToOffMapCluster(const RenderContext& ctx, int x, int y,
                                           SDL_Color planeColor, Aircraft* aircraft) {
  // Pure spatial clustering - no state machine logic here
  uint32_t addr = aircraft->addr;
  int centerX = ctx.screenWidth >> 1;
  int centerY = ctx.screenHeight >> 1;

  float inx = static_cast<float>(x - centerX);
  float iny = static_cast<float>(y - centerY);
  float dist = std::sqrt(inx * inx + iny * iny);
  float dirX = (dist > 0.001f) ? inx / dist : 0.0f;
  float dirY = (dist > 0.001f) ? iny / dist : 1.0f;

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

  // Avoid status bar
  if (uiStatusBarTopY_ > 0 && iny > 0) {
    float absX = static_cast<float>(centerX) + outx;
    float absY = static_cast<float>(centerY) + outy;

    if (absY > static_cast<float>(uiStatusBarTopY_) && absX < static_cast<float>(uiStatusBarRightX_)) {
      outy = static_cast<float>(uiStatusBarTopY_ - centerY);
      if (std::abs(iny) > 0.001f) {
        outx = outy * inx / iny;
      }
    }
  }

  // Avoid scale bar
  if (scaleBarBottomY_ > 0 && iny < 0) {
    float absX = static_cast<float>(centerX) + outx;
    float absY = static_cast<float>(centerY) + outy;

    if (absY < static_cast<float>(scaleBarBottomY_) && absX < static_cast<float>(scaleBarRightX_)) {
      outy = static_cast<float>(scaleBarBottomY_ - centerY);
      if (std::abs(iny) > 0.001f) {
        outx = outy * inx / iny;
      }
    }
  }

  float radiusSq = offMapClusterRadius_ * offMapClusterRadius_;

  // Find which previous cluster this aircraft was in (for cluster-mate stickiness)
  const OffMapCluster* prevClusterForAircraft = nullptr;
  for (const auto& prevCluster : prevOffMapClusters_) {
    if (prevCluster.memberAddrs.count(addr) > 0) {
      prevClusterForAircraft = &prevCluster;
      break;
    }
  }

  OffMapCluster* nearest = nullptr;
  float nearestDistSq = radiusSq;

  // Priority 1: Find a cluster containing previous cluster-mates (with relaxed distance)
  if (prevClusterForAircraft && prevClusterForAircraft->count > 1) {
    for (auto& cluster : offMapClusters_) {
      bool hasClusterMate = false;
      for (uint32_t otherAddr : cluster.memberAddrs) {
        if (prevClusterForAircraft->memberAddrs.count(otherAddr) > 0) {
          hasClusterMate = true;
          break;
        }
      }
      if (hasClusterMate) {
        float cdx = outx - cluster.edgeX;
        float cdy = outy - cluster.edgeY;
        float cdistSq = cdx * cdx + cdy * cdy;
        // Use 4x radius tolerance (2x linear) for cluster-mate stickiness
        if (cdistSq < radiusSq * 4.0f) {
          nearest = &cluster;
          break;
        }
      }
    }
  }

  // Priority 2: Fall back to nearest cluster within normal radius
  if (!nearest) {
    for (auto& cluster : offMapClusters_) {
      float cdx = outx - cluster.edgeX;
      float cdy = outy - cluster.edgeY;
      float cdistSq = cdx * cdx + cdy * cdy;

      if (cdistSq < nearestDistSq) {
        nearestDistSq = cdistSq;
        nearest = &cluster;
      }
    }
  }

  if (nearest) {
    float newCount = static_cast<float>(nearest->count + 1);
    nearest->avgDistance = (nearest->avgDistance * static_cast<float>(nearest->count) + dist) / newCount;
    nearest->color = lerpColor(nearest->color, planeColor, 1.0f / newCount);
    nearest->singleAircraft = nullptr;
    nearest->count++;
    nearest->memberAddrs.insert(addr);
  } else {
    OffMapCluster newCluster;
    newCluster.count = 1;
    newCluster.edgeX = outx;
    newCluster.edgeY = outy;
    newCluster.dirX = dirX;
    newCluster.dirY = dirY;
    newCluster.avgDistance = dist;
    newCluster.color = planeColor;
    newCluster.singleAircraft = aircraft;
    newCluster.memberAddrs.insert(addr);
    offMapClusters_.push_back(newCluster);
  }
}

void AircraftRenderer::drawOffMapClusterArrows(const RenderContext& ctx) {
  using ui::ClusterState;

  float halfWidth = static_cast<float>(ctx.screenWidth >> 1);
  float halfHeight = static_cast<float>(ctx.screenHeight >> 1);
  float edgeDist = std::sqrt(halfWidth * halfWidth + halfHeight * halfHeight);
  float fadeStartDist = edgeDist;
  float fadeEndDist = edgeDist + 2.0f * static_cast<float>(ctx.screenWidth);

  for (const auto& cluster : offMapClusters_) {
    if (cluster.count == 0) continue;

    if (cluster.count == 1 && cluster.singleAircraft) {
      uint32_t addr = cluster.singleAircraft->addr;
      auto* viewState = viewStates_.get(addr);

      // Skip drawing arrow if in unmerge animation
      if (viewState && viewState->clusterState == ClusterState::SOLO &&
          viewState->animProgress < 1.0f) {
        float arrowWidth = 6.0f * ctx.uiScale;
        int centerX = ctx.screenWidth >> 1;
        int centerY = ctx.screenHeight >> 1;

        viewState->screenX = static_cast<int>(centerX + cluster.edgeX -
                                               2.0f * arrowWidth * cluster.dirX);
        viewState->screenY = static_cast<int>(centerY + cluster.edgeY -
                                               2.0f * arrowWidth * cluster.dirY);
        continue;
      }
    }

    float distRatio = clamp((cluster.avgDistance - fadeStartDist) / (fadeEndDist - fadeStartDist),
                            0.0f, 1.0f);
    SDL_Color drawColor = lerpColor(cluster.color, ctx.style->grey_dark, distRatio);

    drawOffMapArrow(ctx, cluster.edgeX, cluster.edgeY, cluster.dirX, cluster.dirY,
                    drawColor, cluster.count);

    if (cluster.count == 1 && cluster.singleAircraft) {
      float arrowWidth = 6.0f * ctx.uiScale;
      int centerX = ctx.screenWidth >> 1;
      int centerY = ctx.screenHeight >> 1;

      auto* viewState = viewStates_.get(cluster.singleAircraft->addr);
      if (viewState) {
        viewState->screenX = static_cast<int>(centerX + cluster.edgeX -
                                               2.0f * arrowWidth * cluster.dirX);
        viewState->screenY = static_cast<int>(centerY + cluster.edgeY -
                                               2.0f * arrowWidth * cluster.dirY);
      }
    }
  }
}

void AircraftRenderer::drawOffMapClusterLabels(const RenderContext& ctx, Aircraft* selectedAircraft) {
  using ui::ClusterState;

  for (const auto& cluster : offMapClusters_) {
    if (cluster.count == 0) continue;

    if (cluster.count == 1 && cluster.singleAircraft) {
      uint32_t addr = cluster.singleAircraft->addr;
      auto* viewState = viewStates_.get(addr);

      // Skip label if in unmerge animation
      if (viewState && viewState->clusterState == ClusterState::SOLO &&
          viewState->animProgress < 1.0f) {
        continue;
      }

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

  if (count > 1) {
    int labelX = static_cast<int>(centerX + edgeX - 5.0f * arrowWidth * vec[0]);
    int labelY = static_cast<int>(centerY + edgeY - 5.0f * arrowWidth * vec[1]);

    Label countLabel;
    countLabel.setFont(ctx.labelFont);
    countLabel.setColor(planeColor);
    countLabel.setPosition(labelX - ctx.labelFontWidth, labelY - ctx.labelFontHeight / 2);
    countLabel.setText(std::to_string(count));
    countLabel.draw(ctx.renderer);
  }
}

// On-map plane clustering implementation

void AircraftRenderer::clearOnMapClusters(const RenderContext& ctx) {
  clusterRadius_ = 20.0f * ctx.uiScale;
  onMapClusters_.clear();
}

void AircraftRenderer::addToOnMapCluster(const RenderContext& /* ctx */, int x, int y, float heading,
                                          SDL_Color planeColor, Aircraft* aircraft) {
  // Pure spatial clustering - no state machine logic here
  uint32_t addr = aircraft->addr;
  float fx = static_cast<float>(x);
  float fy = static_cast<float>(y);
  float radiusSq = clusterRadius_ * clusterRadius_;

  // Find which previous cluster this aircraft was in (for cluster-mate stickiness)
  const OnMapCluster* prevClusterForAircraft = nullptr;
  for (const auto& prevCluster : prevOnMapClusters_) {
    if (prevCluster.memberAddrs.count(addr) > 0) {
      prevClusterForAircraft = &prevCluster;
      break;
    }
  }

  OnMapCluster* nearest = nullptr;
  float nearestDistSq = radiusSq;

  // Priority 1: Find a cluster containing previous cluster-mates (with relaxed distance)
  if (prevClusterForAircraft && prevClusterForAircraft->count > 1) {
    for (auto& cluster : onMapClusters_) {
      bool hasClusterMate = false;
      for (uint32_t otherAddr : cluster.memberAddrs) {
        if (prevClusterForAircraft->memberAddrs.count(otherAddr) > 0) {
          hasClusterMate = true;
          break;
        }
      }
      if (hasClusterMate) {
        float dx = fx - cluster.centerX;
        float dy = fy - cluster.centerY;
        float distSq = dx * dx + dy * dy;
        // Use 4x radius tolerance (2x linear) for cluster-mate stickiness
        if (distSq < radiusSq * 4.0f) {
          nearest = &cluster;
          break;
        }
      }
    }
  }

  // Priority 2: Fall back to nearest cluster within normal radius
  if (!nearest) {
    for (auto& cluster : onMapClusters_) {
      float dx = fx - cluster.centerX;
      float dy = fy - cluster.centerY;
      float distSq = dx * dx + dy * dy;

      if (distSq < nearestDistSq) {
        nearestDistSq = distSq;
        nearest = &cluster;
      }
    }
  }

  if (nearest) {
    float newCount = static_cast<float>(nearest->count + 1);
    nearest->avgHeading = lerpAngle(nearest->avgHeading, heading, 1.0f / newCount);
    nearest->color = lerpColor(nearest->color, planeColor, 1.0f / newCount);
    nearest->singleAircraft = nullptr;
    nearest->count++;
    nearest->memberAddrs.insert(addr);
  } else {
    OnMapCluster newCluster;
    newCluster.count = 1;
    newCluster.centerX = fx;
    newCluster.centerY = fy;
    newCluster.avgHeading = heading;
    newCluster.color = planeColor;
    newCluster.singleAircraft = aircraft;
    newCluster.memberAddrs.insert(addr);
    onMapClusters_.push_back(newCluster);
  }
}

void AircraftRenderer::drawOnMapClusterIcons(const RenderContext& ctx) {
  using ui::ClusterState;

  for (const auto& cluster : onMapClusters_) {
    if (cluster.count == 0) continue;

    int x = static_cast<int>(cluster.centerX);
    int y = static_cast<int>(cluster.centerY);

    if (cluster.count == 1 && cluster.singleAircraft) {
      uint32_t addr = cluster.singleAircraft->addr;
      auto* viewState = viewStates_.get(addr);

      // Skip drawing icon if in unmerge animation (animating from cluster to solo)
      if (viewState && viewState->clusterState == ClusterState::SOLO &&
          viewState->animProgress < 1.0f) {
        viewState->screenX = x;
        viewState->screenY = y;
        continue;
      }

      drawPlaneIcon(ctx, x, y, cluster.avgHeading, cluster.color);

      if (viewState) {
        viewState->screenX = x;
        viewState->screenY = y;
      }
    } else if (cluster.count > 1) {
      int radius = static_cast<int>(8.0f * ctx.uiScale);
      circleRGBA(ctx.renderer, x, y, radius,
                 cluster.color.r, cluster.color.g, cluster.color.b, SDL_ALPHA_OPAQUE);

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

void AircraftRenderer::drawOnMapClusterLabels(const RenderContext& ctx, Aircraft* selectedAircraft) {
  using ui::ClusterState;

  for (const auto& cluster : onMapClusters_) {
    if (cluster.count == 0) continue;

    if (cluster.count == 1 && cluster.singleAircraft) {
      uint32_t addr = cluster.singleAircraft->addr;
      auto* viewState = viewStates_.get(addr);

      // Skip label if in unmerge animation
      if (viewState && viewState->clusterState == ClusterState::SOLO &&
          viewState->animProgress < 1.0f) {
        continue;
      }

      drawPlaneText(ctx, cluster.singleAircraft, selectedAircraft);
    }
  }
}

void AircraftRenderer::drawPlaneText(const RenderContext& ctx, Aircraft* p,
                                     Aircraft* selectedAircraft) {
  auto& viewState = getOrCreateViewState(ctx, p);

  // Update label text
  viewState.label->update(p->flight, p->altitude, p->speed);

  // Draw label
  viewState.label->draw(ctx.renderer, (p == selectedAircraft), labelConfig_.showLabels,
                        viewState.screenX, viewState.screenY);
}

void AircraftRenderer::drawTrails(const RenderContext& ctx, const AircraftList& aircraftList,
                                  const MapView& mapView, int left, int top, int right, int bottom) {
  int currentX, currentY, prevX, prevY;
  float dx, dy;

  for (const auto& aircraft : aircraftList) {
    if (aircraft->positionHistory.empty()) continue;

    if (isInMultiPlaneCluster(aircraft->addr)) continue;

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
      if (currentOOB && prevOOB) continue;

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
  // Build neighbor list
  auto neighbors = buildNeighborList(aircraftList);

  // Build spatial grid for efficient neighbor lookups
  // Cell size of 100px is larger than typical label interaction distance
  labelSpatialGrid_.build(neighbors, labelConfig_.screenWidth, labelConfig_.screenHeight, 100.0f);

  // Clear acceleration
  for (const auto& aircraft : aircraftList) {
    auto* viewState = viewStates_.get(aircraft->addr);
    if (viewState && viewState->label) {
      viewState->label->clearAcceleration();
    }
  }

  // Calculate forces using spatial grid for efficient neighbor queries
  for (const auto& aircraft : aircraftList) {
    auto* viewState = viewStates_.get(aircraft->addr);
    if (viewState && viewState->label) {
      // Query spatial grid for nearby neighbors only
      labelSpatialGrid_.getNearbyNeighbors(
          viewState->label->getX(), viewState->label->getY(),
          viewState->label->getWidth(), viewState->label->getHeight(),
          neighbors, nearbyNeighborsTemp_);

      // Use optimized path with pre-filtered neighbors
      viewState->label->calculateForcesFromNearby(nearbyNeighborsTemp_, neighbors, labelConfig_,
                                                  viewState->screenX, viewState->screenY);
    }
  }

  // Apply forces
  for (const auto& aircraft : aircraftList) {
    auto* viewState = viewStates_.get(aircraft->addr);
    if (viewState && viewState->label) {
      viewState->label->applyForces();
    }
  }
}

void AircraftRenderer::moveLabels(float dx, float dy) {
  for (auto& [addr, state] : viewStates_) {
    if (state.label) {
      state.label->move(dx, dy);
    }
  }
}

void AircraftRenderer::syncLabelsToAircraft(AircraftList& aircraftList) {
  for (const auto& aircraft : aircraftList) {
    auto* viewState = viewStates_.get(aircraft->addr);
    if (viewState && viewState->label) {
      viewState->label->syncToAircraftPosition(viewState->screenX, viewState->screenY);
    }
  }
}

// State machine: update cluster state for each aircraft based on spatial clustering result
void AircraftRenderer::updateClusterStates(const RenderContext& /* ctx */,
                                            const AircraftList& aircraftList) {
  using ui::ClusterState;

  for (const auto& aircraft : aircraftList) {
    auto* viewState = viewStates_.get(aircraft->addr);
    if (!viewState) continue;

    // Check if spatially in a multi-plane cluster this frame
    bool spatiallyInCluster = isInMultiPlaneCluster(aircraft->addr);

    // Advance the state machine
    switch (viewState->clusterState) {
      case ClusterState::SOLO:
        if (spatiallyInCluster) {
          // Start merge hysteresis
          viewState->clusterState = ClusterState::MERGE_PENDING;
          viewState->stateFrameCount = 0;
          // Find a cluster-mate to use as anchor
          for (const auto& cluster : onMapClusters_) {
            if (cluster.count > 1 && cluster.memberAddrs.count(aircraft->addr) > 0) {
              for (uint32_t otherAddr : cluster.memberAddrs) {
                if (otherAddr != aircraft->addr) {
                  viewState->clusterAnchorAddr = otherAddr;
                  break;
                }
              }
              break;
            }
          }
          if (viewState->clusterAnchorAddr == 0) {
            for (const auto& cluster : offMapClusters_) {
              if (cluster.count > 1 && cluster.memberAddrs.count(aircraft->addr) > 0) {
                for (uint32_t otherAddr : cluster.memberAddrs) {
                  if (otherAddr != aircraft->addr) {
                    viewState->clusterAnchorAddr = otherAddr;
                    break;
                  }
                }
                break;
              }
            }
          }
        }
        break;

      case ClusterState::MERGE_PENDING:
        viewState->stateFrameCount++;
        if (!spatiallyInCluster) {
          // Left cluster before hysteresis complete - back to SOLO
          viewState->clusterState = ClusterState::SOLO;
          viewState->stateFrameCount = 0;
        } else if (viewState->stateFrameCount >= HYSTERESIS_FRAMES) {
          // Hysteresis complete - transition to CLUSTERED with animation
          viewState->clusterState = ClusterState::CLUSTERED;
          viewState->stateFrameCount = 0;
          viewState->animProgress = 0.0f;
          highFramerate = true;

          // Collapse label during merge
          if (viewState->label) {
            viewState->label->forceCollapse();
          }
        }
        break;

      case ClusterState::CLUSTERED:
        if (!spatiallyInCluster) {
          // Start unmerge hysteresis
          viewState->clusterState = ClusterState::UNMERGE_PENDING;
          viewState->stateFrameCount = 0;
        }
        break;

      case ClusterState::UNMERGE_PENDING:
        viewState->stateFrameCount++;
        if (spatiallyInCluster) {
          // Back in cluster before hysteresis complete - stay CLUSTERED
          viewState->clusterState = ClusterState::CLUSTERED;
          viewState->stateFrameCount = 0;
        } else if (viewState->stateFrameCount >= HYSTERESIS_FRAMES) {
          // Hysteresis complete - transition to SOLO with animation
          viewState->clusterState = ClusterState::SOLO;
          viewState->stateFrameCount = 0;
          viewState->animProgress = 0.0f;
          highFramerate = true;

          // Reset and expand label on unmerge
          if (viewState->label) {
            viewState->label->resetToAircraftPosition(viewState->screenX, viewState->screenY);
            viewState->label->forceExpand();
          }
        }
        break;
    }
  }
}

// Draw animations for aircraft transitioning between cluster states
void AircraftRenderer::drawClusterAnimations(const RenderContext& ctx,
                                              const AircraftList& aircraftList) {
  using ui::ClusterState;
  const float animStep = 1.0f / static_cast<float>(ANIMATION_FRAMES);

  for (const auto& aircraft : aircraftList) {
    auto* viewState = viewStates_.get(aircraft->addr);
    if (!viewState) continue;

    // Animation happens when animProgress < 1.0 and state is SOLO (after unmerge)
    // or CLUSTERED (after merge)
    if (viewState->animProgress >= 1.0f) continue;

    // Advance animation progress
    viewState->animProgress = std::min(viewState->animProgress + animStep, 1.0f);

    float aircraftX = static_cast<float>(viewState->screenX);
    float aircraftY = static_cast<float>(viewState->screenY);

    // Find cluster center for animation
    float clusterX = aircraftX;
    float clusterY = aircraftY;

    if (viewState->clusterAnchorAddr != 0) {
      // Try to find cluster center from current clusters
      if (!findClusterCenter(onMapClusters_, viewState->clusterAnchorAddr, clusterX, clusterY)) {
        float edgeX, edgeY, dirX, dirY;
        if (findOffMapClusterPosition(viewState->clusterAnchorAddr, edgeX, edgeY, dirX, dirY)) {
          clusterX = static_cast<float>(ctx.screenWidth >> 1) + edgeX;
          clusterY = static_cast<float>(ctx.screenHeight >> 1) + edgeY;
        } else {
          // Fall back to anchor's screen position
          auto* anchorState = viewStates_.get(viewState->clusterAnchorAddr);
          if (anchorState) {
            clusterX = static_cast<float>(anchorState->screenX);
            clusterY = static_cast<float>(anchorState->screenY);
          }
        }
      }
    }

    float easedProgress = easeOutQuad(viewState->animProgress);
    float animX, animY;

    // Determine animation direction based on state
    if (viewState->clusterState == ClusterState::CLUSTERED) {
      // Merge animation: aircraft → cluster
      animX = lerp(aircraftX, clusterX, easedProgress);
      animY = lerp(aircraftY, clusterY, easedProgress);
    } else {
      // Unmerge animation: cluster → aircraft
      animX = lerp(clusterX, aircraftX, easedProgress);
      animY = lerp(clusterY, aircraftY, easedProgress);
    }

    // Draw animation circle
    circleRGBA(ctx.renderer, static_cast<int>(animX), static_cast<int>(animY),
               4 * ctx.uiScale,
               ctx.style->planeColor.r, ctx.style->planeColor.g, ctx.style->planeColor.b,
               127);

    if (viewState->animProgress < 1.0f) {
      highFramerate = true;
    }
  }
}

bool AircraftRenderer::findClusterCenter(
    const std::vector<OnMapCluster>& clusters, uint32_t memberAddr,
    float& outX, float& outY) const {
  for (const auto& cluster : clusters) {
    if (cluster.memberAddrs.count(memberAddr) > 0) {
      outX = cluster.centerX;
      outY = cluster.centerY;
      return true;
    }
  }
  return false;
}

bool AircraftRenderer::findOffMapClusterPosition(uint32_t memberAddr, float& outEdgeX, float& outEdgeY,
                                                  float& outDirX, float& outDirY) const {
  for (const auto& cluster : offMapClusters_) {
    if (cluster.memberAddrs.count(memberAddr) > 0) {
      outEdgeX = cluster.edgeX;
      outEdgeY = cluster.edgeY;
      outDirX = cluster.dirX;
      outDirY = cluster.dirY;
      return true;
    }
  }
  return false;
}

bool AircraftRenderer::isInMultiPlaneCluster(uint32_t addr) const {
  for (const auto& cluster : onMapClusters_) {
    if (cluster.count > 1 && cluster.memberAddrs.count(addr) > 0) {
      return true;
    }
  }
  for (const auto& cluster : offMapClusters_) {
    if (cluster.count > 1 && cluster.memberAddrs.count(addr) > 0) {
      return true;
    }
  }
  return false;
}

bool AircraftRenderer::isOffMap(int x, int y, int screenWidth, int screenHeight) const {
  // Basic screen bounds check
  if (x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) {
    return true;
  }

  // Status bar region (bottom-left area)
  if (uiStatusBarTopY_ > 0 && y > uiStatusBarTopY_ && x < uiStatusBarRightX_) {
    return true;
  }

  // Scale bar region (top-left area)
  if (scaleBarBottomY_ > 0 && y < scaleBarBottomY_ && x < scaleBarRightX_) {
    return true;
  }

  return false;
}

}  // namespace viz1090
