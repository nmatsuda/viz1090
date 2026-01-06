// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// Copyright (C) 2014, Malcolm Robb <Support@ATTAvionics.com>
// Copyright (C) 2012, Salvatore Sanfilippo <antirez at gmail dot com>
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

#ifndef VIZ1090_SDL_HELPERS_H
#define VIZ1090_SDL_HELPERS_H

#include <memory>
#include <stdexcept>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace viz1090 {

/// SDL initialization error
class SdlError : public std::runtime_error {
public:
  explicit SdlError(const std::string& aMessage)
      : std::runtime_error(aMessage + ": " + SDL_GetError()) {}
};

/// TTF initialization error
class TtfError : public std::runtime_error {
public:
  explicit TtfError(const std::string& aMessage)
      : std::runtime_error(aMessage + ": " + TTF_GetError()) {}
};

// Custom deleters for SDL types
struct SdlWindowDeleter {
  void operator()(SDL_Window* aWindow) const {
    if (aWindow) {
      SDL_DestroyWindow(aWindow);
    }
  }
};

struct SdlRendererDeleter {
  void operator()(SDL_Renderer* aRenderer) const {
    if (aRenderer) {
      SDL_DestroyRenderer(aRenderer);
    }
  }
};

struct SdlTextureDeleter {
  void operator()(SDL_Texture* aTexture) const {
    if (aTexture) {
      SDL_DestroyTexture(aTexture);
    }
  }
};

struct SdlSurfaceDeleter {
  void operator()(SDL_Surface* aSurface) const {
    if (aSurface) {
      SDL_FreeSurface(aSurface);
    }
  }
};

struct TtfFontDeleter {
  void operator()(TTF_Font* aFont) const {
    if (aFont) {
      TTF_CloseFont(aFont);
    }
  }
};

// RAII wrappers using unique_ptr with custom deleters
using SdlWindowPtr = std::unique_ptr<SDL_Window, SdlWindowDeleter>;
using SdlRendererPtr = std::unique_ptr<SDL_Renderer, SdlRendererDeleter>;
using SdlTexturePtr = std::unique_ptr<SDL_Texture, SdlTextureDeleter>;
using SdlSurfacePtr = std::unique_ptr<SDL_Surface, SdlSurfaceDeleter>;
using TtfFontPtr = std::unique_ptr<TTF_Font, TtfFontDeleter>;

/// RAII wrapper for SDL initialization
class SdlContext {
public:
  explicit SdlContext(Uint32 aFlags = SDL_INIT_VIDEO) {
    if (SDL_Init(aFlags) < 0) {
      throw SdlError("SDL_Init failed");
    }
  }

  ~SdlContext() { SDL_Quit(); }

  // Non-copyable
  SdlContext(const SdlContext&) = delete;
  SdlContext& operator=(const SdlContext&) = delete;

  // Movable
  SdlContext(SdlContext&&) = default;
  SdlContext& operator=(SdlContext&&) = default;
};

/// RAII wrapper for SDL_ttf initialization
class TtfContext {
public:
  TtfContext() {
    if (TTF_Init() < 0) {
      throw TtfError("TTF_Init failed");
    }
  }

  ~TtfContext() { TTF_Quit(); }

  // Non-copyable
  TtfContext(const TtfContext&) = delete;
  TtfContext& operator=(const TtfContext&) = delete;

  // Movable
  TtfContext(TtfContext&&) = default;
  TtfContext& operator=(TtfContext&&) = default;
};

/// Helper to create SDL window with RAII
[[nodiscard]] inline SdlWindowPtr
createWindow(const char* aTitle, int aX, int aY, int aW, int aH, Uint32 aFlags) {
  SDL_Window* window = SDL_CreateWindow(aTitle, aX, aY, aW, aH, aFlags);
  if (!window) {
    throw SdlError("SDL_CreateWindow failed");
  }
  return SdlWindowPtr(window);
}

/// Helper to create SDL renderer with RAII
[[nodiscard]] inline SdlRendererPtr
createRenderer(SDL_Window* aWindow, int aIndex, Uint32 aFlags) {
  SDL_Renderer* renderer = SDL_CreateRenderer(aWindow, aIndex, aFlags);
  if (!renderer) {
    throw SdlError("SDL_CreateRenderer failed");
  }
  return SdlRendererPtr(renderer);
}

/// Helper to create SDL texture with RAII
[[nodiscard]] inline SdlTexturePtr
createTexture(SDL_Renderer* aRenderer, Uint32 aFormat, int aAccess, int aW, int aH) {
  SDL_Texture* texture = SDL_CreateTexture(aRenderer, aFormat, aAccess, aW, aH);
  if (!texture) {
    throw SdlError("SDL_CreateTexture failed");
  }
  return SdlTexturePtr(texture);
}

/// Helper to load TTF font with RAII
[[nodiscard]] inline TtfFontPtr
loadFont(const char* aPath, int aSize) {
  TTF_Font* font = TTF_OpenFont(aPath, aSize);
  if (!font) {
    throw TtfError("TTF_OpenFont failed");
  }
  return TtfFontPtr(font);
}

}  // namespace viz1090

#endif  // VIZ1090_SDL_HELPERS_H
