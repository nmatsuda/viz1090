#include "ui/AircraftLabel.h"
#include "core/Aircraft.h"
#include "core/AircraftList.h"

#include <algorithm>

// Static member definitions
float AircraftLabel::densityMult_ = 0.15f;
bool AircraftLabel::densityChanged_ = false;
bool AircraftLabel::showLabels_ = true;
int AircraftLabel::uiStatusBarTopY_ = 0;
int AircraftLabel::uiStatusBarRightX_ = 0;

void AircraftLabel::setDensityMult(float value) {
  if (value < 0.0f) value = 0.0f;
  if (value > 1.0f) value = 1.0f;
  densityMult_ = value;
}

void AircraftLabel::adjustDensityMult(float delta) {
  setDensityMult(densityMult_ + delta);
  densityChanged_ = true;
}

#include "SDL2/SDL2_gfxPrimitives.h"

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
  return (x > 0) - (x < 0);
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
AircraftLabel::getFullRect(int labelLevel) {
  SDL_Rect rect = {static_cast<int>(x), static_cast<int>(y), 0, 0};

  SDL_Rect currentRect;

  if (labelLevel < 2) {
    currentRect = speedLabel.getRect();

    rect.w = std::max(rect.w, currentRect.w);
    rect.h += currentRect.h;
  }

  if (labelLevel < 1) {
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
AircraftLabel::update() {
  char flight[17] = "";
  snprintf(flight, 17, " %s", p->flight);

  std::string flightString = flight;
  flightString.erase(std::remove_if(flightString.begin(), flightString.end(), isspace),
                     flightString.end());

  flightLabel.setText(flightString);

  char alt[10] = "";
  if (metric) {
    snprintf(alt, 10, "%d m", static_cast<int>(p->altitude / 3.2828));
  } else {
    snprintf(alt, 10, "%d'", p->altitude);
  }

  altitudeLabel.setText(alt);

  char speed[10] = "";
  if (metric) {
    snprintf(speed, 10, "%d km/h", static_cast<int>(p->speed * 1.852));
  } else {
    snprintf(speed, 10, "%d mph", p->speed);
  }

  speedLabel.setText(speed);
}

void
AircraftLabel::clearAcceleration() {
  ddx = 0;
  ddy = 0;
}

float
AircraftLabel::calculateDensity(const AircraftList& aircraftList, int labelLevel) {
  float density_max = 0;

  for (const auto& check_p : aircraftList) {
    if (check_p->addr == p->addr) {
      continue;
    }

    if (!check_p->label) {
      continue;
    }

    if (check_p->label->x + check_p->label->w < 0) {
      continue;
    }

    if (check_p->label->y + check_p->label->h < 0) {
      continue;
    }

    if (check_p->label->x > screen_width) {
      continue;
    }

    if (check_p->label->y > screen_height) {
      continue;
    }

    SDL_Rect currentRect = getFullRect(labelLevel);

    float width_proportion = (currentRect.w + check_p->label->w) / fabs(x - check_p->label->x);
    float height_proportion = (currentRect.h + check_p->label->h) / fabs(y - check_p->label->y);

    float density = width_proportion * height_proportion;

    if (density > density_max) {
      density_max = density;
    }
  }

  return density_max;
}

void
AircraftLabel::calculateForces(const AircraftList& aircraftList) {
  float p_left = static_cast<float>(x);
  float p_right = static_cast<float>(x + w);
  float p_top = static_cast<float>(y);
  float p_bottom = static_cast<float>(y + h);

  float boxmid_x = (p_left + p_right) / 2.0f;
  float boxmid_y = (p_top + p_bottom) / 2.0f;

  float offset_x = boxmid_x - p->x;
  float offset_y = boxmid_y - p->y;

  float target_length_x = attachment_dist + w / 2.0f;
  float target_length_y = attachment_dist + h / 2.0f;

  // stay icon_dist away from own icon

  ddx -= sign(offset_x) * attachment_force * (fabs(offset_x) - target_length_x);
  ddy -= sign(offset_y) * attachment_force * (fabs(offset_y) - target_length_y);

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
  if (uiStatusBarTopY_ > 0 && p_left < static_cast<float>(uiStatusBarRightX_)) {
    // Label is in the region where status bar exists - use status bar top as boundary
    effectiveBottomEdge = static_cast<float>(uiStatusBarTopY_);
  }

  if (p_bottom > effectiveBottomEdge - edge_margin) {
    ddy += boundary_force * (effectiveBottomEdge - edge_margin - p_bottom);
  }

  float all_x = 0;
  float all_y = 0;
  int count = 0;
  // check against other labels

  for (const auto& check_p : aircraftList) {
    if (check_p->addr == p->addr) {
      continue;
    }

    if (!check_p->label) {
      continue;
    }

    float check_left = static_cast<float>(check_p->label->x);
    float check_right = static_cast<float>(check_p->label->x + check_p->label->w);
    float check_top = static_cast<float>(check_p->label->y);
    float check_bottom = static_cast<float>(check_p->label->y + check_p->label->h);

    float checkboxmid_x = static_cast<float>(check_left + check_right) / 2.0f;
    float checkboxmid_y = static_cast<float>(check_top + check_bottom) / 2.0f;

    bool overlap = true;

    if (p_left >= check_right + 10 || check_left >= p_right + 10)
      overlap = false;

    if (p_top >= check_bottom + 10 || check_top >= p_bottom + 10)
      overlap = false;

    if (overlap) {

      float td = fabs(p_top - check_bottom);
      float bd = fabs(p_bottom - check_top);
      float ld = fabs(p_left - check_right);
      float rd = fabs(p_right - check_left);

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

    if (p_right >= check_p->x && check_p->x >= p_left && p_bottom >= check_p->y &&
        check_p->y >= p_top) {
      float x_mag, y_mag;

      if (boxmid_x - check_p->x > 0) {
        x_mag = check_p->x - p_left + 10;
      } else {
        x_mag = check_p->x - p_right - 10;
      }

      if (boxmid_y - check_p->y > 0) {
        y_mag = check_p->y - p_top + 10;
      } else {
        y_mag = check_p->y - p_bottom - 10;
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
    ddx += density_force * all_x / count;
    ddy += density_force * all_y / count;
  }

  float level_rate = 0.25f;

  float randtime = 5000.0f + 5000.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  if (densityChanged_ || elapsed(lastLevelChange) > randtime) {
    if (labelLevel < -1.2f + densityMult_ * calculateDensity(aircraftList, labelLevel - 1)) {
      if (labelLevel <= 2) {
        if (ceil(labelLevel) - labelLevel <= level_rate) {
          labelLevel += 0.5f;
        }

        labelLevel += level_rate;
        isChanging = true;
        lastLevelChange = now();
      }
    } else if (labelLevel > 1.2f + densityMult_ * calculateDensity(aircraftList, labelLevel + 1)) {
      if (labelLevel >= 0) {
        if (labelLevel - floor(labelLevel) <= level_rate) {
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
  //
  // The key insight: small-amplitude oscillations (1-2 pixels) are the problem.
  // Large movements should flow freely. So we detect when:
  // 1. Velocity is small (< 2 pixels)
  // 2. Velocity is about to reverse direction (vel * accel < 0)
  // In that case, we kill the velocity entirely to prevent oscillation.

  float vel_x = x - prev_x;
  float vel_y = y - prev_y;

  // Base damping - fairly gentle to allow smooth movement
  float base_damp = 0.8f;

  // Detect small-amplitude oscillation conditions:
  // If velocity is small AND acceleration opposes it, we're oscillating
  constexpr float oscillation_threshold = 1.5f;  // pixels

  // X-axis oscillation check
  if (fabs(vel_x) < oscillation_threshold && vel_x * ddx < 0) {
    // Small velocity about to reverse - kill it
    vel_x = 0;
    // Also reduce acceleration to let it settle
    ddx *= 0.5f;
  } else {
    vel_x *= base_damp;
  }

  // Y-axis oscillation check
  if (fabs(vel_y) < oscillation_threshold && vel_y * ddy < 0) {
    vel_y = 0;
    ddy *= 0.5f;
  } else {
    vel_y *= base_damp;
  }

  // Apply velocity limit
  if (fabs(vel_x) > velocity_limit) {
    vel_x = sign(vel_x) * velocity_limit;
  }
  if (fabs(vel_y) > velocity_limit) {
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

  // Check if still changing (implicit velocity is non-trivial)
  if (fabs(vel_x) > 0.01f || fabs(vel_y) > 0.01f || fabs(ddx) > 0.01f || fabs(ddy) > 0.01f) {
    isChanging = true;
  }

  // Handle NaN
  if (isnan(x)) {
    x = 0;
    prev_x = 0;
  }
  if (isnan(y)) {
    y = 0;
    prev_y = 0;
  }
}

// SDL_Color signalToColor(int signal) {
//     SDL_Color planeColor;

//     if(signal > 127) {
//         signal = 127;
//     }

//     if(signal < 0) {
//         planeColor = setColor(96, 96, 96);
//     } else {
//         planeColor = setColor(parula[signal][0], parula[signal][1], parula[signal][2]);
//     }

//     return planeColor;
// }
// void View::drawSignalMarks(Aircraft *p, int x, int y) {
//     unsigned char * pSig       = p->signalLevel;
//     unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] +
//                                               pSig[4] + pSig[5] + pSig[6] + pSig[7] + 3) >> 3;

//     SDL_Color barColor = signalToColor(signalAverage);

//     Uint8 seenFade;

//     if(elapsed(p->msSeen) < 1024) {
//         seenFade = (Uint8) (255.0 - elapsed(p->msSeen) / 4.0);

//         circleRGBA(renderer, x + mapFontWidth, y - 5, 2 * screen_uiscale, barColor.r, barColor.g,
//         barColor.b, seenFade);
//     }

//     if(elapsed(p->msSeenLatLon) < 1024) {
//         seenFade = (Uint8) (255.0 - elapsed(p->msSeenLatLon) / 4.0);

//         hlineRGBA(renderer, x + mapFontWidth + 5 * screen_uiscale, x + mapFontWidth + 9 *
//         screen_uiscale, y - 5, barColor.r, barColor.g, barColor.b, seenFade); vlineRGBA(renderer,
//         x + mapFontWidth + 7 * screen_uiscale, y - 2 * screen_uiscale - 5, y + 2 * screen_uiscale
//         - 5, barColor.r, barColor.g, barColor.b, seenFade);
//     }
// }

void
AircraftLabel::move(float dx, float dy) {
  // Move both current and previous position to preserve implicit velocity
  x += dx;
  y += dy;
  prev_x += dx;
  prev_y += dy;
}

void
AircraftLabel::syncToAircraftPosition() {
  // Calculate how much the aircraft's screen position has changed
  float aircraft_dx = static_cast<float>(p->x) - lastAircraftX;
  float aircraft_dy = static_cast<float>(p->y) - lastAircraftY;

  // Move the label by the same amount
  if (aircraft_dx != 0.0f || aircraft_dy != 0.0f) {
    move(aircraft_dx, aircraft_dy);
  }

  // Update the last known position
  lastAircraftX = static_cast<float>(p->x);
  lastAircraftY = static_cast<float>(p->y);
}

void
AircraftLabel::resetToAircraftPosition() {
  // Snap label directly to nominal position near aircraft
  // This is used when a label reappears after being hidden (e.g., unmerging from cluster)
  float targetOffsetX = attachment_dist + w / 2.0f;
  float targetOffsetY = attachment_dist + h / 2.0f;
  float targetX = static_cast<float>(p->x) + targetOffsetX;
  float targetY = static_cast<float>(p->y) + targetOffsetY;

  x = targetX;
  y = targetY;
  prev_x = targetX;
  prev_y = targetY;
  ddx = 0.0f;
  ddy = 0.0f;

  // Update last known aircraft position
  lastAircraftX = static_cast<float>(p->x);
  lastAircraftY = static_cast<float>(p->y);
}

void
AircraftLabel::draw(SDL_Renderer* renderer, bool selected) {
  // Skip drawing if labels are globally disabled (unless this aircraft is selected)
  if (!showLabels_ && !selected) {
    return;
  }

  if (x == 0 || y == 0) {
    return;
  }

  // Check if label is too far from aircraft - reset if more than 50% of screen dimension
  float distX = std::fabs(x - static_cast<float>(p->x));
  float distY = std::fabs(y - static_cast<float>(p->y));
  float maxDist = static_cast<float>(std::min(screen_width, screen_height)) * 0.5f;
  if (distX > maxDist || distY > maxDist) {
    resetToAircraftPosition();
  }

  // char buff[100];
  // snprintf(buff, sizeof(buff), "%f %f", x, y);
  // debugLabel.setText(buff);

  int totalWidth = 0;
  int totalHeight = 0;

  // int margin = 4 * screen_uiscale;

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

    if (x + w / 2 > p->x) {
      anchor_x = x;
    } else {
      anchor_x = x + w;
    }

    if (y + h / 2 > p->y) {
      anchor_y = y - margin;
    } else {
      anchor_y = y + h + margin;
    }

    if (abs(anchor_x - p->x) > abs(anchor_y - p->y)) {
      exit_x = (anchor_x + p->x) / 2;
      exit_y = anchor_y;
    } else {
      exit_x = anchor_x;
      exit_y = (anchor_y + p->y) / 2;
    }

    Sint16 vx[3] = {static_cast<Sint16>(p->x), static_cast<Sint16>(exit_x),
                    static_cast<Sint16>(anchor_x)};

    Sint16 vy[3] = {static_cast<Sint16>(p->y), static_cast<Sint16>(exit_y),
                    static_cast<Sint16>(anchor_y)};

    boxRGBA(renderer, x, y, x + w, y + h, style.labelBackground.r, style.labelBackground.g,
            style.labelBackground.b, drawColor.a);

    //       char buff[100];
    // snprintf(buff, sizeof(buff), "%d", drawColor.a);
    // debugLabel.setText(buff);

    bezierRGBA(renderer, vx, vy, 3, 2, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

    // lineRGBA(renderer, x,y - margin, x + tick, y - margin, drawColor.r, drawColor.g, drawColor.b,
    // drawColor.a);
    lineRGBA(renderer, x, y - margin, x + w, y - margin, drawColor.r, drawColor.g, drawColor.b,
             drawColor.a);
    lineRGBA(renderer, x, y - margin, x, y - margin + tick, drawColor.r, drawColor.g, drawColor.b,
             drawColor.a);

    // lineRGBA(renderer, x + w, y - margin, x + w - tick, y - margin, drawColor.r, drawColor.g,
    // drawColor.b, drawColor.a);
    lineRGBA(renderer, x + w, y - margin, x + w, y - margin + tick, drawColor.r, drawColor.g,
             drawColor.b, drawColor.a);

    // lineRGBA(renderer, x, y + h + margin, x + tick, y + h + margin, drawColor.r, drawColor.g,
    // drawColor.b, drawColor.a);
    lineRGBA(renderer, x, y + h + margin, x + w, y + h + margin, drawColor.r, drawColor.g,
             drawColor.b, drawColor.a);
    lineRGBA(renderer, x, y + h + margin, x, y + h + margin - tick, drawColor.r, drawColor.g,
             drawColor.b, drawColor.a);

    // lineRGBA(renderer, x + w, y + h + margin,x + w - tick, y + h + margin, drawColor.r,
    // drawColor.g, drawColor.b, drawColor.a);
    lineRGBA(renderer, x + w, y + h + margin, x + w, y + h + margin - tick, drawColor.r,
             drawColor.g, drawColor.b, drawColor.a);
  }

  if (labelLevel < 2 || selected) {
    // drawSignalMarks(p, x, y);

    SDL_Color drawColor = style.labelColor;
    drawColor.a = static_cast<int>(255.0f * opacity);

    flightLabel.setColor(drawColor);
    flightLabel.setPosition(x, y);
    flightLabel.draw(renderer);
    // outRect = drawString(flight, x, y, mapBoldFont, drawColor);
    outRect = flightLabel.getRect();

    totalWidth = std::max(totalWidth, outRect.w);
    totalHeight += outRect.h;
  }

  if (labelLevel < 1 || selected) {
    SDL_Color drawColor = style.subLabelColor;
    drawColor.a = static_cast<int>(255.0f * opacity);

    altitudeLabel.setColor(drawColor);
    altitudeLabel.setPosition(x, y + totalHeight);
    altitudeLabel.draw(renderer);
    outRect = altitudeLabel.getRect();

    totalWidth = std::max(totalWidth, outRect.w);
    totalHeight += outRect.h;

    speedLabel.setColor(drawColor);
    speedLabel.setPosition(x, y + totalHeight);
    speedLabel.draw(renderer);
    outRect = speedLabel.getRect();

    totalWidth = std::max(totalWidth, outRect.w);
    totalHeight += outRect.h;
  }

  debugLabel.setPosition(x, y + totalHeight);
  debugLabel.draw(renderer);

  target_w = totalWidth;
  target_h = totalHeight;

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

bool
AircraftLabel::getIsChanging() {
  return isChanging;
}

AircraftLabel::AircraftLabel(Aircraft* p, bool& metric, int screen_width, int screen_height,
                             TTF_Font* font, const Style& style)
    : p(p),
      labelLevel(0),
      metric(metric),
      x(static_cast<float>(p->x)),
      y(static_cast<float>(p->y) + 20.0f),
      w(0),
      h(0),
      target_w(0),
      target_h(0),
      prev_x(static_cast<float>(p->x)),        // Verlet: previous position = current (no initial velocity)
      prev_y(static_cast<float>(p->y) + 20.0f),
      ddx(0),
      ddy(0),
      opacity(0.0f),
      target_opacity(0.0f),
      screen_width(screen_width),
      screen_height(screen_height),
      isChanging(false),
      lastAircraftX(static_cast<float>(p->x)),
      lastAircraftY(static_cast<float>(p->y)),
      style(style) {
  flightLabel.setFont(font);
  altitudeLabel.setFont(font);
  speedLabel.setFont(font);
  debugLabel.setFont(font);

  lastLevelChange = now();
}
