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

#include "ui/AircraftLabel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "SDL2/SDL2_gfxPrimitives.h"

namespace viz1090 {
namespace ui {

using fmilliseconds = std::chrono::duration<float, std::milli>;

static std::chrono::high_resolution_clock::time_point
now() {
  return std::chrono::high_resolution_clock::now();
}

static float
elapsed(std::chrono::high_resolution_clock::time_point ref) {
  return (fmilliseconds{now() - ref}).count();
}

static float
sign(float x) {
  return static_cast<float>((x > 0) - (x < 0));
}

void AircraftLabel::forceCollapse() {
  labelLevel = 3.0f;
  lastLevelChange = now();
}

void AircraftLabel::forceExpand() {
  labelLevel = 0.0f;
  lastLevelChange = now();
}

SDL_Rect
AircraftLabel::getFullRect(int labelLevelVal) {
  SDL_Rect rect = {static_cast<int>(x), static_cast<int>(y), 0, 0};

  SDL_Rect currentRect;

  if (labelLevelVal < 2) {
    currentRect = speedLabel.getRect();

    rect.w = std::max(rect.w, currentRect.w);
    rect.h += currentRect.h;
  }

  if (labelLevelVal < 1) {
    currentRect = altitudeLabel.getRect();

    rect.w = std::max(rect.w, currentRect.w);
    rect.h += currentRect.h;

    currentRect = speedLabel.getRect();

    rect.w = std::max(rect.w, currentRect.w);
    rect.h += currentRect.h;
  }

  return rect;
}

void
AircraftLabel::update(const char* flight, int altitude, int speed) {
  char flightBuf[17] = "";
  std::snprintf(flightBuf, 17, " %s", flight);

  std::string flightString = flightBuf;
  flightString.erase(std::remove_if(flightString.begin(), flightString.end(), isspace),
                     flightString.end());

  flightLabel.setText(flightString);

  char alt[10] = "";
  if (metric) {
    std::snprintf(alt, 10, "%d m", static_cast<int>(altitude / 3.2828));
  } else {
    std::snprintf(alt, 10, "%d'", altitude);
  }

  altitudeLabel.setText(alt);

  char speedBuf[10] = "";
  if (metric) {
    std::snprintf(speedBuf, 10, "%d km/h", static_cast<int>(speed * 1.852));
  } else {
    std::snprintf(speedBuf, 10, "%d mph", speed);
  }

  speedLabel.setText(speedBuf);
}

void
AircraftLabel::clearAcceleration() {
  ddx = 0;
  ddy = 0;
}

float
AircraftLabel::calculateDensity(const std::vector<LabelNeighbor>& neighbors, int labelLevelVal) {
  float density_max = 0;

  SDL_Rect currentRect = getFullRect(labelLevelVal);

  for (const auto& neighbor : neighbors) {
    if (neighbor.addr == aircraftAddr_) {
      continue;
    }

    if (neighbor.x + neighbor.w < 0) {
      continue;
    }

    if (neighbor.y + neighbor.h < 0) {
      continue;
    }

    if (neighbor.x > screen_width) {
      continue;
    }

    if (neighbor.y > screen_height) {
      continue;
    }

    float dx = std::fabs(x - neighbor.x);
    float dy = std::fabs(y - neighbor.y);

    // Avoid division by very small numbers
    if (dx < 1.0f) dx = 1.0f;
    if (dy < 1.0f) dy = 1.0f;

    float width_proportion = (currentRect.w + neighbor.w) / dx;
    float height_proportion = (currentRect.h + neighbor.h) / dy;

    float density = width_proportion * height_proportion;

    if (density > density_max) {
      density_max = density;
    }
  }

  return density_max;
}

float
AircraftLabel::calculateDensityFromNearby(const std::vector<const LabelNeighbor*>& nearbyNeighbors, int labelLevelVal) {
  float density_max = 0;

  SDL_Rect currentRect = getFullRect(labelLevelVal);

  for (const auto* neighbor : nearbyNeighbors) {
    if (neighbor->addr == aircraftAddr_) {
      continue;
    }

    if (neighbor->x + neighbor->w < 0) {
      continue;
    }

    if (neighbor->y + neighbor->h < 0) {
      continue;
    }

    if (neighbor->x > screen_width) {
      continue;
    }

    if (neighbor->y > screen_height) {
      continue;
    }

    float dx = std::fabs(x - neighbor->x);
    float dy = std::fabs(y - neighbor->y);

    // Avoid division by very small numbers
    if (dx < 1.0f) dx = 1.0f;
    if (dy < 1.0f) dy = 1.0f;

    float width_proportion = (currentRect.w + neighbor->w) / dx;
    float height_proportion = (currentRect.h + neighbor->h) / dy;

    float density = width_proportion * height_proportion;

    if (density > density_max) {
      density_max = density;
    }
  }

  return density_max;
}

void
AircraftLabel::calculateForces(const std::vector<LabelNeighbor>& neighbors,
                               const LabelConfig& config,
                               int aircraftScreenX, int aircraftScreenY) {
  // Wrapper: convert value vector to pointer vector and delegate
  std::vector<const LabelNeighbor*> ptrs;
  ptrs.reserve(neighbors.size());
  for (const auto& n : neighbors) {
    ptrs.push_back(&n);
  }
  calculateSoftForces(ptrs, config, aircraftScreenX, aircraftScreenY);
}

void
AircraftLabel::calculateForcesFromNearby(const std::vector<const LabelNeighbor*>& nearbyNeighbors,
                                         const std::vector<LabelNeighbor>& /* allNeighbors */,
                                         const LabelConfig& config,
                                         int aircraftScreenX, int aircraftScreenY) {
  // Wrapper: delegate to new soft forces method
  calculateSoftForces(nearbyNeighbors, config, aircraftScreenX, aircraftScreenY);
}

void
AircraftLabel::calculateSoftForces(const std::vector<const LabelNeighbor*>& nearbyNeighbors,
                                   const LabelConfig& config,
                                   int aircraftScreenX, int aircraftScreenY) {
  float p_left = x;
  float p_right = x + w;
  float p_top = y;
  float p_bottom = y + h;

  float boxmid_x = (p_left + p_right) / 2.0f;
  float boxmid_y = (p_top + p_bottom) / 2.0f;

  // 1. Critically-damped attachment spring
  // Target position: offset from aircraft in the quadrant the label is already in
  float offset_x = boxmid_x - static_cast<float>(aircraftScreenX);
  float offset_y = boxmid_y - static_cast<float>(aircraftScreenY);

  float target_length_x = attachment_dist + w / 2.0f;
  float target_length_y = attachment_dist + h / 2.0f;

  // Default to positive quadrant if label is exactly on aircraft
  float sx = (offset_x >= 0) ? 1.0f : -1.0f;
  float sy = (offset_y >= 0) ? 1.0f : -1.0f;

  float target_x = static_cast<float>(aircraftScreenX) + sx * target_length_x;
  float target_y = static_cast<float>(aircraftScreenY) + sy * target_length_y;

  // F = k * (target - pos) - 2*sqrt(k) * vel
  float critical_damp = 2.0f * std::sqrt(attachment_k);
  ddx += attachment_k * (target_x - boxmid_x) - critical_damp * vel_x;
  ddy += attachment_k * (target_y - boxmid_y) - critical_damp * vel_y;

  // 2. Boundary springs
  if (p_left < edge_margin) {
    ddx += boundary_k * (edge_margin - p_left);
  }

  if (p_right > screen_width - edge_margin) {
    ddx += boundary_k * (screen_width - edge_margin - p_right);
  }

  if (p_top < edge_margin) {
    ddy += boundary_k * (edge_margin - p_top);
  }

  // Bottom edge boundary - respect UI status bar bounds
  float effectiveBottomEdge = static_cast<float>(screen_height);
  if (config.uiStatusBarTopY > 0 && p_left < static_cast<float>(config.uiStatusBarRightX)) {
    effectiveBottomEdge = static_cast<float>(config.uiStatusBarTopY);
  }

  if (p_bottom > effectiveBottomEdge - edge_margin) {
    ddy += boundary_k * (effectiveBottomEdge - edge_margin - p_bottom);
  }

  // 3. Soft density pressure — direction away from neighbors, weighted by inverse distance
  float all_x = 0;
  float all_y = 0;
  int count = 0;

  for (const auto* neighbor : nearbyNeighbors) {
    if (neighbor->addr == aircraftAddr_) {
      continue;
    }

    float checkboxmid_x = neighbor->x + neighbor->w / 2.0f;
    float checkboxmid_y = neighbor->y + neighbor->h / 2.0f;

    float dx = boxmid_x - checkboxmid_x;
    float dy = boxmid_y - checkboxmid_y;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 1.0f) dist = 1.0f;

    float invDist = 1.0f / dist;
    all_x += sign(dx) * invDist;
    all_y += sign(dy) * invDist;
    count++;
  }

  if (count > 0) {
    ddx += density_force * all_x / static_cast<float>(count);
    ddy += density_force * all_y / static_cast<float>(count);
  }

  // 4. Label level/collapsing logic (preserved)
  float level_rate = 0.25f;

  float randtime = 5000.0f + 5000.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  if (config.densityChanged || elapsed(lastLevelChange) > randtime) {
    if (labelLevel < -1.2f + config.densityMultiplier * calculateDensityFromNearby(nearbyNeighbors, static_cast<int>(labelLevel) - 1)) {
      if (labelLevel <= 2) {
        if (std::ceil(labelLevel) - labelLevel <= level_rate) {
          labelLevel += 0.5f;
        }

        labelLevel += level_rate;
        isChanging = true;
        lastLevelChange = now();
      }
    } else if (labelLevel > 1.2f + config.densityMultiplier * calculateDensityFromNearby(nearbyNeighbors, static_cast<int>(labelLevel) + 1)) {
      if (labelLevel >= 0) {
        if (labelLevel - std::floor(labelLevel) <= level_rate) {
          labelLevel -= 0.5f;
        }

        labelLevel -= level_rate;
        isChanging = true;
        lastLevelChange = now();
      }
    }
  }
}

void
AircraftLabel::applyForces() {
  // Wrapper: delegate to new integration method
  integrateSemiImplicitEuler();
}

void
AircraftLabel::integrateSemiImplicitEuler() {
  // Semi-implicit Euler: update velocity first, then position
  vel_x = vel_x * damping + ddx;
  vel_y = vel_y * damping + ddy;

  // Clamp velocity
  if (std::fabs(vel_x) > velocity_limit) {
    vel_x = sign(vel_x) * velocity_limit;
  }
  if (std::fabs(vel_y) > velocity_limit) {
    vel_y = sign(vel_y) * velocity_limit;
  }

  // Update position
  x += vel_x;
  y += vel_y;

  // Track oscillation (direction changes for instrumentation)
  float signX = sign(vel_x);
  float signY = sign(vel_y);

  bool dirChangedX = (signX != 0 && lastSignX != 0 && signX != lastSignX);
  bool dirChangedY = (signY != 0 && lastSignY != 0 && signY != lastSignY);

  // Update circular buffer and running count
  bool oldX = dirHistoryX[dirHistoryIndex];
  bool oldY = dirHistoryY[dirHistoryIndex];
  dirHistoryX[dirHistoryIndex] = dirChangedX;
  dirHistoryY[dirHistoryIndex] = dirChangedY;

  dirChangeCountX += (dirChangedX ? 1 : 0) - (oldX ? 1 : 0);
  dirChangeCountY += (dirChangedY ? 1 : 0) - (oldY ? 1 : 0);

  dirHistoryIndex = (dirHistoryIndex + 1) & 15;  // % 16

  if (signX != 0) lastSignX = signX;
  if (signY != 0) lastSignY = signY;

  // Check if still changing
  if (std::fabs(vel_x) > 0.01f || std::fabs(vel_y) > 0.01f ||
      std::fabs(ddx) > 0.01f || std::fabs(ddy) > 0.01f) {
    isChanging = true;
  }

  // Handle NaN
  if (std::isnan(x)) { x = 0; vel_x = 0; }
  if (std::isnan(y)) { y = 0; vel_y = 0; }
}

void
AircraftLabel::move(float dx, float dy) {
  // Shift position only — velocity unchanged (panning is frame-of-reference shift)
  x += dx;
  y += dy;
}

void
AircraftLabel::syncToAircraftPosition(int aircraftScreenX, int aircraftScreenY) {
  // Calculate how much the aircraft's screen position has changed
  float aircraft_dx = static_cast<float>(aircraftScreenX) - lastAircraftX;
  float aircraft_dy = static_cast<float>(aircraftScreenY) - lastAircraftY;

  // Move the label by the same amount
  if (aircraft_dx != 0.0f || aircraft_dy != 0.0f) {
    move(aircraft_dx, aircraft_dy);
  }

  // Update the last known position
  lastAircraftX = static_cast<float>(aircraftScreenX);
  lastAircraftY = static_cast<float>(aircraftScreenY);
}

void
AircraftLabel::resetToAircraftPosition(int aircraftScreenX, int aircraftScreenY) {
  // Snap label directly to nominal position near aircraft
  float targetOffsetX = attachment_dist + w / 2.0f;
  float targetOffsetY = attachment_dist + h / 2.0f;
  float targetX = static_cast<float>(aircraftScreenX) + targetOffsetX;
  float targetY = static_cast<float>(aircraftScreenY) + targetOffsetY;

  x = targetX;
  y = targetY;
  vel_x = 0.0f;
  vel_y = 0.0f;
  ddx = 0.0f;
  ddy = 0.0f;

  // Update last known aircraft position
  lastAircraftX = static_cast<float>(aircraftScreenX);
  lastAircraftY = static_cast<float>(aircraftScreenY);
}

void
AircraftLabel::draw(SDL_Renderer* renderer, bool selected, bool showLabels,
                    int aircraftScreenX, int aircraftScreenY) {
  // Skip drawing if labels are globally disabled (unless this aircraft is selected)
  if (!showLabels && !selected) {
    return;
  }

  if (x == 0 || y == 0) {
    return;
  }

  // Check if label is too far from aircraft - reset if more than 50% of screen dimension
  float distX = std::fabs(x - static_cast<float>(aircraftScreenX));
  float distY = std::fabs(y - static_cast<float>(aircraftScreenY));
  float maxDist = static_cast<float>(std::min(screen_width, screen_height)) * 0.5f;
  if (distX > maxDist || distY > maxDist) {
    resetToAircraftPosition(aircraftScreenX, aircraftScreenY);
  }

  int totalWidth = 0;
  int totalHeight = 0;

  int margin = 4;

  SDL_Rect outRect;

  if (opacity == 0 && labelLevel < 2) {
    target_opacity = 1.0f;
  }

  if (opacity > 0 && labelLevel >= 2) {
    target_opacity = 0.0f;
  }

  opacity += 0.15f * (target_opacity - opacity);

  if (opacity < 0.005f) {
    opacity = 0;
  }

  if (w != 0 && h != 0 && opacity > 0) {
    SDL_Color drawColor = style.labelLineColor;

    drawColor.a = static_cast<int>(255.0f * opacity);

    if (selected) {
      drawColor = style.selectedColor;
    }

    int tick = 4;

    int anchor_x, anchor_y, exit_x, exit_y;

    if (x + w / 2 > aircraftScreenX) {
      anchor_x = static_cast<int>(x);
    } else {
      anchor_x = static_cast<int>(x + w);
    }

    if (y + h / 2 > aircraftScreenY) {
      anchor_y = static_cast<int>(y) - margin;
    } else {
      anchor_y = static_cast<int>(y + h) + margin;
    }

    if (std::abs(anchor_x - aircraftScreenX) > std::abs(anchor_y - aircraftScreenY)) {
      exit_x = (anchor_x + aircraftScreenX) / 2;
      exit_y = anchor_y;
    } else {
      exit_x = anchor_x;
      exit_y = (anchor_y + aircraftScreenY) / 2;
    }

    Sint16 vx[3] = {static_cast<Sint16>(aircraftScreenX), static_cast<Sint16>(exit_x),
                    static_cast<Sint16>(anchor_x)};

    Sint16 vy[3] = {static_cast<Sint16>(aircraftScreenY), static_cast<Sint16>(exit_y),
                    static_cast<Sint16>(anchor_y)};

    int ix = getRenderX();
    int iy = getRenderY();
    int iw = static_cast<int>(std::round(w));
    int ih = static_cast<int>(std::round(h));

    boxRGBA(renderer, ix, iy, ix + iw, iy + ih, style.labelBackground.r, style.labelBackground.g,
            style.labelBackground.b, drawColor.a);

    bezierRGBA(renderer, vx, vy, 3, 2, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

    lineRGBA(renderer, ix, iy - margin, ix + iw, iy - margin, drawColor.r, drawColor.g, drawColor.b,
             drawColor.a);
    lineRGBA(renderer, ix, iy - margin, ix, iy - margin + tick, drawColor.r, drawColor.g, drawColor.b,
             drawColor.a);

    lineRGBA(renderer, ix + iw, iy - margin, ix + iw, iy - margin + tick, drawColor.r, drawColor.g,
             drawColor.b, drawColor.a);

    lineRGBA(renderer, ix, iy + ih + margin, ix + iw, iy + ih + margin, drawColor.r, drawColor.g,
             drawColor.b, drawColor.a);
    lineRGBA(renderer, ix, iy + ih + margin, ix, iy + ih + margin - tick, drawColor.r, drawColor.g,
             drawColor.b, drawColor.a);

    lineRGBA(renderer, ix + iw, iy + ih + margin, ix + iw, iy + ih + margin - tick, drawColor.r,
             drawColor.g, drawColor.b, drawColor.a);
  }

  int ix = getRenderX();
  int iy = getRenderY();

  // Only draw text if label dimensions are established (prevents flicker on first frame after expand)
  bool dimensionsValid = (w != 0 && h != 0);

  if (labelLevel < 2 || selected) {
    SDL_Color drawColor = style.labelColor;
    drawColor.a = static_cast<int>(255.0f * opacity);

    flightLabel.setColor(drawColor);
    flightLabel.setPosition(ix, iy);
    if (dimensionsValid) {
      flightLabel.draw(renderer);
    }
    outRect = flightLabel.getRect();

    totalWidth = std::max(totalWidth, outRect.w);
    totalHeight += outRect.h;
  }

  if (labelLevel < 1 || selected) {
    SDL_Color drawColor = style.subLabelColor;
    drawColor.a = static_cast<int>(255.0f * opacity);

    altitudeLabel.setColor(drawColor);
    altitudeLabel.setPosition(ix, iy + totalHeight);
    if (dimensionsValid) {
      altitudeLabel.draw(renderer);
    }
    outRect = altitudeLabel.getRect();

    totalWidth = std::max(totalWidth, outRect.w);
    totalHeight += outRect.h;

    speedLabel.setColor(drawColor);
    speedLabel.setPosition(ix, iy + totalHeight);
    if (dimensionsValid) {
      speedLabel.draw(renderer);
    }
    outRect = speedLabel.getRect();

    totalWidth = std::max(totalWidth, outRect.w);
    totalHeight += outRect.h;
  }

  debugLabel.setPosition(ix, iy + totalHeight);
  debugLabel.draw(renderer);

  target_w = static_cast<float>(totalWidth);
  target_h = static_cast<float>(totalHeight);

  w += 0.25f * (target_w - w);
  h += 0.25f * (target_h - h);

  if (w < 0.05f) {
    w = 0;
  }

  if (h < 0.05f) {
    h = 0;
  }

  isChanging = false;
}

AircraftLabel::AircraftLabel(uint32_t aircraftAddr, bool& metric, int screenWidth, int screenHeight,
                             TTF_Font* font, const Style& style)
    : aircraftAddr_(aircraftAddr),
      labelLevel(0),
      metric(metric),
      x(0),
      y(20.0f),
      w(0),
      h(0),
      target_w(0),
      target_h(0),
      vel_x(0.0f),
      vel_y(0.0f),
      ddx(0),
      ddy(0),
      opacity(0.0f),
      target_opacity(0.0f),
      pressure(0),
      screen_width(screenWidth),
      screen_height(screenHeight),
      isChanging(false),
      lastAircraftX(0),
      lastAircraftY(0),
      style(style) {
  flightLabel.setFont(font);
  altitudeLabel.setFont(font);
  speedLabel.setFont(font);
  debugLabel.setFont(font);

  lastLevelChange = now();
}

bool
AircraftLabel::projectAwayFromLabel(float otherX, float otherY, float otherW, float otherH,
                                    float margin, float strength) {
  // Use collision bounds (includes reticle extent) for this label
  float myX = getCollisionX();
  float myY = getCollisionY();
  float myW = getCollisionW();
  float myH = getCollisionH();

  float myRight = myX + myW;
  float myBottom = myY + myH;
  // otherX/Y/W/H are already collision bounds (from neighbor list)
  float otherRight = otherX + otherW;
  float otherBottom = otherY + otherH;

  // No overlap if separated by at least margin
  if (myX >= otherRight + margin || otherX >= myRight + margin ||
      myY >= otherBottom + margin || otherY >= myBottom + margin) {
    return false;
  }

  // Compute penetration depth on each side (including margin)
  float penRight = myRight + margin - otherX;      // push left
  float penLeft = otherRight + margin - myX;        // push right
  float penBottom = myBottom + margin - otherY;     // push up
  float penTop = otherBottom + margin - myY;         // push down

  // Find minimum translation vector
  float minPen = penRight;
  float pushX = -penRight;
  float pushY = 0;

  if (penLeft < minPen) { minPen = penLeft; pushX = penLeft; pushY = 0; }
  if (penBottom < minPen) { minPen = penBottom; pushX = 0; pushY = -penBottom; }
  if (penTop < minPen) { minPen = penTop; pushX = 0; pushY = penTop; }

  x += pushX * strength;
  y += pushY * strength;

  return true;
}

bool
AircraftLabel::projectAwayFromIcon(float iconX, float iconY, float iconRadius) {
  // Use collision bounds (includes reticle extent)
  float myX = getCollisionX();
  float myY = getCollisionY();
  float myW = getCollisionW();
  float myH = getCollisionH();

  // Test if icon center point is inside expanded collision bounds
  float left = myX - iconRadius;
  float right = myX + myW + iconRadius;
  float top = myY - iconRadius;
  float bottom = myY + myH + iconRadius;

  if (iconX < left || iconX > right || iconY < top || iconY > bottom) {
    return false;
  }

  // Penetration from each side
  float penLeft = iconX - left;
  float penRight = right - iconX;
  float penTop = iconY - top;
  float penBottom = bottom - iconY;

  // Find minimum penetration and push along that direction
  float minPen = penLeft;
  float pushX = penLeft;   // push label right
  float pushY = 0;

  if (penRight < minPen) { minPen = penRight; pushX = -penRight; pushY = 0; }
  if (penTop < minPen) { minPen = penTop; pushX = 0; pushY = penTop; }
  if (penBottom < minPen) { minPen = penBottom; pushX = 0; pushY = -penBottom; }

  x += pushX * 0.45f;
  y += pushY * 0.45f;

  return true;
}

float
AircraftLabel::getVelocityMagnitude() const {
  return std::sqrt(vel_x * vel_x + vel_y * vel_y);
}

float
AircraftLabel::getAccelMagnitude() const {
  return std::sqrt(ddx * ddx + ddy * ddy);
}

float
AircraftLabel::getOscillationScore() const {
  // Return fraction of recent frames with direction changes (max of X and Y)
  return static_cast<float>(std::max(dirChangeCountX, dirChangeCountY)) / 16.0f;
}

}  // namespace ui
}  // namespace viz1090
