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

#include "viz1090/Config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace viz1090 {

void
printUsage(const char* aProgramName) {
  std::fprintf(stderr, R"(
Usage: %s [options]

Network options:
  --server <host>       Server hostname (default: 127.0.0.1)
  --port <port>         Server port (default: 30005)
  --no-reconnect        Disable auto-reconnect

Display options:
  --width <pixels>      Window width (0 = auto)
  --height <pixels>     Window height (0 = auto)
  --fullscreen          Start in fullscreen mode
  --screen <index>      Display index for fullscreen
  --upscale <factor>    Render upscale factor
  --uiscale <factor>    UI element scale factor
  --fps                 Show FPS counter

Map options:
  --lat <degrees>       Initial map center latitude
  --lon <degrees>       Initial map center longitude
  --dist <km>           Initial map radius
  --metric              Use metric units (km)
  --imperial            Use imperial units (nm)
  --mapdata <path>      Path to map data directory

Receiver options:
  --rx-lat <degrees>    Receiver latitude
  --rx-lon <degrees>    Receiver longitude
  --mode-ac             Enable Mode A/C decoding

Font options:
  --fontpath <path>     Path to font directory

Other options:
  --help                Show this help message

)", aProgramName);
}

bool
parseArgs(int aArgc, char* aArgv[], AppConfig& aConfig) {
  for (int i = 1; i < aArgc; i++) {
    const char* arg = aArgv[i];

    // Helper to get next argument
    auto nextArg = [&]() -> const char* {
      if (i + 1 >= aArgc) {
        std::fprintf(stderr, "Error: %s requires an argument\n", arg);
        return nullptr;
      }
      return aArgv[++i];
    };

    // Network options
    if (std::strcmp(arg, "--server") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.network.host = val;
    } else if (std::strcmp(arg, "--port") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.network.port = static_cast<uint16_t>(std::atoi(val));
    } else if (std::strcmp(arg, "--no-reconnect") == 0) {
      aConfig.network.autoReconnect = false;
    }
    // Display options
    else if (std::strcmp(arg, "--width") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.display.width = std::atoi(val);
    } else if (std::strcmp(arg, "--height") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.display.height = std::atoi(val);
    } else if (std::strcmp(arg, "--fullscreen") == 0) {
      aConfig.display.fullscreen = true;
    } else if (std::strcmp(arg, "--screen") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.display.screenIndex = std::atoi(val);
    } else if (std::strcmp(arg, "--upscale") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.display.upscale = std::atoi(val);
    } else if (std::strcmp(arg, "--uiscale") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.display.uiScale = std::atoi(val);
    } else if (std::strcmp(arg, "--fps") == 0) {
      aConfig.display.showFps = true;
    }
    // Map options
    else if (std::strcmp(arg, "--lat") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.map.centerLat = std::atof(val);
    } else if (std::strcmp(arg, "--lon") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.map.centerLon = std::atof(val);
    } else if (std::strcmp(arg, "--dist") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.map.maxDistance = std::atof(val);
    } else if (std::strcmp(arg, "--metric") == 0) {
      aConfig.map.metric = true;
    } else if (std::strcmp(arg, "--imperial") == 0) {
      aConfig.map.metric = false;
    } else if (std::strcmp(arg, "--mapdata") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.map.dataPath = val;
    }
    // Receiver options
    else if (std::strcmp(arg, "--rx-lat") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.receiver.latitude = std::atof(val);
    } else if (std::strcmp(arg, "--rx-lon") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.receiver.longitude = std::atof(val);
    } else if (std::strcmp(arg, "--mode-ac") == 0) {
      aConfig.receiver.modeAC = true;
    }
    // Font options
    else if (std::strcmp(arg, "--fontpath") == 0) {
      const char* val = nextArg();
      if (!val) return false;
      aConfig.fonts.path = val;
    }
    // Help
    else if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
      printUsage(aArgv[0]);
      return false;
    }
    // Unknown option
    else if (arg[0] == '-') {
      std::fprintf(stderr, "Unknown option: %s\n", arg);
      printUsage(aArgv[0]);
      return false;
    }
  }

  return true;
}

}  // namespace viz1090
