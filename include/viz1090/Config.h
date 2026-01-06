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

#ifndef VIZ1090_CONFIG_H
#define VIZ1090_CONFIG_H

#include <chrono>
#include <cstdint>
#include <string>

#include "Types.h"

namespace viz1090 {

/// Application configuration
///
/// Centralizes all configurable parameters with sensible defaults.
/// Can be populated from command-line arguments or config file.
struct AppConfig {
  // Network settings
  struct Network {
    std::string host = "127.0.0.1";
    uint16_t port = 30005;  // Beast output port
    bool autoReconnect = true;
    Seconds reconnectDelay{5};
  } network;

  // Display settings
  struct Display {
    int width = 0;          // 0 = auto-detect
    int height = 0;         // 0 = auto-detect
    int upscale = 1;        // Render upscale factor
    int uiScale = 1;        // UI element scale
    bool fullscreen = false;
    int screenIndex = 0;    // Which display to use
    bool vsync = true;
    bool showFps = false;
  } display;

  // Map settings
  struct Map {
    double centerLat = 0.0;
    double centerLon = 0.0;
    double maxDistance = 200.0;  // km or nm depending on metric
    bool metric = true;
    std::string dataPath = "mapdata";
  } map;

  // Aircraft display settings
  struct Aircraft {
    Seconds displayTtl{30};      // How long to show aircraft after last message
    Seconds deleteTtl{300};      // How long before removing aircraft
    int trailLength = 120;       // Number of trail points
    Seconds trailTtl{240};       // How long trail points live
    bool showLabels = true;
    bool showTrails = true;
    bool showSignalBars = true;
  } aircraft;

  // Receiver settings
  struct Receiver {
    double latitude = 0.0;
    double longitude = 0.0;
    bool modeAC = false;  // Enable Mode A/C decoding
  } receiver;

  // Font settings
  struct Fonts {
    std::string path = "font";
    std::string mapFont = "TerminusTTF-4.47.0.ttf";
    std::string labelFont = "TerminusTTF-4.47.0.ttf";
    int mapFontSize = 12;
    int labelFontSize = 12;
    int messageFontSize = 18;
  } fonts;
};

/// View constants (previously #defines)
namespace ViewConstants {
  inline constexpr int kRoundRadius = 3;
  inline constexpr int kPadding = 5;
  inline constexpr int kMinMapFeature = 2;
  inline constexpr int kFrameTimeMs = 33;  // ~30 fps
  inline constexpr double kLatLonMult = 111.195;  // km per degree
}  // namespace ViewConstants

/// Parse command-line arguments into config
/// @param aArgc Argument count
/// @param aArgv Argument values
/// @param aConfig Config to populate
/// @return true if parsing succeeded, false if help was requested or error
bool parseArgs(int aArgc, char* aArgv[], AppConfig& aConfig);

/// Print usage information
void printUsage(const char* aProgramName);

}  // namespace viz1090

#endif  // VIZ1090_CONFIG_H
