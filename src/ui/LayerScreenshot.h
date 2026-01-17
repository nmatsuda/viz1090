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

#ifndef LAYER_SCREENSHOT_H
#define LAYER_SCREENSHOT_H

#include <SDL2/SDL.h>
#include <string>

namespace viz1090 {

/// Manages layered screenshot capture to RGBA PNGs
/// Each layer is rendered to a separate texture with transparency support
class LayerScreenshot {
public:
  /// Layer identifiers for the different rendering passes
  enum class Layer {
    Background,      // Solid background color
    MapLines,        // Geography lines (coastlines, borders)
    MapText,         // Place names
    Airports,        // Airport lines
    ScaleBars,       // Scale bar markings
    AircraftTrails,  // Trail lines behind aircraft
    AircraftIcons,   // Plane icons
    OffMapAircraft,  // Off-screen aircraft arrows
    LabelLines,      // Lines connecting labels to aircraft
    LabelText,       // Aircraft label text
    StatusButtons,   // Status bar backgrounds
    StatusText,      // Status bar text
    InputFeedback,   // Click ripples, selection brackets, cursor
    Count            // Number of layers
  };

  LayerScreenshot() = default;
  ~LayerScreenshot();

  /// Initialize the screenshot system with renderer and dimensions
  /// Must be called before capturing layers
  void init(SDL_Renderer* renderer, int width, int height);

  /// Begin capturing a layer - sets render target to layer texture
  /// The texture is cleared to transparent before rendering
  void beginLayer(Layer layer);

  /// End capturing current layer - restores default render target
  void endLayer();

  /// Save all captured layers to timestamped folder
  /// Creates folder: screenshots/YYYYMMDD_HHMMSS/
  /// Returns the folder path on success, empty string on failure
  std::string saveAllLayers();

  /// Check if screenshot capture is currently active
  bool isCapturing() const { return capturing_; }

  /// Get layer name for file naming
  static const char* getLayerName(Layer layer);

private:
  /// Create textures for all layers
  void createTextures();

  /// Destroy all textures
  void destroyTextures();

  /// Save a single layer texture to PNG file
  bool saveLayerToPNG(Layer layer, const std::string& filepath);

  SDL_Renderer* renderer_{nullptr};
  int width_{0};
  int height_{0};
  bool initialized_{false};
  bool capturing_{false};
  Layer currentLayer_{Layer::Count};

  /// Textures for each layer (with alpha channel support)
  SDL_Texture* layerTextures_[static_cast<int>(Layer::Count)]{};
};

}  // namespace viz1090

#endif  // LAYER_SCREENSHOT_H
