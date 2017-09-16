
#include "dump1090.h"
#include "structs.h"

Game game;

extern void drawPlaneHeading(double , double , double, int, char *);
extern void drawPlane(double , double, int);
extern void drawTrail(double *, double *, time_t *, int);
extern void drawGrid();

//
// ============================= Utility functions ==========================
//
static uint64_t mstime(void) {
    struct timeval tv;
    uint64_t mst;

    gettimeofday(&tv, NULL);
    mst = ((uint64_t)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}


void drawMap(void) {
    struct aircraft *a = Modes.aircrafts;
    time_t now = time(NULL);

    // Refresh screen every (MODES_INTERACTIVE_REFRESH_TIME) miliseconde
    if ((mstime() - Modes.interactive_last_update) < MODES_INTERACTIVE_REFRESH_TIME)
       {return;}

    Modes.interactive_last_update = mstime();

    SDL_FillRect(game.screen, NULL, 0);

    drawGrid();

    while(a) {
        if ((now - a->seen) < Modes.interactive_display_ttl) {
            if (a->bFlags & MODES_ACFLAGS_LATLON_VALID) {

                unsigned char * pSig       = a->signalLevel;
                unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
                                              pSig[4] + pSig[5] + pSig[6] + pSig[7] + 3) >> 3; 

                drawTrail(a->oldDx, a->oldDy, a->oldSeen, a->oldIdx);

                int colorIdx;
                if((int)(now - a->seen) > MODES_INTERACTIVE_DISPLAY_ACTIVE) {
                    colorIdx = -1;
                } else {
                    colorIdx = signalAverage;
                }

                if(MODES_ACFLAGS_HEADING_VALID) {
                    drawPlaneHeading(a->dx, a->dy,a->track, colorIdx, a->flight);
                } else {
                    drawPlane(a->dx, a->dy, colorIdx);
                }                
            }
        }
        a = a->next;
    }

    SDL_Flip(game.screen);
}
