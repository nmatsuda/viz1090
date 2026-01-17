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

#include "ui/UIOverlay.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "SDL2/SDL2_gfxPrimitives.h"
#include "app/AppData.h"
#include "ui/Label.h"

namespace viz1090 {

UIOverlay::UIOverlay() {
  // Set up menu button
  menuButton_.setLabel("menu");
  menuButton_.setCallback([this]() { menuPanel_.open(); });
}

void UIOverlay::setFrameAllCallback(FrameAllCallback callback) {
  frameAllCallback_ = std::move(callback);

  // Add frame-all button to the menu panel
  menuPanel_.addButton("frame all", [this]() {
    if (frameAllCallback_) {
      frameAllCallback_();
    }
    menuPanel_.close();
  });
}

void UIOverlay::setThemeSupport(ThemeListProvider listProvider,
                                CurrentThemeProvider currentProvider,
                                ThemeSelectedCallback selectedCallback) {
  menuPanel_.setThemeSupport(std::move(listProvider),
                             std::move(currentProvider),
                             std::move(selectedCallback));
}

void UIOverlay::drawStatusBox(const RenderContext& ctx, int* left, int* top,
                              const std::string& label, const std::string& message,
                              SDL_Color color) {
  int labelWidth = static_cast<int>((label.length() + ((label.length() > 0) ? 1 : 0)) *
                                    ctx.labelFontWidth);
  int messageWidth = static_cast<int>((message.length() + ((message.length() > 0) ? 1 : 0)) *
                                      ctx.messageFontWidth);

  if (*left + labelWidth + messageWidth + ctx.padding() > ctx.screenWidth) {
    *left = ctx.padding();
    *top = *top - ctx.messageFontHeight - ctx.padding();
  }

  // filled black background
  if (messageWidth) {
    roundedBoxRGBA(ctx.renderer, *left, *top, *left + labelWidth + messageWidth,
                   *top + ctx.messageFontHeight, ctx.cornerRadius(),
                   ctx.style->buttonBackground.r, ctx.style->buttonBackground.g,
                   ctx.style->buttonBackground.b, SDL_ALPHA_OPAQUE);
  }

  // filled label box
  if (labelWidth) {
    roundedBoxRGBA(ctx.renderer, *left, *top, *left + labelWidth,
                   *top + ctx.messageFontHeight, ctx.cornerRadius(),
                   color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
  }

  // outline message box
  if (messageWidth) {
    roundedRectangleRGBA(ctx.renderer, *left, *top, *left + labelWidth + messageWidth,
                         *top + ctx.messageFontHeight, ctx.cornerRadius(),
                         color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
  }

  Label currentLabel;
  currentLabel.setFont(ctx.labelFont);
  currentLabel.setColor(ctx.style->buttonBackground);
  currentLabel.setPosition(*left + ctx.labelFontWidth / 2, *top);
  currentLabel.setText(label);
  currentLabel.draw(ctx.renderer);

  currentLabel.setFont(ctx.messageFont);
  currentLabel.setColor(color);
  currentLabel.setPosition(*left + labelWidth + ctx.messageFontWidth / 2, *top);
  currentLabel.setText(message);
  currentLabel.draw(ctx.renderer);

  *left = *left + labelWidth + messageWidth + ctx.padding();
}

void UIOverlay::drawButton(const RenderContext& ctx, int* left, int* top, Button& button) {
  int buttonWidth = static_cast<int>((button.label().length() + 1) * ctx.labelFontWidth);

  if (*left + buttonWidth + ctx.padding() > ctx.screenWidth) {
    *left = ctx.padding();
    *top = *top - ctx.messageFontHeight - ctx.padding();
  }

  button.draw(ctx, *left, *top);
  *left = *left + buttonWidth + ctx.padding();
}

void UIOverlay::drawCenteredStatusBox(const RenderContext& ctx,
                                      const std::string& label, const std::string& message,
                                      SDL_Color color) {
  int labelWidth = static_cast<int>((label.length() + ((label.length() > 0) ? 1 : 0)) *
                                    ctx.labelFontWidth);
  int messageWidth = static_cast<int>((message.length() + ((message.length() > 0) ? 1 : 0)) *
                                      ctx.messageFontWidth);

  int left = (ctx.screenWidth - (labelWidth + messageWidth)) / 2;
  int top = (ctx.screenHeight - ctx.labelFontHeight) / 2;

  drawStatusBox(ctx, &left, &top, label, message, color);
}

void UIOverlay::draw(const RenderContext& ctx, const AppData& appData, float lastFrameTime,
                     float centerLat, float centerLon, int mapLoadPercent) {
  int left = ctx.padding();
  int top = ctx.screenHeight - ctx.messageFontHeight - ctx.padding();

  if (showFps) {
    char fps[60] = " ";
    snprintf(fps, 40, "%5.1f", 1000.0f / lastFrameTime);

    drawStatusBox(ctx, &left, &top, "fps", fps, ctx.style->grey_dark);
  }

  if (!appData.connected()) {
    drawStatusBox(ctx, &left, &top, "init", "connecting", ctx.style->red);
  } else {
    char strLoc[20] = " ";
    snprintf(strLoc, 20, "%7.3fN %7.3f%c", centerLat, std::fabs(centerLon),
             (centerLon > 0) ? 'E' : 'W');
    drawStatusBox(ctx, &left, &top, "loc", strLoc, ctx.style->buttonColor);

    char strPlaneCount[10] = " ";
    snprintf(strPlaneCount, 10, "%d/%d", appData.numVisiblePlanes, appData.numPlanes);
    drawStatusBox(ctx, &left, &top, "disp", strPlaneCount, ctx.style->buttonColor);

    char strMsgRate[18] = " ";
    snprintf(strMsgRate, 18, "%.0f/s", appData.msgRate);
    drawStatusBox(ctx, &left, &top, "rate", strMsgRate, ctx.style->buttonColor);

    char strSig[18] = " ";
    snprintf(strSig, 18, "%.0f%%", 100.0 * appData.avgSig / 1024.0);
    drawStatusBox(ctx, &left, &top, "sAvg", strSig, ctx.style->buttonColor);
  }

  if (mapLoadPercent < 100) {
    char loaded[32] = " ";
    snprintf(loaded, 32, "loading map %d%%", mapLoadPercent);
    drawStatusBox(ctx, &left, &top, "init", loaded, ctx.style->orange);
  }

  // Draw menu button after status labels
  drawButton(ctx, &left, &top, menuButton_);
}

void UIOverlay::drawMenuPanel(const RenderContext& ctx) {
  menuPanel_.draw(ctx);
}

bool UIOverlay::handleClick(int x, int y) {
  // Check menu panel first (it's on top)
  if (menuPanel_.isOpen()) {
    return menuPanel_.handleClick(x, y);
  }

  // Check menu button
  if (menuButton_.handleClick(x, y)) {
    return true;
  }

  return false;
}

UIOverlay::StatusBarBounds UIOverlay::calculateStatusBarBounds(const RenderContext& ctx,
                                                                const AppData& appData,
                                                                int mapLoadPercent) const {
  StatusBarBounds bounds;
  bounds.bottomY = ctx.screenHeight;
  bounds.topY = ctx.screenHeight - ctx.messageFontHeight - ctx.padding();

  int left = ctx.padding();
  int top = bounds.topY;

  // Helper to simulate adding an element - matches drawStatusBox logic exactly
  // Width = (label.length() + 1) * labelFontWidth + (message.length() + 1) * messageFontWidth
  auto simulateStatusBox = [&](const char* label, size_t messageLen) {
    size_t labelLen = std::strlen(label);
    int labelWidth = static_cast<int>((labelLen + 1) * ctx.labelFontWidth);
    int messageWidth = static_cast<int>((messageLen + 1) * ctx.messageFontWidth);
    int totalWidth = labelWidth + messageWidth;

    if (left + totalWidth + ctx.padding() > ctx.screenWidth) {
      left = ctx.padding();
      top = top - ctx.messageFontHeight - ctx.padding();
    }
    left = left + totalWidth + ctx.padding();
    if (top < bounds.topY) {
      bounds.topY = top;
    }
  };

  auto simulateButton = [&](size_t labelLen) {
    int buttonWidth = static_cast<int>((labelLen + 1) * ctx.labelFontWidth);
    if (left + buttonWidth + ctx.padding() > ctx.screenWidth) {
      left = ctx.padding();
      top = top - ctx.messageFontHeight - ctx.padding();
    }
    left = left + buttonWidth + ctx.padding();
    if (top < bounds.topY) {
      bounds.topY = top;
    }
  };

  // Simulate fps box if shown: "fps" + "XXX.X" (5 chars typical)
  if (showFps) {
    simulateStatusBox("fps", 5);
  }

  if (!appData.connected()) {
    // "init" + "connecting" (10 chars)
    simulateStatusBox("init", 10);
  } else {
    // "loc" + "XX.XXXN XXX.XXXE" (17 chars from "%7.3fN %7.3f%c")
    simulateStatusBox("loc", 17);

    // "disp" + "XX/XX" (5 chars typical, could be more with many planes)
    char strPlaneCount[10];
    snprintf(strPlaneCount, 10, "%d/%d", appData.numVisiblePlanes, appData.numPlanes);
    simulateStatusBox("disp", std::strlen(strPlaneCount));

    // "rate" + "XXXX/s" (6 chars typical)
    char strMsgRate[18];
    snprintf(strMsgRate, 18, "%.0f/s", appData.msgRate);
    simulateStatusBox("rate", std::strlen(strMsgRate));

    // "sAvg" + "XXX%" (4 chars typical)
    char strSig[18];
    snprintf(strSig, 18, "%.0f%%", 100.0 * appData.avgSig / 1024.0);
    simulateStatusBox("sAvg", std::strlen(strSig));
  }

  if (mapLoadPercent < 100) {
    // "init" + "loading map XX%" (15 chars typical)
    char loaded[32];
    snprintf(loaded, 32, "loading map %d%%", mapLoadPercent);
    simulateStatusBox("init", std::strlen(loaded));
  }

  // Menu button
  simulateButton(menuButton_.label().length());

  // Track the rightmost extent of the bottom row
  int bottomRowTop = ctx.screenHeight - ctx.messageFontHeight - ctx.padding();
  if (top == bottomRowTop) {
    // All elements fit on one row
    bounds.rightX = left;
  } else {
    // Multiple rows - need to find right edge of bottom row
    // For simplicity, set rightX to full width when wrapped (conservative)
    bounds.rightX = ctx.screenWidth;
  }

  return bounds;
}

}  // namespace viz1090
