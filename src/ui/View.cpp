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

#include "ui/View.h"

#include <cmath>
#include <iostream>
#include <thread>

#include "ui/AircraftLabel.h"
#include "ui/MathUtils.h"
#include "viz1090/Profiler.h"

namespace viz1090 {

namespace {
constexpr int TARGET_FRAME_TIME = 30;
}

//
// Font management
//

TTF_Font*
View::loadFont(const char* name, int size) {
  TTF_Font* font = TTF_OpenFont(name, size);

  if (font == nullptr) {
    printf("Failed to open Font %s: %s\n", name, TTF_GetError());
    exit(1);
  }

  return font;
}

void
View::closeFont(TTF_Font* font) {
  if (font != nullptr) {
    TTF_CloseFont(font);
  }
}

void
View::font_init() {
  mapFont = loadFont("font/TerminusTTF-4.46.0.ttf", 12 * screen_uiscale);
  mapBoldFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * screen_uiscale);
  listFont = loadFont("font/TerminusTTF-4.46.0.ttf", 12 * screen_uiscale);
  messageFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * screen_uiscale);
  labelFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * screen_uiscale);

  mapFontWidth = 5 * screen_uiscale;
  mapFontHeight = 12 * screen_uiscale;
  messageFontWidth = 6 * screen_uiscale;
  messageFontHeight = 12 * screen_uiscale;
  labelFontWidth = 6 * screen_uiscale;
  labelFontHeight = 12 * screen_uiscale;
}

//
// SDL initialization
//

void
View::SDL_init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Could not initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  if (TTF_Init() < 0) {
    printf("Couldn't initialize SDL TTF: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_ShowCursor(SDL_DISABLE);

  Uint32 flags = 0;

  if (fullscreen) {
    flags = flags | SDL_WINDOW_FULLSCREEN_DESKTOP;
  }

  if (screen_width == 0) {
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);
    screen_width = DM.w;
    screen_height = DM.h;
  }

  window = SDL_CreateWindow("viz1090", SDL_WINDOWPOS_CENTERED_DISPLAY(screen_index),
                            SDL_WINDOWPOS_CENTERED_DISPLAY(screen_index), screen_width,
                            screen_height, flags);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  // Initialize map view texture
  mapView.initTexture(renderer, screen_width, screen_height);

  if (fullscreen) {
    SDL_RenderSetLogicalSize(renderer, screen_width, screen_height);
  }
}

//
// Render context management
//

void
View::updateRenderContext() {
  renderContext.renderer = renderer;
  renderContext.screenWidth = screen_width;
  renderContext.screenHeight = screen_height;
  renderContext.uiScale = screen_uiscale;
  renderContext.style = &styleManager_.currentTheme();
  renderContext.mapFont = mapFont;
  renderContext.mapBoldFont = mapBoldFont;
  renderContext.labelFont = labelFont;
  renderContext.messageFont = messageFont;
  renderContext.listFont = listFont;
  renderContext.mapFontWidth = mapFontWidth;
  renderContext.mapFontHeight = mapFontHeight;
  renderContext.labelFontWidth = labelFontWidth;
  renderContext.labelFontHeight = labelFontHeight;
  renderContext.messageFontWidth = messageFontWidth;
  renderContext.messageFontHeight = messageFontHeight;
}

//
// Viewport control - delegates to MapView
//

void
View::moveCenterRelative(float dx, float dy) {
  mapView.moveCenterRelative(dx, dy, screen_width, screen_height);
  highFramerate = true;
}

void
View::moveCenterAbsolute(float x, float y) {
  mapView.moveCenterAbsolute(x, y, screen_width, screen_height);
  highFramerate = true;
}

void
View::animateCenterAbsolute(float x, float y) {
  mapView.animateCenterAbsolute(x, y, screen_width, screen_height);
  highFramerate = true;
}

void
View::recenterOnOrigin() {
  // Clear any selected aircraft so view doesn't track it
  selectedAircraft = nullptr;

  // Animate to the origin position
  mapView.mapTargetLon = mapView.originLon;
  mapView.mapTargetLat = mapView.originLat;
  mapView.setMoved();
  highFramerate = true;
}

void
View::frameAllAircraft() {
  // Lock aircraft list for thread-safe iteration
  auto lock = appData->lockAircraftList();

  if (appData->aircraftList.empty()) {
    return;
  }

  // Find bounding box of all aircraft with valid positions
  float minLat = 90.0f;
  float maxLat = -90.0f;
  float minLon = 180.0f;
  float maxLon = -180.0f;
  int validCount = 0;

  for (const auto& aircraft : appData->aircraftList) {
    // Only include aircraft with valid lat/lon (non-zero and within valid range)
    if (aircraft->lat != 0.0f || aircraft->lon != 0.0f) {
      if (aircraft->lat >= -90.0f && aircraft->lat <= 90.0f &&
          aircraft->lon >= -180.0f && aircraft->lon <= 180.0f) {
        minLat = std::min(minLat, aircraft->lat);
        maxLat = std::max(maxLat, aircraft->lat);
        minLon = std::min(minLon, aircraft->lon);
        maxLon = std::max(maxLon, aircraft->lon);
        validCount++;
      }
    }
  }

  if (validCount == 0) {
    return;
  }

  // Calculate center point
  float centerLat = (minLat + maxLat) / 2.0f;
  float centerLon = (minLon + maxLon) / 2.0f;

  // Calculate distance needed to show all aircraft
  // Convert spans to km
  float latSpan = (maxLat - minLat) * LATLONMULT;
  float lonSpan = (maxLon - minLon) * LATLONMULT * std::cos(centerLat * M_PI / 180.0f);

  // Account for aspect ratio: maxDist maps to half the LARGER screen dimension,
  // so we need to scale spans based on which screen axis they'll use.
  // screenDist uses: scale_factor * 0.5 * d / maxDist, where scale_factor = max(w,h)
  // The half-span in each direction is the required "radius" for that axis
  float latHalfSpan = latSpan / 2.0f;  // vertical (Y axis)
  float lonHalfSpan = lonSpan / 2.0f;  // horizontal (X axis)

  // Scale based on aspect ratio: if screen is wider than tall, vertical space is limiting
  float effectiveLatRadius = latHalfSpan;
  float effectiveLonRadius = lonHalfSpan;
  if (screen_width > screen_height) {
    // Wider screen: Y axis is shorter, so lat needs more zoom (scale up)
    effectiveLatRadius = latHalfSpan * (static_cast<float>(screen_width) / screen_height);
  } else {
    // Taller screen: X axis is shorter, so lon needs more zoom (scale up)
    effectiveLonRadius = lonHalfSpan * (static_cast<float>(screen_height) / screen_width);
  }

  float maxRadius = std::max(effectiveLatRadius, effectiveLonRadius);

  // Add 20% padding so aircraft aren't right at the edge
  float newMaxDist = maxRadius * 1.2f;

  // Set minimum zoom level
  if (newMaxDist < 5.0f) {
    newMaxDist = 5.0f;
  }

  // Clear any selected aircraft so view doesn't track it
  selectedAircraft = nullptr;

  // Animate to the new center and zoom level
  mapView.mapTargetLon = centerLon;
  mapView.mapTargetLat = centerLat;
  mapView.mapTargetMaxDist = newMaxDist;
  mapView.setMoved();
  highFramerate = true;
}

//
// Input handling - delegates to InputFeedback
//

void
View::registerClick(int tapcount, int x, int y) {
  // Check if UI elements handle the click first
  if (uiOverlay.handleClick(x, y)) {
    highFramerate = true;
    return;
  }

  // Otherwise handle as map/aircraft interaction
  {
    // Lock aircraft list for thread-safe iteration during click detection
    auto lock = appData->lockAircraftList();
    inputFeedback.registerClick(tapcount, x, y, appData->aircraftList, &selectedAircraft, mapView);
  }
  highFramerate = true;
}

void
View::registerMouseMove(int x, int y) {
  inputFeedback.registerMouseMove(x, y);
  highFramerate = true;
}

//
// Main draw function
//

void
View::draw() {
  PROFILE_BEGIN_FRAME();

  drawStartTime = now();

  // Frame rate limiting
  if (lastFrameTime < TARGET_FRAME_TIME) {
    SDL_Delay(static_cast<Uint32>(TARGET_FRAME_TIME - lastFrameTime));
  }

  // Update render context with current state
  updateRenderContext();

  // Update map view animations
  mapView.update();

  // Check if components need high framerate
  if (mapView.isAnimating() || inputFeedback.needsHighFramerate() ||
      aircraftRenderer.needsHighFramerate()) {
    highFramerate = true;
  }

  // Update aircraft positions for selected aircraft tracking
  if (selectedAircraft) {
    mapView.setTarget(selectedAircraft->lon, selectedAircraft->lat);
  }

  // Draw geography (map lines, place names)
  mapView.drawGeography(renderContext);

  // Draw scale bars
  mapView.drawScaleBars(renderContext);

  // Draw aircraft if connected
  if (appData->connected()) {
    // Lock aircraft list for thread-safe iteration during render
    auto lock = appData->lockAircraftList();

    // Draw trails first (behind aircraft icons)
    aircraftRenderer.drawTrails(renderContext, appData->aircraftList, mapView,
                                0, 0, screen_width, screen_height);

    // Draw aircraft (icons and labels) - this updates aircraft screen positions
    aircraftRenderer.draw(renderContext, appData->aircraftList, selectedAircraft, mapView);

    // Sync labels to updated aircraft positions (handles zoom/recenter view changes)
    aircraftRenderer.syncLabelsToAircraft(appData->aircraftList);

    // Resolve label conflicts (physics simulation)
    for (int i = 0; i < 8; i++) {
      aircraftRenderer.resolveLabelConflicts(appData->aircraftList);
    }
    // Clear the density changed flag after all labels have been updated
    AircraftLabel::clearDensityChanged();
  }

  // Draw status overlay (status bar and menu button)
  uiOverlay.draw(renderContext, *appData, lastFrameTime, mapView.centerLat, mapView.centerLon,
                 mapView.map.loaded);

  // Draw menu panel (if open) - drawn on top of everything except input feedback
  uiOverlay.drawMenuPanel(renderContext);

  // Draw input feedback (click ripple, selection brackets, mouse cursor)
  inputFeedback.draw(renderContext, selectedAircraft);

  // Present frame
  {
    PROFILE_SCOPE("SDL_RenderPresent");
    SDL_RenderPresent(renderer);
  }

  // Update timing
  lastFrameTime = elapsed(drawStartTime);

  // Reset framerate flags
  inputFeedback.resetHighFramerate();
  aircraftRenderer.resetHighFramerate();
  highFramerate = false;

  PROFILE_END_FRAME();
}

//
// Constructor / Destructor
//

View::View(AppData* appData)
    : appData(appData), mapView(metric) {
  // Load themes from the themes directory
  if (!styleManager_.loadFromDirectory("themes")) {
    std::fprintf(stderr, "Warning: No themes loaded from 'themes' directory\n");
  }

  // Set metric preference on components
  aircraftRenderer.setMetric(&metric);

  // Set up UI callbacks
  uiOverlay.setFrameAllCallback([this]() { frameAllAircraft(); });
  uiOverlay.setThemeSupport(
      [this]() { return styleManager_.themeNames(); },
      [this]() { return styleManager_.currentTheme().name; },
      [this](const std::string& themeName) {
        if (styleManager_.setTheme(themeName)) {
          mapView.setMoved();
          highFramerate = true;
          std::fprintf(stderr, "Switched to theme: %s\n", themeName.c_str());
        }
      });

  // Start map loading in background thread
  std::thread t1(&Map::load, &mapView.map);
  t1.detach();
}

View::~View() {
  closeFont(mapFont);
  closeFont(mapBoldFont);
  closeFont(messageFont);
  closeFont(labelFont);
  closeFont(listFont);

  TTF_Quit();
  SDL_Quit();
}

}  // namespace viz1090
