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

#include "AppData.h"
#include "View.h"
#include "Input.h"
#include <cstring> 

int go = 1;


AppData appData;
Style style;

//
// ================================ Main ====================================
//
void showHelp(void) {
    printf(
"-----------------------------------------------------------------------------\n"
"|                        viz1090 ADSB Viewer        Ver : 0.1 |\n"
"-----------------------------------------------------------------------------\n"
  "--server <IPv4/hosname>          TCP Beast output listen IPv4 (default: 127.0.0.1)\n"
  "--port <port>                    TCP Beast output listen port (default: 30005)\n"
  "--lat <latitude>                 Latitide in degrees\n"
  "--lon <longitude>                Longitude in degrees\n"
  "--metric                         Use metric units\n"
  "--help                           Show this help\n"
  "--uiscale <factor>               UI global scaling (default: 1)\n"  
  "--screensize <width> <height>    Set frame buffer resolution (default: screen resolution)\n"
  "--screenindex <i>                Set the index of the display to use (default: 0)\n"
  "--fullscreen                     Start fullscreen\n"
    );
}


//
//=========================================================================
//


int main(int argc, char **argv) {
    int j;

    AppData appData;
    View view(&appData);
    Input input(&appData,&view);

    signal(SIGINT, SIG_DFL);  // reset signal handler - bit extra safety

    appData.initialize();

    // Parse the command line options
    for (j = 1; j < argc; j++) {
        int more = ((j + 1) < argc); // There are more arguments

        if        (!strcmp(argv[j],"--port") && more) {
            appData.modes.net_input_beast_port = atoi(argv[++j]);
        } else if (!strcmp(argv[j],"--server") && more) {
            std::strcpy(appData.server, argv[++j]);            
        } else if (!strcmp(argv[j],"--lat") && more) {
            appData.modes.fUserLat = atof(argv[++j]);
            view.centerLat = appData.modes.fUserLat;
        } else if (!strcmp(argv[j],"--lon") && more) {
            appData.modes.fUserLon = atof(argv[++j]);
            view.centerLon = appData.modes.fUserLon;
        } else if (!strcmp(argv[j],"--metric")) {
            view.metric = 1;
        } else if (!strcmp(argv[j],"--fullscreen")) {
            view.fullscreen = 1;         
        } else if (!strcmp(argv[j],"--screenindex")) {
            view.screen_index = atoi(argv[++j]);         
        } else if (!strcmp(argv[j],"--uiscale") && more) {
            view.screen_uiscale = atoi(argv[++j]);   
         } else if (!strcmp(argv[j],"--screensize") && more) {
            view.screen_width = atoi(argv[++j]);        
            view.screen_height = atoi(argv[++j]);        
        } else if (!strcmp(argv[j],"--help")) {
            showHelp();
            exit(0);
        } else {
            fprintf(stderr, "Unknown or not enough arguments for option '%s'.\n\n", argv[j]);
            showHelp();
            exit(1);
        }
    }
    
    int go;

    appData.connect();
  
    
    view.SDL_init();
    view.font_init();
            
    go = 1;
          
    while (go == 1)
    {
        input.getInput();
        appData.update();
        view.draw();
    }
    
    appData.disconnect();

    return (0);
}
//
//=========================================================================
//
