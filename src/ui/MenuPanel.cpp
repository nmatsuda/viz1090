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

#include "ui/MenuPanel.h"

#include "SDL2/SDL2_gfxPrimitives.h"

namespace viz1090 {

MenuPanel::MenuPanel() {
  // Close button is always present
  closeButton_.setLabel("close");
  closeButton_.setCallback([this]() { close(); });
}

void MenuPanel::addButton(const std::string& label, ActionCallback callback) {
  buttons_.emplace_back(label, std::move(callback));
}

void MenuPanel::draw(const RenderContext& ctx) {
  if (!open_) {
    return;
  }

  // Calculate panel dimensions based on content
  // Panel width is based on widest button + padding
  int maxButtonWidth = 0;
  for (const auto& button : buttons_) {
    int buttonWidth = static_cast<int>((button.label().length() + 1) * ctx.labelFontWidth);
    maxButtonWidth = std::max(maxButtonWidth, buttonWidth);
  }
  // Include close button
  int closeWidth = static_cast<int>((closeButton_.label().length() + 1) * ctx.labelFontWidth);
  maxButtonWidth = std::max(maxButtonWidth, closeWidth);

  int panelPadding = ctx.padding() * 2;
  int buttonSpacing = ctx.padding();
  int numButtons = static_cast<int>(buttons_.size()) + 1;  // +1 for close button

  int panelWidth = maxButtonWidth + panelPadding * 2;
  int panelHeight = numButtons * ctx.labelFontHeight +
                    (numButtons - 1) * buttonSpacing + panelPadding * 2;

  // Center the panel on screen
  int panelLeft = (ctx.screenWidth - panelWidth) / 2;
  int panelTop = (ctx.screenHeight - panelHeight) / 2;

  // Update cached bounds
  panelBounds_.x = panelLeft;
  panelBounds_.y = panelTop;
  panelBounds_.w = panelWidth;
  panelBounds_.h = panelHeight;

  // Draw semi-transparent background overlay
  boxRGBA(ctx.renderer, 0, 0, ctx.screenWidth, ctx.screenHeight,
          0, 0, 0, 128);

  // Draw panel background
  roundedBoxRGBA(ctx.renderer, panelLeft, panelTop,
                 panelLeft + panelWidth, panelTop + panelHeight,
                 ctx.cornerRadius() * 2,
                 ctx.style->buttonBackground.r, ctx.style->buttonBackground.g,
                 ctx.style->buttonBackground.b, SDL_ALPHA_OPAQUE);

  // Draw panel border
  roundedRectangleRGBA(ctx.renderer, panelLeft, panelTop,
                       panelLeft + panelWidth, panelTop + panelHeight,
                       ctx.cornerRadius() * 2,
                       ctx.style->buttonOutline.r, ctx.style->buttonOutline.g,
                       ctx.style->buttonOutline.b, SDL_ALPHA_OPAQUE);

  // Draw buttons centered in the panel
  int buttonTop = panelTop + panelPadding;

  for (auto& button : buttons_) {
    // Center button horizontally within panel
    int buttonWidth = static_cast<int>((button.label().length() + 1) * ctx.labelFontWidth);
    int centeredLeft = panelLeft + (panelWidth - buttonWidth) / 2;
    button.draw(ctx, centeredLeft, buttonTop);
    buttonTop += ctx.labelFontHeight + buttonSpacing;
  }

  // Draw close button last
  int centeredLeft = panelLeft + (panelWidth - closeWidth) / 2;
  closeButton_.draw(ctx, centeredLeft, buttonTop);
}

bool MenuPanel::handleClick(int x, int y) {
  if (!open_) {
    return false;
  }

  // Check if click is within panel bounds
  if (x < panelBounds_.x || x >= panelBounds_.x + panelBounds_.w ||
      y < panelBounds_.y || y >= panelBounds_.y + panelBounds_.h) {
    // Click outside panel - close it
    close();
    return true;
  }

  // Check buttons
  for (auto& button : buttons_) {
    if (button.handleClick(x, y)) {
      return true;
    }
  }

  // Check close button
  if (closeButton_.handleClick(x, y)) {
    return true;
  }

  // Click was inside panel but not on a button - consume it
  return true;
}

}  // namespace viz1090
