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
//

#ifndef VIEW_H
#define VIEW_H

#include <chrono>
#include <string>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "app/AppData.h"
#include "style/Style.h"
#include "ui/AircraftRenderer.h"
#include "ui/InputFeedback.h"
#include "ui/MapView.h"
#include "ui/RenderContext.h"
#include "ui/UIOverlay.h"

namespace viz1090 {

/// Main view class that orchestrates all rendering components
class View {
public:
  View(AppData* appData);
  ~View();

  /// Initialize SDL subsystems
  void SDL_init();

  /// Initialize fonts
  void font_init();

  /// Main draw function - orchestrates all rendering
  void draw();

  /// Input event handlers
  void registerClick(int tapcount, int x, int y);
  void registerMouseMove(int x, int y);

  /// Viewport control - delegates to MapView
  void moveCenterRelative(float dx, float dy);
  void moveCenterAbsolute(float x, float y);
  void animateCenterAbsolute(float x, float y);

  // Configuration
  bool metric{false};
  bool fps{false};

  // Screen configuration
  int screen_upscale{1};
  int screen_uiscale{1};
  int screen_width{0};
  int screen_height{0};
  int screen_depth{32};
  int fullscreen{0};
  int screen_index{0};

  // SDL handles (public for compatibility with existing code)
  SDL_Window* window{nullptr};
  SDL_Renderer* renderer{nullptr};

  // Expose map view for external access to viewport state
  MapView& getMapView() { return mapView; }
  const MapView& getMapView() const { return mapView; }

private:
  TTF_Font* loadFont(const char* name, int size);
  void closeFont(TTF_Font* font);
  void updateRenderContext();

  // Core data
  AppData* appData;
  Aircraft* selectedAircraft{nullptr};

  // Style
  Style style;

  // Components
  MapView mapView;
  AircraftRenderer aircraftRenderer;
  UIOverlay uiOverlay;
  InputFeedback inputFeedback;

  // Shared render context
  RenderContext renderContext;

  // Fonts (owned by View, shared via RenderContext)
  TTF_Font* mapFont{nullptr};
  TTF_Font* mapBoldFont{nullptr};
  TTF_Font* listFont{nullptr};
  TTF_Font* messageFont{nullptr};
  TTF_Font* labelFont{nullptr};

  int mapFontWidth{5};
  int mapFontHeight{12};
  int labelFontWidth{6};
  int labelFontHeight{12};
  int messageFontWidth{6};
  int messageFontHeight{12};

  // Timing
  float lastFrameTime{0.0f};
  std::chrono::high_resolution_clock::time_point drawStartTime;

  // State
  int startupState{0};
  bool highFramerate{false};
};

}  // namespace viz1090

#endif  // VIEW_H
