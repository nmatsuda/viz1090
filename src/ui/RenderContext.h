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

#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "style/Style.h"

namespace viz1090 {

/// Shared rendering context passed to all drawing components
/// Bundles commonly needed rendering state to avoid passing many parameters
struct RenderContext {
  // SDL rendering
  SDL_Renderer* renderer{nullptr};

  // Screen dimensions
  int screenWidth{0};
  int screenHeight{0};
  int uiScale{1};

  // Style reference for consistent theming
  const Style* style{nullptr};

  // Fonts
  TTF_Font* mapFont{nullptr};
  TTF_Font* mapBoldFont{nullptr};
  TTF_Font* labelFont{nullptr};
  TTF_Font* messageFont{nullptr};
  TTF_Font* listFont{nullptr};

  // Font dimensions
  int mapFontWidth{5};
  int mapFontHeight{12};
  int labelFontWidth{6};
  int labelFontHeight{12};
  int messageFontWidth{6};
  int messageFontHeight{12};

  // Helper accessors
  [[nodiscard]] int padding() const { return 5; }
  [[nodiscard]] int cornerRadius() const { return 3; }
};

}  // namespace viz1090

#endif  // RENDER_CONTEXT_H
