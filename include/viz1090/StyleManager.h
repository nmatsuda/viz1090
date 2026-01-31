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

#ifndef VIZ1090_STYLE_MANAGER_H
#define VIZ1090_STYLE_MANAGER_H

#include <SDL2/SDL.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace viz1090 {

/// A colormap for mapping scalar values to colors
struct Colormap {
  std::string name;
  std::string description;
  std::vector<std::array<uint8_t, 3>> colors;

  /// Get color at index (clamped to valid range)
  [[nodiscard]] SDL_Color at(size_t aIndex) const;

  /// Get interpolated color for normalized value [0, 1]
  [[nodiscard]] SDL_Color sample(float aNormalized) const;
};

/// Theme colors for UI elements
struct Theme {
  std::string name;
  std::string description;

  // UI element colors
  SDL_Color backgroundColor{0, 0, 0, 255};
  SDL_Color oceanColor{0, 0, 20, 255};             // Ocean/water background
  SDL_Color landColor{0, 0, 0, 255};               // Land fill color
  SDL_Color selectedColor{249, 38, 114, 255};
  SDL_Color planeColor{0, 255, 174, 255};
  SDL_Color planeGoneColor{127, 127, 127, 255};
  SDL_Color trailColor{0, 255, 174, 255};
  SDL_Color geoColor{33, 0, 122, 255};
  SDL_Color countryBorderColor{80, 80, 180, 255};  // National boundaries (brighter than state)
  SDL_Color coastlineColor{50, 50, 120, 255};      // Land-sea boundaries
  SDL_Color riverColor{30, 30, 100, 255};          // Rivers and lake centerlines
  SDL_Color lakeColor{30, 30, 100, 255};           // Lake boundaries
  SDL_Color airportColor{85, 0, 255, 255};
  SDL_Color labelColor{255, 255, 255, 255};
  SDL_Color labelLineColor{64, 64, 64, 255};
  SDL_Color subLabelColor{127, 127, 127, 255};
  SDL_Color labelBackground{0, 0, 0, 255};
  SDL_Color scaleBarColor{196, 196, 196, 255};
  SDL_Color buttonColor{196, 196, 196, 255};
  SDL_Color buttonBackground{0, 0, 0, 255};
  SDL_Color buttonTextColor{196, 196, 196, 255};
  SDL_Color buttonOutline{196, 196, 196, 255};
  SDL_Color clickColor{127, 127, 127, 255};

  // Named palette colors
  SDL_Color black{0, 0, 0, 255};
  SDL_Color white{255, 255, 255, 255};
  SDL_Color red{255, 0, 0, 255};
  SDL_Color green{0, 255, 0, 255};
  SDL_Color blue{0, 0, 255, 255};
  SDL_Color orange{253, 151, 31, 255};
  SDL_Color grey{127, 127, 127, 255};
  SDL_Color grey_dark{64, 64, 64, 255};  // Named to match original Style struct
};

/// Manages themes and colormaps loaded from JSON files
class StyleManager {
public:
  StyleManager();

  /// Load themes and colormaps from a directory
  /// @param aThemesDir Path to themes directory
  /// @return true if at least one theme was loaded
  bool loadFromDirectory(std::string_view aThemesDir);

  /// Load a single theme file
  /// @param aPath Path to theme JSON file
  /// @return true on success
  bool loadTheme(std::string_view aPath);

  /// Load a single colormap file
  /// @param aPath Path to colormap JSON file
  /// @return true on success
  bool loadColormap(std::string_view aPath);

  /// Get list of available theme names
  [[nodiscard]] std::vector<std::string> themeNames() const;

  /// Get list of available colormap names
  [[nodiscard]] std::vector<std::string> colormapNames() const;

  /// Get a theme by name
  [[nodiscard]] std::optional<Theme> theme(std::string_view aName) const;

  /// Get a colormap by name
  [[nodiscard]] std::optional<Colormap> colormap(std::string_view aName) const;

  /// Get the current active theme
  [[nodiscard]] const Theme& currentTheme() const { return mCurrentTheme; }

  /// Get the current active colormap
  [[nodiscard]] const Colormap& currentColormap() const {
    return mCurrentColormap;
  }

  /// Set the active theme by name
  /// @return true if theme was found and set
  bool setTheme(std::string_view aName);

  /// Set the active colormap by name
  /// @return true if colormap was found and set
  bool setColormap(std::string_view aName);

private:
  std::unordered_map<std::string, Theme> mThemes;
  std::unordered_map<std::string, Colormap> mColormaps;
  Theme mCurrentTheme;
  Colormap mCurrentColormap;

  static SDL_Color parseColor(const std::vector<int>& aValues);
  void initDefaults();
};

}  // namespace viz1090

#endif  // VIZ1090_STYLE_MANAGER_H
