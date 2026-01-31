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

#include "viz1090/StyleManager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace viz1090 {

// Simple JSON parsing helpers (minimal implementation to avoid dependencies)
namespace {

std::string trim(const std::string& aStr) {
  auto start = aStr.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) return "";
  auto end = aStr.find_last_not_of(" \t\n\r");
  return aStr.substr(start, end - start + 1);
}

// Parse a JSON array of integers like [255, 128, 64] or [255, 128, 64, 255]
std::vector<int> parseIntArray(const std::string& aJson) {
  std::vector<int> result;
  std::string nums = aJson;

  // Remove brackets
  auto start = nums.find('[');
  auto end = nums.find(']');
  if (start != std::string::npos && end != std::string::npos) {
    nums = nums.substr(start + 1, end - start - 1);
  }

  std::stringstream ss(nums);
  std::string token;
  while (std::getline(ss, token, ',')) {
    try {
      result.push_back(std::stoi(trim(token)));
    } catch (...) {
      // Skip invalid entries
    }
  }
  return result;
}

// Extract string value for a key from JSON
std::string extractString(const std::string& aJson, const std::string& aKey) {
  std::string searchKey = "\"" + aKey + "\"";
  auto pos = aJson.find(searchKey);
  if (pos == std::string::npos) return "";

  pos = aJson.find(':', pos);
  if (pos == std::string::npos) return "";

  auto startQuote = aJson.find('"', pos + 1);
  if (startQuote == std::string::npos) return "";

  auto endQuote = aJson.find('"', startQuote + 1);
  if (endQuote == std::string::npos) return "";

  return aJson.substr(startQuote + 1, endQuote - startQuote - 1);
}

// Extract array value for a key from JSON
std::string extractArray(const std::string& aJson, const std::string& aKey) {
  std::string searchKey = "\"" + aKey + "\"";
  auto pos = aJson.find(searchKey);
  if (pos == std::string::npos) return "";

  pos = aJson.find(':', pos);
  if (pos == std::string::npos) return "";

  auto startBracket = aJson.find('[', pos);
  if (startBracket == std::string::npos) return "";

  // Find matching closing bracket
  int depth = 1;
  size_t endBracket = startBracket + 1;
  while (endBracket < aJson.size() && depth > 0) {
    if (aJson[endBracket] == '[') depth++;
    else if (aJson[endBracket] == ']') depth--;
    endBracket++;
  }

  return aJson.substr(startBracket, endBracket - startBracket);
}

// Extract object value for a key from JSON
std::string extractObject(const std::string& aJson, const std::string& aKey) {
  std::string searchKey = "\"" + aKey + "\"";
  auto pos = aJson.find(searchKey);
  if (pos == std::string::npos) return "";

  pos = aJson.find(':', pos);
  if (pos == std::string::npos) return "";

  auto startBrace = aJson.find('{', pos);
  if (startBrace == std::string::npos) return "";

  // Find matching closing brace
  int depth = 1;
  size_t endBrace = startBrace + 1;
  while (endBrace < aJson.size() && depth > 0) {
    if (aJson[endBrace] == '{') depth++;
    else if (aJson[endBrace] == '}') depth--;
    endBrace++;
  }

  return aJson.substr(startBrace, endBrace - startBrace);
}

// Parse all color arrays from a colors/palette object
std::unordered_map<std::string, std::vector<int>> parseColorObject(
    const std::string& aJson) {
  std::unordered_map<std::string, std::vector<int>> result;

  // Find all "key": [r, g, b, a] patterns
  size_t pos = 0;
  while (pos < aJson.size()) {
    auto keyStart = aJson.find('"', pos);
    if (keyStart == std::string::npos) break;

    auto keyEnd = aJson.find('"', keyStart + 1);
    if (keyEnd == std::string::npos) break;

    std::string key = aJson.substr(keyStart + 1, keyEnd - keyStart - 1);

    auto colonPos = aJson.find(':', keyEnd);
    if (colonPos == std::string::npos) break;

    auto bracketPos = aJson.find('[', colonPos);
    auto nextQuote = aJson.find('"', colonPos);
    auto nextBrace = aJson.find('{', colonPos);

    // Check if next value is an array (not a string or object)
    if (bracketPos != std::string::npos &&
        (nextQuote == std::string::npos || bracketPos < nextQuote) &&
        (nextBrace == std::string::npos || bracketPos < nextBrace)) {
      auto bracketEnd = aJson.find(']', bracketPos);
      if (bracketEnd != std::string::npos) {
        std::string arrayStr = aJson.substr(bracketPos, bracketEnd - bracketPos + 1);
        result[key] = parseIntArray(arrayStr);
        pos = bracketEnd + 1;
        continue;
      }
    }

    pos = keyEnd + 1;
  }

  return result;
}

}  // namespace

SDL_Color Colormap::at(size_t aIndex) const {
  if (colors.empty()) {
    return {0, 0, 0, 255};
  }
  size_t idx = std::min(aIndex, colors.size() - 1);
  return {colors[idx][0], colors[idx][1], colors[idx][2], 255};
}

SDL_Color Colormap::sample(float aNormalized) const {
  if (colors.empty()) {
    return {0, 0, 0, 255};
  }

  float clamped = std::max(0.0f, std::min(1.0f, aNormalized));
  float idx = clamped * static_cast<float>(colors.size() - 1);

  size_t lower = static_cast<size_t>(std::floor(idx));
  size_t upper = std::min(lower + 1, colors.size() - 1);
  float t = idx - static_cast<float>(lower);

  // Linear interpolation
  auto lerp = [t](uint8_t a, uint8_t b) -> uint8_t {
    return static_cast<uint8_t>(
        static_cast<float>(a) * (1.0f - t) + static_cast<float>(b) * t);
  };

  return {lerp(colors[lower][0], colors[upper][0]),
          lerp(colors[lower][1], colors[upper][1]),
          lerp(colors[lower][2], colors[upper][2]), 255};
}

StyleManager::StyleManager() {
  initDefaults();
}

void StyleManager::initDefaults() {
  // Initialize default theme
  mCurrentTheme.name = "Default";
  mCurrentTheme.description = "Built-in default theme";

  // Initialize default colormap (simple grayscale)
  mCurrentColormap.name = "Grayscale";
  mCurrentColormap.description = "Built-in grayscale colormap";
  mCurrentColormap.colors.resize(128);
  for (size_t i = 0; i < 128; ++i) {
    uint8_t val = static_cast<uint8_t>(i * 2);
    mCurrentColormap.colors[i] = {val, val, val};
  }
}

SDL_Color StyleManager::parseColor(const std::vector<int>& aValues) {
  SDL_Color color{0, 0, 0, 255};
  if (aValues.size() >= 3) {
    color.r = static_cast<uint8_t>(std::clamp(aValues[0], 0, 255));
    color.g = static_cast<uint8_t>(std::clamp(aValues[1], 0, 255));
    color.b = static_cast<uint8_t>(std::clamp(aValues[2], 0, 255));
    if (aValues.size() >= 4) {
      color.a = static_cast<uint8_t>(std::clamp(aValues[3], 0, 255));
    }
  }
  return color;
}

bool StyleManager::loadFromDirectory(std::string_view aThemesDir) {
  bool loaded = false;

  try {
    fs::path themesPath(aThemesDir);

    // Load theme files from root of themes directory
    if (fs::exists(themesPath)) {
      for (const auto& entry : fs::directory_iterator(themesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
          if (loadTheme(entry.path().string())) {
            loaded = true;
          }
        }
      }
    }

    // Load colormap files from colormaps subdirectory
    fs::path colormapsPath = themesPath / "colormaps";
    if (fs::exists(colormapsPath)) {
      for (const auto& entry : fs::directory_iterator(colormapsPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
          loadColormap(entry.path().string());
        }
      }
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "Error loading themes: %s\n", e.what());
  }

  // Set default theme if available
  if (!mThemes.empty()) {
    if (mThemes.count("Default")) {
      setTheme("Default");
    } else {
      setTheme(mThemes.begin()->first);
    }
  }

  // Set default colormap if available
  if (!mColormaps.empty()) {
    if (mColormaps.count("Parula")) {
      setColormap("Parula");
    } else {
      setColormap(mColormaps.begin()->first);
    }
  }

  return loaded;
}

bool StyleManager::loadTheme(std::string_view aPath) {
  try {
    std::string pathStr{aPath};
    std::ifstream file{pathStr};
    if (!file.is_open()) {
      return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    Theme theme;
    theme.name = extractString(json, "name");
    theme.description = extractString(json, "description");

    if (theme.name.empty()) {
      // Use filename as name
      fs::path p(aPath);
      theme.name = p.stem().string();
    }

    // Parse colors object
    std::string colorsJson = extractObject(json, "colors");
    if (!colorsJson.empty()) {
      auto colors = parseColorObject(colorsJson);

      if (colors.count("background")) theme.backgroundColor = parseColor(colors["background"]);
      if (colors.count("selected")) theme.selectedColor = parseColor(colors["selected"]);
      if (colors.count("plane")) theme.planeColor = parseColor(colors["plane"]);
      if (colors.count("planeGone")) theme.planeGoneColor = parseColor(colors["planeGone"]);
      if (colors.count("trail")) theme.trailColor = parseColor(colors["trail"]);
      if (colors.count("geo")) theme.geoColor = parseColor(colors["geo"]);
      if (colors.count("countryBorder")) theme.countryBorderColor = parseColor(colors["countryBorder"]);
      if (colors.count("coastline")) theme.coastlineColor = parseColor(colors["coastline"]);
      if (colors.count("airport")) theme.airportColor = parseColor(colors["airport"]);
      if (colors.count("label")) theme.labelColor = parseColor(colors["label"]);
      if (colors.count("labelLine")) theme.labelLineColor = parseColor(colors["labelLine"]);
      if (colors.count("subLabel")) theme.subLabelColor = parseColor(colors["subLabel"]);
      if (colors.count("labelBackground")) theme.labelBackground = parseColor(colors["labelBackground"]);
      if (colors.count("scaleBar")) theme.scaleBarColor = parseColor(colors["scaleBar"]);
      if (colors.count("button")) theme.buttonColor = parseColor(colors["button"]);
      if (colors.count("buttonBackground")) theme.buttonBackground = parseColor(colors["buttonBackground"]);
      if (colors.count("buttonTextColor")) theme.buttonTextColor = parseColor(colors["buttonTextColor"]);
      if (colors.count("buttonOutline")) theme.buttonOutline = parseColor(colors["buttonOutline"]);
      if (colors.count("click")) theme.clickColor = parseColor(colors["click"]);
    }

    // Parse palette object
    std::string paletteJson = extractObject(json, "palette");
    if (!paletteJson.empty()) {
      auto palette = parseColorObject(paletteJson);

      if (palette.count("black")) theme.black = parseColor(palette["black"]);
      if (palette.count("white")) theme.white = parseColor(palette["white"]);
      if (palette.count("red")) theme.red = parseColor(palette["red"]);
      if (palette.count("green")) theme.green = parseColor(palette["green"]);
      if (palette.count("blue")) theme.blue = parseColor(palette["blue"]);
      if (palette.count("orange")) theme.orange = parseColor(palette["orange"]);
      if (palette.count("grey")) theme.grey = parseColor(palette["grey"]);
      if (palette.count("grey_dark")) theme.grey_dark = parseColor(palette["grey_dark"]);
    }

    mThemes[theme.name] = theme;
    std::fprintf(stderr, "Loaded theme: %s\n", theme.name.c_str());
    return true;

  } catch (const std::exception& e) {
    std::fprintf(stderr, "Error loading theme %s: %s\n",
                 std::string(aPath).c_str(), e.what());
    return false;
  }
}

bool StyleManager::loadColormap(std::string_view aPath) {
  try {
    std::string pathStr{aPath};
    std::ifstream file{pathStr};
    if (!file.is_open()) {
      return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    Colormap cmap;
    cmap.name = extractString(json, "name");
    cmap.description = extractString(json, "description");

    if (cmap.name.empty()) {
      fs::path p(aPath);
      cmap.name = p.stem().string();
    }

    // Parse colors array
    std::string colorsArray = extractArray(json, "colors");
    if (!colorsArray.empty()) {
      // Parse array of arrays [[r,g,b], [r,g,b], ...]
      size_t pos = 0;
      while (pos < colorsArray.size()) {
        auto innerStart = colorsArray.find('[', pos);
        if (innerStart == std::string::npos || innerStart == 0) break;

        // Skip the outer array bracket
        if (colorsArray[innerStart - 1] == '[' ||
            colorsArray[innerStart - 1] == ',') {
          // This is an inner array
        }

        auto innerEnd = colorsArray.find(']', innerStart);
        if (innerEnd == std::string::npos) break;

        std::string inner = colorsArray.substr(innerStart, innerEnd - innerStart + 1);
        auto values = parseIntArray(inner);

        if (values.size() >= 3) {
          cmap.colors.push_back({
              static_cast<uint8_t>(std::clamp(values[0], 0, 255)),
              static_cast<uint8_t>(std::clamp(values[1], 0, 255)),
              static_cast<uint8_t>(std::clamp(values[2], 0, 255))});
        }

        pos = innerEnd + 1;
      }
    }

    if (!cmap.colors.empty()) {
      mColormaps[cmap.name] = cmap;
      std::fprintf(stderr, "Loaded colormap: %s (%zu colors)\n",
                   cmap.name.c_str(), cmap.colors.size());
      return true;
    }

    return false;

  } catch (const std::exception& e) {
    std::fprintf(stderr, "Error loading colormap %s: %s\n",
                 std::string(aPath).c_str(), e.what());
    return false;
  }
}

std::vector<std::string> StyleManager::themeNames() const {
  std::vector<std::string> names;
  names.reserve(mThemes.size());
  for (const auto& [name, _] : mThemes) {
    names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
}

std::vector<std::string> StyleManager::colormapNames() const {
  std::vector<std::string> names;
  names.reserve(mColormaps.size());
  for (const auto& [name, _] : mColormaps) {
    names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
}

std::optional<Theme> StyleManager::theme(std::string_view aName) const {
  auto it = mThemes.find(std::string(aName));
  if (it != mThemes.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<Colormap> StyleManager::colormap(std::string_view aName) const {
  auto it = mColormaps.find(std::string(aName));
  if (it != mColormaps.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool StyleManager::setTheme(std::string_view aName) {
  auto it = mThemes.find(std::string(aName));
  if (it != mThemes.end()) {
    mCurrentTheme = it->second;
    return true;
  }
  return false;
}

bool StyleManager::setColormap(std::string_view aName) {
  auto it = mColormaps.find(std::string(aName));
  if (it != mColormaps.end()) {
    mCurrentColormap = it->second;
    return true;
  }
  return false;
}

}  // namespace viz1090
