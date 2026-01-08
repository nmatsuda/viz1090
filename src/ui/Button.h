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

#ifndef BUTTON_H
#define BUTTON_H

#include <SDL2/SDL.h>

#include <functional>
#include <string>

#include "ui/RenderContext.h"

namespace viz1090 {

/// A clickable button that follows the status box visual style
class Button {
public:
  using ClickCallback = std::function<void()>;

  Button() = default;
  Button(const std::string& label, ClickCallback callback);

  /// Draw the button at the specified position
  /// Returns the width of the button for layout purposes
  int draw(const RenderContext& ctx, int left, int top);

  /// Check if point is within button bounds
  [[nodiscard]] bool containsPoint(int x, int y) const;

  /// Handle click - returns true if button was clicked
  bool handleClick(int x, int y);

  /// Setters
  void setLabel(const std::string& label) { label_ = label; }
  void setCallback(ClickCallback callback) { callback_ = std::move(callback); }
  void setColor(SDL_Color color) { color_ = color; }

  /// Getters
  [[nodiscard]] const std::string& label() const { return label_; }
  [[nodiscard]] int width() const { return width_; }
  [[nodiscard]] int height() const { return height_; }
  [[nodiscard]] const SDL_Rect& bounds() const { return bounds_; }

private:
  std::string label_;
  ClickCallback callback_;
  SDL_Color color_{196, 196, 196, 255};  // Default to buttonColor

  // Cached bounds for hit testing
  SDL_Rect bounds_{0, 0, 0, 0};
  int width_{0};
  int height_{0};
};

}  // namespace viz1090

#endif  // BUTTON_H
