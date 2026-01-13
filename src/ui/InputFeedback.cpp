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

#include "ui/InputFeedback.h"

#include <cmath>

#include "SDL2/SDL2_gfxPrimitives.h"
#include "core/Aircraft.h"
#include "ui/MapView.h"
#include "ui/MathUtils.h"
#include "viz1090/Profiler.h"

namespace viz1090 {

void InputFeedback::draw(const RenderContext& ctx, Aircraft* selectedAircraft) {
  PROFILE_SCOPE("drawClick");

  drawClickRipple(ctx);
  drawSelectionBrackets(ctx, selectedAircraft);
  drawMouse(ctx);
}

void InputFeedback::drawClickRipple(const RenderContext& ctx) {
  if (clickx && clicky) {
    highFramerate = true;

    int radius = static_cast<int>(0.25f * elapsed(clickTime));
    int alpha = 128 - static_cast<int>(0.5f * elapsed(clickTime));
    if (alpha < 0) {
      alpha = 0;
      clickx = 0;
      clicky = 0;
    }

    filledCircleRGBA(ctx.renderer, clickx, clicky, radius, ctx.style->clickColor.r,
                     ctx.style->clickColor.g, ctx.style->clickColor.b,
                     static_cast<Uint8>(alpha));
  }
}

void InputFeedback::drawSelectionBrackets(const RenderContext& ctx,
                                          Aircraft* selectedAircraft) {
  if (!selectedAircraft) {
    return;
  }

  int boxSize;
  if (elapsed(clickTime) < 300) {
    boxSize = static_cast<int>(
        20.0 * (1.0 - (1.0 - elapsed(clickTime) / 300.0) * std::cos(std::sqrt(elapsed(clickTime)))));
  } else {
    boxSize = 20;
  }

  // Top-left corner
  lineRGBA(ctx.renderer, selectedAircraft->x - boxSize, selectedAircraft->y - boxSize,
           selectedAircraft->x - boxSize / 2, selectedAircraft->y - boxSize,
           ctx.style->selectedColor.r, ctx.style->selectedColor.g,
           ctx.style->selectedColor.b, 255);
  lineRGBA(ctx.renderer, selectedAircraft->x - boxSize, selectedAircraft->y - boxSize,
           selectedAircraft->x - boxSize, selectedAircraft->y - boxSize / 2,
           ctx.style->selectedColor.r, ctx.style->selectedColor.g,
           ctx.style->selectedColor.b, 255);

  // Top-right corner
  lineRGBA(ctx.renderer, selectedAircraft->x + boxSize, selectedAircraft->y - boxSize,
           selectedAircraft->x + boxSize / 2, selectedAircraft->y - boxSize,
           ctx.style->selectedColor.r, ctx.style->selectedColor.g,
           ctx.style->selectedColor.b, 255);
  lineRGBA(ctx.renderer, selectedAircraft->x + boxSize, selectedAircraft->y - boxSize,
           selectedAircraft->x + boxSize, selectedAircraft->y - boxSize / 2,
           ctx.style->selectedColor.r, ctx.style->selectedColor.g,
           ctx.style->selectedColor.b, 255);

  // Bottom-right corner
  lineRGBA(ctx.renderer, selectedAircraft->x + boxSize, selectedAircraft->y + boxSize,
           selectedAircraft->x + boxSize / 2, selectedAircraft->y + boxSize,
           ctx.style->selectedColor.r, ctx.style->selectedColor.g,
           ctx.style->selectedColor.b, 255);
  lineRGBA(ctx.renderer, selectedAircraft->x + boxSize, selectedAircraft->y + boxSize,
           selectedAircraft->x + boxSize, selectedAircraft->y + boxSize / 2,
           ctx.style->selectedColor.r, ctx.style->selectedColor.g,
           ctx.style->selectedColor.b, 255);

  // Bottom-left corner
  lineRGBA(ctx.renderer, selectedAircraft->x - boxSize, selectedAircraft->y + boxSize,
           selectedAircraft->x - boxSize / 2, selectedAircraft->y + boxSize,
           ctx.style->selectedColor.r, ctx.style->selectedColor.g,
           ctx.style->selectedColor.b, 255);
  lineRGBA(ctx.renderer, selectedAircraft->x - boxSize, selectedAircraft->y + boxSize,
           selectedAircraft->x - boxSize, selectedAircraft->y + boxSize / 2,
           ctx.style->selectedColor.r, ctx.style->selectedColor.g,
           ctx.style->selectedColor.b, 255);
}

void InputFeedback::drawMouse(const RenderContext& ctx) {
  constexpr uint64_t kFadeDurationMs = 100;
  constexpr uint64_t kIdleTimeoutMs = 50;  // Time before we consider mouse "stopped"

  // Check if mouse has stopped moving (no movement for kIdleTimeoutMs)
  bool currentlyActive = elapsed(mouseLastMoveTime) < kIdleTimeoutMs;

  // Detect state transitions
  if (currentlyActive && !mouseWasActive) {
    // Mouse just started moving - begin fade in
    mouseFadeStartTime = now();
  } else if (!currentlyActive && mouseWasActive) {
    // Mouse just stopped - begin fade out
    mouseFadeStartTime = now();
  }

  mouseWasActive = currentlyActive;

  // Calculate opacity based on fade animation
  uint64_t fadeElapsed = elapsed(mouseFadeStartTime);
  float fadeProgress = std::min(1.0f, static_cast<float>(fadeElapsed) / kFadeDurationMs);

  if (currentlyActive) {
    // Fading in
    mouseOpacity = fadeProgress;
  } else {
    // Fading out
    mouseOpacity = 1.0f - fadeProgress;
  }

  // Don't draw if fully transparent
  if (mouseOpacity <= 0.0f) {
    mouseActive = false;
    return;
  }

  mouseActive = true;
  highFramerate = true;

  int alpha = static_cast<int>(255.0f * mouseOpacity);
  int crosshairSize = static_cast<int>(10 * ctx.uiScale);

  // Draw crosshairs
  lineRGBA(ctx.renderer, mousex - crosshairSize, mousey, mousex + crosshairSize,
           mousey, ctx.style->white.r, ctx.style->white.g, ctx.style->white.b,
           static_cast<Uint8>(alpha));
  lineRGBA(ctx.renderer, mousex, mousey - crosshairSize, mousex,
           mousey + crosshairSize, ctx.style->white.r, ctx.style->white.g,
           ctx.style->white.b, static_cast<Uint8>(alpha));
}

void InputFeedback::registerClick(int tapcount, int x, int y,
                                  AircraftList& aircraftList,
                                  Aircraft** selectedAircraft, MapView& mapView) {
  if (tapcount == 1) {
    Aircraft* selection = nullptr;

    for (const auto& p : aircraftList) {
      if (x && y) {
        int distSq = (p->x - x) * (p->x - x) + (p->y - y) * (p->y - y);
        if (distSq < 900) {
          if (selection) {
            int selDistSq = (selection->x - x) * (selection->x - x) +
                            (selection->y - y) * (selection->y - y);
            if (distSq < selDistSq) {
              selection = p.get();
            }
          } else {
            selection = p.get();
          }
        }
      }
    }

    *selectedAircraft = selection;
  } else if (tapcount == 2) {
    mapView.setTargetZoom(0.25f * mapView.maxDist);
    mapView.animateCenterAbsolute(static_cast<float>(x), static_cast<float>(y),
                                  800, 600);  // TODO: pass actual screen dimensions
  }

  clickx = x;
  clicky = y;
  clickTime = now();
}

void InputFeedback::registerMouseMove(int x, int y) {
  mouseLastMoveTime = now();
  mousex = x;
  mousey = y;
  highFramerate = true;
}

}  // namespace viz1090
