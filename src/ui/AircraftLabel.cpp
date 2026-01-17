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

    SDL_Rect currentRect = getFullRect(labelLevelVal);

    float width_proportion = (currentRect.w + neighbor.w) / std::fabs(x - neighbor.x);
    float height_proportion = (currentRect.h + neighbor.h) / std::fabs(y - neighbor.y);

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
  float p_left = x;
  float p_right = x + w;
  float p_top = y;
  float p_bottom = y + h;

  float boxmid_x = (p_left + p_right) / 2.0f;
  float boxmid_y = (p_top + p_bottom) / 2.0f;

  float offset_x = boxmid_x - static_cast<float>(aircraftScreenX);
  float offset_y = boxmid_y - static_cast<float>(aircraftScreenY);

  float target_length_x = attachment_dist + w / 2.0f;
  float target_length_y = attachment_dist + h / 2.0f;

  // stay icon_dist away from own icon
  ddx -= sign(offset_x) * attachment_force * (std::fabs(offset_x) - target_length_x);
  ddy -= sign(offset_y) * attachment_force * (std::fabs(offset_y) - target_length_y);

  // screen edge
  if (p_left < edge_margin) {
    ddx += boundary_force * (edge_margin - p_left);
  }

  if (p_right > screen_width - edge_margin) {
    ddx += boundary_force * (screen_width - edge_margin - p_right);
  }

  if (p_top < edge_margin) {
    ddy += boundary_force * (edge_margin - p_top);
  }

  // Bottom edge boundary - respect UI status bar bounds
  float effectiveBottomEdge = static_cast<float>(screen_height);
  if (config.uiStatusBarTopY > 0 && p_left < static_cast<float>(config.uiStatusBarRightX)) {
    // Label is in the region where status bar exists - use status bar top as boundary
    effectiveBottomEdge = static_cast<float>(config.uiStatusBarTopY);
  }

  if (p_bottom > effectiveBottomEdge - edge_margin) {
    ddy += boundary_force * (effectiveBottomEdge - edge_margin - p_bottom);
  }

  float all_x = 0;
  float all_y = 0;
  int count = 0;

  // check against other labels
  for (const auto& neighbor : neighbors) {
    if (neighbor.addr == aircraftAddr_) {
      continue;
    }

    float check_left = neighbor.x;
    float check_right = neighbor.x + neighbor.w;
    float check_top = neighbor.y;
    float check_bottom = neighbor.y + neighbor.h;

    float checkboxmid_x = (check_left + check_right) / 2.0f;
    float checkboxmid_y = (check_top + check_bottom) / 2.0f;

    bool overlap = true;

    if (p_left >= check_right + 10 || check_left >= p_right + 10)
      overlap = false;

    if (p_top >= check_bottom + 10 || check_top >= p_bottom + 10)
      overlap = false;

    if (overlap) {
      float td = std::fabs(p_top - check_bottom);
      float bd = std::fabs(p_bottom - check_top);
      float ld = std::fabs(p_left - check_right);
      float rd = std::fabs(p_right - check_left);

      float x_mag, y_mag;

      if (boxmid_y > checkboxmid_y) {
        y_mag = check_bottom - p_top + 10;
      } else {
        y_mag = check_top - p_bottom - 10;
        td = bd;
      }

      if (boxmid_x > checkboxmid_x) {
        x_mag = check_right - p_left + 10;
      } else {
        x_mag = check_left - p_right - 10;
        ld = rd;
      }

      if (td < ld) {
        x_mag = 0;
      } else {
        y_mag = 0;
      }

      ddx += label_force * x_mag;
      ddy += label_force * y_mag;
    }

    // stay at least label_dist away from other icons
    float check_x = static_cast<float>(neighbor.aircraftX);
    float check_y = static_cast<float>(neighbor.aircraftY);

    if (p_right >= check_x && check_x >= p_left && p_bottom >= check_y && check_y >= p_top) {
      float x_mag, y_mag;

      if (boxmid_x - check_x > 0) {
        x_mag = check_x - p_left + 10;
      } else {
        x_mag = check_x - p_right - 10;
      }

      if (boxmid_y - check_y > 0) {
        y_mag = check_y - p_top + 10;
      } else {
        y_mag = check_y - p_bottom - 10;
      }

      ddx += icon_force * x_mag;
      ddy += icon_force * y_mag;
    }

    all_x += sign(boxmid_x - checkboxmid_x);
    all_y += sign(boxmid_y - checkboxmid_y);

    count++;
  }

  // move away from others
  if (count > 0) {
    ddx += density_force * all_x / static_cast<float>(count);
    ddy += density_force * all_y / static_cast<float>(count);
  }

  float level_rate = 0.25f;

  float randtime = 5000.0f + 5000.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  if (config.densityChanged || elapsed(lastLevelChange) > randtime) {
    if (labelLevel < -1.2f + config.densityMultiplier * calculateDensity(neighbors, static_cast<int>(labelLevel) - 1)) {
      if (labelLevel <= 2) {
        if (std::ceil(labelLevel) - labelLevel <= level_rate) {
          labelLevel += 0.5f;
        }

        labelLevel += level_rate;
        isChanging = true;
        lastLevelChange = now();
      }
    } else if (labelLevel > 1.2f + config.densityMultiplier * calculateDensity(neighbors, static_cast<int>(labelLevel) + 1)) {
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

  // add drag force (using implicit velocity from Verlet)
  float vel_x = x - prev_x;
  float vel_y = y - prev_y;
  ddx -= drag_force * vel_x * vel_x * sign(vel_x);
  ddy -= drag_force * vel_y * vel_y * sign(vel_y);
}

void
AircraftLabel::applyForces() {
  // Verlet integration with oscillation detection
  float vel_x = x - prev_x;
  float vel_y = y - prev_y;

  // Base damping - fairly gentle to allow smooth movement
  float base_damp = 0.8f;

  // Detect small-amplitude oscillation conditions
  constexpr float oscillation_threshold = 1.5f;  // pixels

  // X-axis oscillation check
  if (std::fabs(vel_x) < oscillation_threshold && vel_x * ddx < 0) {
    vel_x = 0;
    ddx *= 0.5f;
  } else {
    vel_x *= base_damp;
  }

  // Y-axis oscillation check
  if (std::fabs(vel_y) < oscillation_threshold && vel_y * ddy < 0) {
    vel_y = 0;
    ddy *= 0.5f;
  } else {
    vel_y *= base_damp;
  }

  // Apply velocity limit
  if (std::fabs(vel_x) > velocity_limit) {
    vel_x = sign(vel_x) * velocity_limit;
  }
  if (std::fabs(vel_y) > velocity_limit) {
    vel_y = sign(vel_y) * velocity_limit;
  }

  // Calculate new position
  float new_x = x + vel_x + ddx;
  float new_y = y + vel_y + ddy;

  // Update previous position to current before moving
  prev_x = x;
  prev_y = y;

  // Update current position
  x = new_x;
  y = new_y;

  // Check if still changing
  if (std::fabs(vel_x) > 0.01f || std::fabs(vel_y) > 0.01f ||
      std::fabs(ddx) > 0.01f || std::fabs(ddy) > 0.01f) {
    isChanging = true;
  }

  // Handle NaN
  if (std::isnan(x)) {
    x = 0;
    prev_x = 0;
  }
  if (std::isnan(y)) {
    y = 0;
    prev_y = 0;
  }
}

void
AircraftLabel::move(float dx, float dy) {
  // Move both current and previous position to preserve implicit velocity
  x += dx;
  y += dy;
  prev_x += dx;
  prev_y += dy;
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
  prev_x = targetX;
  prev_y = targetY;
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

    int ix = static_cast<int>(x);
    int iy = static_cast<int>(y);
    int iw = static_cast<int>(w);
    int ih = static_cast<int>(h);

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

  int ix = static_cast<int>(x);
  int iy = static_cast<int>(y);

  if (labelLevel < 2 || selected) {
    SDL_Color drawColor = style.labelColor;
    drawColor.a = static_cast<int>(255.0f * opacity);

    flightLabel.setColor(drawColor);
    flightLabel.setPosition(ix, iy);
    flightLabel.draw(renderer);
    outRect = flightLabel.getRect();

    totalWidth = std::max(totalWidth, outRect.w);
    totalHeight += outRect.h;
  }

  if (labelLevel < 1 || selected) {
    SDL_Color drawColor = style.subLabelColor;
    drawColor.a = static_cast<int>(255.0f * opacity);

    altitudeLabel.setColor(drawColor);
    altitudeLabel.setPosition(ix, iy + totalHeight);
    altitudeLabel.draw(renderer);
    outRect = altitudeLabel.getRect();

    totalWidth = std::max(totalWidth, outRect.w);
    totalHeight += outRect.h;

    speedLabel.setColor(drawColor);
    speedLabel.setPosition(ix, iy + totalHeight);
    speedLabel.draw(renderer);
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
      prev_x(0),
      prev_y(20.0f),
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

}  // namespace ui
}  // namespace viz1090
