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

#include "ui/Button.h"

#include "SDL2/SDL2_gfxPrimitives.h"
#include "ui/Label.h"

namespace viz1090 {

Button::Button(const std::string& label, ClickCallback callback)
    : label_(label), callback_(std::move(callback)) {}

int Button::draw(const RenderContext& ctx, int left, int top) {
  // Calculate button dimensions based on label
  int labelWidth = ctx.labelTextWidth(label_);
  height_ = ctx.labelFontHeight();
  width_ = labelWidth;

  // Update bounds for hit testing
  bounds_.x = left;
  bounds_.y = top;
  bounds_.w = width_;
  bounds_.h = height_;

  // Draw filled black background
  roundedBoxRGBA(ctx.renderer, left, top, left + width_, top + height_,
                 ctx.cornerRadius(), ctx.style->buttonBackground.r,
                 ctx.style->buttonBackground.g, ctx.style->buttonBackground.b,
                 SDL_ALPHA_OPAQUE);

  // Draw solid outline
  roundedRectangleRGBA(ctx.renderer, left, top, left + width_, top + height_,
                       ctx.cornerRadius(), ctx.style->buttonOutline.r,
                       ctx.style->buttonOutline.g, ctx.style->buttonOutline.b,
                       SDL_ALPHA_OPAQUE);

  // Draw the label text (light text on dark background)
  Label buttonLabel;
  buttonLabel.setFont(ctx.labelFont());
  buttonLabel.setColor(ctx.style->buttonTextColor);
  buttonLabel.setPosition(left + ctx.labelFontWidth() / 2, top);
  buttonLabel.setText(label_);
  buttonLabel.draw(ctx.renderer);

  return width_;
}

bool Button::containsPoint(int x, int y) const {
  return x >= bounds_.x && x < bounds_.x + bounds_.w &&
         y >= bounds_.y && y < bounds_.y + bounds_.h;
}

bool Button::handleClick(int x, int y) {
  if (containsPoint(x, y) && callback_) {
    callback_();
    return true;
  }
  return false;
}

}  // namespace viz1090
