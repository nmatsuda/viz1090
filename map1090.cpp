// view1090, a Mode S messages viewer for dump1090 devices.
//
// Copyright (C) 2014 by Malcolm Robb <Support@ATTAvionics.com>
//
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

#include "view1090.h"
#include "structs.h"
#include "AppData.h"
#include "View.h"
#include "Input.h"

#include <cstring> 

//time utility, might change to std::chrono
uint64_t mstime(void) {
    struct timeval tv;
    uint64_t mst;

    gettimeofday(&tv, NULL);
    mst = ((uint64_t)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}


int go = 1;


AppData appData;
Style style;

//
// ================================ Main ====================================
//
void showHelp(void) {
    printf(
"-----------------------------------------------------------------------------\n"
"|                        view1090 dump1090 Viewer        Ver : " MODES_DUMP1090_VERSION " |\n"
"-----------------------------------------------------------------------------\n"
  "--server <IPv4/hosname>          TCP Beast output listen IPv4 (default: 127.0.0.1)\n"
  "--port <port>                    TCP Beast output listen port (default: 30005)\n"
  "--lat <latitude>                 Reference/receiver latitide for surface posn (opt)\n"
  "--lon <longitude>                Reference/receiver longitude for surface posn (opt)\n"
  "--metric                         Use metric units (meters, km/h, ...)\n"
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
