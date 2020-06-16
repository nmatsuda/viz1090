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
