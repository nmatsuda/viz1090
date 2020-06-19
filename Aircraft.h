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

#include <stdint.h>

#include <ctime>
#include <vector> 
#include <chrono>

class Aircraft {
public:	
    uint32_t        addr;           // ICAO address
    char            flight[16];     // Flight number
    unsigned char   signalLevel[8]; // Last 8 Signal Amplitudes
    double          messageRate;
    int             altitude;       // Altitude
    int             speed;          // Velocity
    int             track;          // Angle of flight
    int             vert_rate;      // Vertical rate.
    time_t          seen;           // Time at which the last packet was received
    time_t          seenLatLon;           // Time at which the last packet was received
    time_t          prev_seen;
    double          lat, lon;       // Coordinated obtained from CPR encoded data
    
    //history

    std::vector <float>   lonHistory, latHistory, headingHistory;
    std::vector <std::chrono::high_resolution_clock::time_point> timestampHistory;

    // float           oldLon[TRAIL_LENGTH];
    // float           oldLat[TRAIL_LENGTH];
    // float           oldHeading[TRAIL_LENGTH];
    // time_t          oldSeen[TRAIL_LENGTH];
    // uint8_t         oldIdx; 
    std::chrono::high_resolution_clock::time_point        created;
    std::chrono::high_resolution_clock::time_point        msSeen;
    std::chrono::high_resolution_clock::time_point        msSeenLatLon;
    int             live;

    struct Aircraft *next;        // Next aircraft in our linked list

//// label stuff -> should go to aircraft icon  class

    int             x, y, cx, cy, w, h;
    float           ox, oy, dox, doy, ddox, ddoy;
    float           pressure;

/// methods

    Aircraft(uint32_t addr);  
    ~Aircraft();
};