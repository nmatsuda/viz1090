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

#include "ui/LayerScreenshot.h"

#include <SDL2/SDL_image.h>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace viz1090 {

LayerScreenshot::~LayerScreenshot() {
  destroyTextures();
}

void LayerScreenshot::init(SDL_Renderer* renderer, int width, int height) {
  if (initialized_ && renderer_ == renderer && width_ == width && height_ == height) {
    return;  // Already initialized with same parameters
  }

  destroyTextures();

  renderer_ = renderer;
  width_ = width;
  height_ = height;

  createTextures();
  initialized_ = true;
}

void LayerScreenshot::createTextures() {
  for (int i = 0; i < static_cast<int>(Layer::Count); ++i) {
    // Create texture with RGBA8888 format for alpha channel support
    // Use TARGET access so we can render to it
    layerTextures_[i] = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width_,
        height_);

    if (!layerTextures_[i]) {
      std::fprintf(stderr, "Failed to create layer texture %d: %s\n", i, SDL_GetError());
      continue;
    }

    // Enable alpha blending for this texture
    SDL_SetTextureBlendMode(layerTextures_[i], SDL_BLENDMODE_BLEND);
  }
}

void LayerScreenshot::destroyTextures() {
  for (int i = 0; i < static_cast<int>(Layer::Count); ++i) {
    if (layerTextures_[i]) {
      SDL_DestroyTexture(layerTextures_[i]);
      layerTextures_[i] = nullptr;
    }
  }
  initialized_ = false;
}

void LayerScreenshot::beginLayer(Layer layer) {
  if (!initialized_ || layer >= Layer::Count) {
    return;
  }

  capturing_ = true;
  currentLayer_ = layer;

  SDL_Texture* texture = layerTextures_[static_cast<int>(layer)];
  if (!texture) {
    return;
  }

  // Set render target to this layer's texture
  SDL_SetRenderTarget(renderer_, texture);

  // Clear to fully transparent
  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
  SDL_RenderClear(renderer_);
}

void LayerScreenshot::endLayer() {
  if (!capturing_) {
    return;
  }

  // Restore default render target
  SDL_SetRenderTarget(renderer_, nullptr);
  currentLayer_ = Layer::Count;
}

const char* LayerScreenshot::getLayerName(Layer layer) {
  switch (layer) {
    case Layer::Background:     return "01_background";
    case Layer::MapLines:       return "02_map_lines";
    case Layer::MapText:        return "03_map_text";
    case Layer::Airports:       return "04_airports";
    case Layer::ScaleBars:      return "05_scale_bars";
    case Layer::AircraftTrails: return "06_aircraft_trails";
    case Layer::AircraftIcons:  return "07_aircraft_icons";
    case Layer::OffMapAircraft: return "08_offmap_aircraft";
    case Layer::LabelLines:     return "09_label_lines";
    case Layer::LabelText:      return "10_label_text";
    case Layer::StatusButtons:  return "11_status_buttons";
    case Layer::StatusText:     return "12_status_text";
    case Layer::InputFeedback:  return "13_input_feedback";
    default:                    return "unknown";
  }
}

std::string LayerScreenshot::saveAllLayers() {
  if (!initialized_) {
    std::fprintf(stderr, "LayerScreenshot not initialized\n");
    return "";
  }

  // Generate timestamp folder name
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::tm tm_now{};
  localtime_r(&time_t_now, &tm_now);

  std::ostringstream folderStream;
  folderStream << "screenshots/"
               << std::put_time(&tm_now, "%Y%m%d_%H%M%S");
  std::string folderPath = folderStream.str();

  // Create directory
  std::error_code ec;
  std::filesystem::create_directories(folderPath, ec);
  if (ec) {
    std::fprintf(stderr, "Failed to create screenshot directory '%s': %s\n",
                 folderPath.c_str(), ec.message().c_str());
    return "";
  }

  std::fprintf(stderr, "Saving layers to: %s\n", folderPath.c_str());

  // Save each layer
  int savedCount = 0;
  for (int i = 0; i < static_cast<int>(Layer::Count); ++i) {
    Layer layer = static_cast<Layer>(i);
    std::string filename = folderPath + "/" + getLayerName(layer) + ".png";

    if (saveLayerToPNG(layer, filename)) {
      savedCount++;
    }
  }

  std::fprintf(stderr, "Saved %d/%d layers\n", savedCount, static_cast<int>(Layer::Count));

  capturing_ = false;
  return folderPath;
}

bool LayerScreenshot::saveLayerToPNG(Layer layer, const std::string& filepath) {
  SDL_Texture* texture = layerTextures_[static_cast<int>(layer)];
  if (!texture) {
    return false;
  }

  // Create a surface to read pixel data into
  SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
      0, width_, height_, 32, SDL_PIXELFORMAT_RGBA32);

  if (!surface) {
    std::fprintf(stderr, "Failed to create surface for %s: %s\n",
                 getLayerName(layer), SDL_GetError());
    return false;
  }

  // Set render target to the texture so we can read from it
  SDL_SetRenderTarget(renderer_, texture);

  // Read pixels from the texture
  if (SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_RGBA32,
                           surface->pixels, surface->pitch) != 0) {
    std::fprintf(stderr, "Failed to read pixels for %s: %s\n",
                 getLayerName(layer), SDL_GetError());
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(renderer_, nullptr);
    return false;
  }

  // Restore default render target
  SDL_SetRenderTarget(renderer_, nullptr);

  // Save to PNG using SDL_image
  if (IMG_SavePNG(surface, filepath.c_str()) != 0) {
    std::fprintf(stderr, "Failed to save PNG %s: %s\n",
                 filepath.c_str(), IMG_GetError());
    SDL_FreeSurface(surface);
    return false;
  }

  SDL_FreeSurface(surface);
  return true;
}

}  // namespace viz1090
