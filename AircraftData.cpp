#include "AircraftData.h"

//
//carried over from view1090.c
//


int AircraftData::setupConnection(struct client *c) {
    int fd;

    // Try to connect to the selected ip address and port. We only support *ONE* input connection which we initiate.here.
    if ((fd = anetTcpConnect(modes.aneterr, View1090.net_input_beast_ipaddr, modes.net_input_beast_port)) != ANET_ERR) {
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


void AircraftData::initialize() {
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


void AircraftData::connect() {
    // Try to connect to the selected ip address and port. We only support *ONE* input connection which we initiate.here.
    c = (struct client *) malloc(sizeof(*c));
    while(1) {
        if ((fd = setupConnection(c)) == ANET_ERR) {
            fprintf(stderr, "Waiting on %s:%d\n", View1090.net_input_beast_ipaddr, modes.net_input_beast_port);     
            sleep(1);      
        } else {
            break;
        }
    }

}


void AircraftData::disconnect() {
    if (fd != ANET_ERR) 
      {close(fd);}
}


void AircraftData::update() {
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
}

AircraftData::AircraftData(){
    // Default everything to zero/NULL
    memset(&modes,    0, sizeof(Modes));
    memset(&View1090, 0, sizeof(View1090));

    // Now initialise things that should not be 0/NULL to their defaults
    modes.check_crc               = 1;
    strcpy(View1090.net_input_beast_ipaddr,VIEW1090_NET_OUTPUT_IP_ADDRESS); 
    modes.net_input_beast_port    = MODES_NET_OUTPUT_BEAST_PORT;
    modes.interactive_rows        = MODES_INTERACTIVE_ROWS;
    modes.interactive_delete_ttl  = MODES_INTERACTIVE_DELETE_TTL;
    modes.interactive_display_ttl = MODES_INTERACTIVE_DISPLAY_TTL;
    modes.fUserLat                = MODES_USER_LATITUDE_DFLT;
    modes.fUserLon                = MODES_USER_LONGITUDE_DFLT;

    modes.interactive             = 0;
    modes.quiet                   = 1;

    // Map options
    appData.maxDist                 = 25.0;
    appData.centerLon               = modes.fUserLon;
    appData.centerLat               = modes.fUserLat;

    // Display options
    appData.screen_uiscale          = 1;
    appData.screen_width            = 0;
    appData.screen_height           = 0;    
    appData.screen_depth            = 32;
    appData.fullscreen              = 0;

    // Initialize status
    Status.msgRate                = 0;
    Status.avgSig                 = 0;
    Status.numPlanes              = 0;
    Status.numVisiblePlanes     = 0;
    Status.maxDist                = 0;

    selectedAircraft =  NULL;
}

