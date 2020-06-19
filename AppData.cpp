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

//
//carried over from view1090.c
//

int AppData::setupConnection(struct client *c) {
    int fd;

    if ((fd = anetTcpConnect(modes.aneterr, server, modes.net_input_beast_port)) != ANET_ERR) {
		anetNonBlock(modes.aneterr, fd);
		c->next    = NULL;
		c->buflen  = 0;
		c->fd      = 
		c->service =
		modes.bis  = fd;
		modes.clients = c;
    }
    return fd;
}

void AppData::initialize() {
    if ( NULL == (modes.icao_cache = (uint32_t *) malloc(sizeof(uint32_t) * MODES_ICAO_CACHE_LEN * 2)))
    {
        fprintf(stderr, "Out of memory allocating data buffer.\n");
        exit(1);
    }
    memset(modes.icao_cache, 0,   sizeof(uint32_t) * MODES_ICAO_CACHE_LEN * 2);
    modesInitErrorInfo(&(modes));
}


void AppData::connect() {
    c = (struct client *) malloc(sizeof(*c));
    while(1) {
        if ((fd = setupConnection(c)) == ANET_ERR) {
            fprintf(stderr, "Waiting on %s:%d\n", server, modes.net_input_beast_port);     
            sleep(1);      
        } else {
            break;
        }
    }

}


void AppData::disconnect() {
    if (fd != ANET_ERR) 
      {close(fd);}
}


void AppData::update() {
    if ((fd == ANET_ERR) || (recv(c->fd, pk_buf, sizeof(pk_buf), MSG_PEEK | MSG_DONTWAIT) == 0)) {
        free(c);
        usleep(1000000);
        c = (struct client *) malloc(sizeof(*c));
        fd = setupConnection(c);
        return;
    }
    char empty;
    modesReadFromClient(&modes, c, &empty,decodeBinMessage);

    interactiveRemoveStaleAircrafts(&modes);

    aircraftList.update(&modes);

    //this can probably be collapsed into somethingelse, came from status.c
    updateStatus();
}


void AppData::updateStatus() {
    // struct aircraft *a = Modes.aircrafts;

    numVisiblePlanes = 0;
    numPlanes = 0;
    maxDist = 0;
    totalCount = 0;
    sigAccumulate = 0.0;
    msgRateAccumulate = 0.0;    


     Aircraft *p = aircraftList.head;

     while(p) {
         unsigned char * pSig       = p->signalLevel;
         unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
                                       pSig[4] + pSig[5] + pSig[6] + pSig[7]);   

         sigAccumulate += signalAverage;
        
         if (p->lon && p->lat) {
                 numVisiblePlanes++;
         }    

         totalCount++;

         msgRateAccumulate += p->messageRate; 

         p = p->next;
     }

     msgRate                = msgRateAccumulate;
     avgSig                 = sigAccumulate / (double) totalCount;
     numPlanes              = totalCount;
     numVisiblePlanes       = numVisiblePlanes;
     maxDist                = maxDist;
}


AppData::AppData(){
    memset(&modes,    0, sizeof(Modes));

    modes.check_crc               = 1;
    strcpy(server,VIEW1090_NET_OUTPUT_IP_ADDRESS); 
    modes.net_input_beast_port    = MODES_NET_OUTPUT_BEAST_PORT;
    modes.interactive_rows        = MODES_INTERACTIVE_ROWS;
    modes.interactive_delete_ttl  = MODES_INTERACTIVE_DELETE_TTL;
    modes.interactive_display_ttl = MODES_INTERACTIVE_DISPLAY_TTL;
    modes.fUserLat                = MODES_USER_LATITUDE_DFLT;
    modes.fUserLon                = MODES_USER_LONGITUDE_DFLT;

    modes.interactive             = 0;
    modes.quiet                   = 1;
}
