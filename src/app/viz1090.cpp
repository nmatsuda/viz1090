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

#include "app/AppData.h"
#include "ui/Input.h"
#include "ui/View.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>


//
// ================================ Main ====================================
//
void
showHelp() {
  std::printf(
      "-----------------------------------------------------------------------------\n"
      "|                        viz1090 ADSB Viewer        Ver : 0.2 |\n"
      "-----------------------------------------------------------------------------\n"
      "--flip-touch                     Flip touchscreen X and Y coordinates\n"
      "--fps                            Show current framerate\n"
      "--fullscreen                     Start fullscreen\n"
      "--help                           Show this help\n"
      "--lat <latitude>                 Latitude in degrees\n"
      "--lon <longitude>                Longitude in degrees\n"
      "--metric                         Use metric units\n"
      "--port <port>                    TCP Beast output listen port (default: 30005)\n"
      "--server <IPv4/hosname>          TCP Beast output listen IPv4 (default: 127.0.0.1)\n"
      "--screensize <width> <height>    Set frame buffer resolution (default: screen "
      "resolution)\n"
      "--screenindex <i>                Set the index of the display to use (default: 0)\n"
      "--uiscale <factor>               UI global scaling (default: 1)\n");
}

//
//=========================================================================
//

int
main(int argc, char** argv) {
  AppData appData;
  viz1090::View view(&appData);
  bool flipTouch = false;

  // Parse the command line options
  for (int j = 1; j < argc; j++) {
    int more = ((j + 1) < argc);  // There are more arguments

    if (!std::strcmp(argv[j], "--port") && more) {
      appData.port = static_cast<uint16_t>(std::atoi(argv[++j]));
    } else if (!std::strcmp(argv[j], "--server") && more) {
      appData.server = argv[++j];
    } else if (!std::strcmp(argv[j], "--lat") && more) {
      appData.userLat = std::atof(argv[++j]);
      view.getMapView().centerLat = static_cast<float>(appData.userLat);
      view.getMapView().originLat = view.getMapView().centerLat;
    } else if (!std::strcmp(argv[j], "--lon") && more) {
      appData.userLon = std::atof(argv[++j]);
      view.getMapView().centerLon = static_cast<float>(appData.userLon);
      view.getMapView().originLon = view.getMapView().centerLon;
    } else if (!std::strcmp(argv[j], "--metric")) {
      view.metric = 1;
    } else if (!std::strcmp(argv[j], "--fps")) {
      view.getUIOverlay()->setShowFps(1);
    } else if (!std::strcmp(argv[j], "--fullscreen")) {
      view.fullscreen = 1;
    } else if (!std::strcmp(argv[j], "--flip-touch")) {
      flipTouch = true;
    } else if (!std::strcmp(argv[j], "--screenindex")) {
      view.screen_index = std::atoi(argv[++j]);
    } else if (!std::strcmp(argv[j], "--uiscale") && more) {
      view.screen_uiscale = std::atoi(argv[++j]);
    } else if (!std::strcmp(argv[j], "--screensize") && more) {
      view.screen_width = std::atoi(argv[++j]);
      view.screen_height = std::atoi(argv[++j]);
    } else if (!std::strcmp(argv[j], "--help")) {
      showHelp();
      std::exit(0);
    } else {
      std::fprintf(stderr, "Unknown or not enough arguments for option '%s'.\n\n",
                   argv[j]);
      showHelp();
      std::exit(1);
    }
  }

  appData.initialize();

  view.SDL_init();
  view.font_init();

  Input input(&appData, &view);
  input.flipTouch = flipTouch;

  std::signal(SIGINT, SIG_DFL);  // reset signal handler - bit extra safety

  // Start connection
  appData.connect();

  // Show keyboard shortcuts at startup
  printKeyboardShortcuts();

  bool running = true;
  while (running) {
    input.getInput();
    view.draw();
    appData.update();
  }

  appData.disconnect();

  return 0;
}
//
//=========================================================================
//
