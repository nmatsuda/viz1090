#include "AppData.h"

//
//carried over from view1090.c
//


int AppData::setupConnection(struct client *c) {
    int fd;

    // Try to connect to the selected ip address and port. We only support *ONE* input connection which we initiate.here.
    if ((fd = anetTcpConnect(modes.aneterr, server, modes.net_input_beast_port)) != ANET_ERR) {
		anetNonBlock(modes.aneterr, fd);
		//
		// Setup a service callback client structure for a beast binary input (from dump1090)
		// This is a bit dodgy under Windows. The fd parameter is a handle to the internet
		// socket on which we are receiving data. Under Linux, these seem to start at 0 and 
		// count upwards. However, Windows uses "HANDLES" and these don't nececeriy start at 0.
		// dump1090 limits fd to values less than 1024, and then uses the fd parameter to 
		// index into an array of clients. This is ok-ish if handles are allocated up from 0.
		// However, there is no gaurantee that Windows will behave like this, and if Windows 
		// allocates a handle greater than 1024, then dump1090 won't like it. On my test machine, 
		// the first Windows handle is usually in the 0x54 (84 decimal) region.

		c->next    = NULL;
		c->buflen  = 0;
		c->fd      = 
		c->service =
		modes.bis  = fd;
		modes.clients = c;
    }
    return fd;
}

//
// end view1090.c
//


void AppData::initialize() {
    // Allocate the various buffers used by Modes
    if ( NULL == (modes.icao_cache = (uint32_t *) malloc(sizeof(uint32_t) * MODES_ICAO_CACHE_LEN * 2)))
    {
        fprintf(stderr, "Out of memory allocating data buffer.\n");
        exit(1);
    }

    // Clear the buffers that have just been allocated, just in-case
    memset(modes.icao_cache, 0,   sizeof(uint32_t) * MODES_ICAO_CACHE_LEN * 2);

    // Prepare error correction tables
    modesInitErrorInfo(&(modes));
}


void AppData::connect() {
    // Try to connect to the selected ip address and port. We only support *ONE* input connection which we initiate.here.
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
    modesReadFromClient(&modes, c,"",decodeBinMessage);

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


  //   PlaneObj *p = appData.planes;

  //   while(p) {
        // unsigned char * pSig       = p->signalLevel;
        // unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
        //                               pSig[4] + pSig[5] + pSig[6] + pSig[7]);   

        // sigAccumulate += signalAverage;
        
        // if (p->lon && p->lat) {
        //         numVisiblePlanes++;
        // }    

        // totalCount++;

  //       msgRateAccumulate += p->messageRate; 

  //       p = p->next;
  //   }

    // Status.msgRate                = msgRateAccumulate;
    // Status.avgSig                 = sigAccumulate / (double) totalCount;
    // Status.numPlanes              = totalCount;
    // Status.numVisiblePlanes       = numVisiblePlanes;
    // Status.maxDist                = maxDist;
}


AppData::AppData(){
    // Default everything to zero/NULL
    memset(&modes,    0, sizeof(Modes));

    // Now initialise things that should not be 0/NULL to their defaults
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
