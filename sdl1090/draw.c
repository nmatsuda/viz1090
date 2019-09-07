#include "dump1090.h"
#include "structs.h"
#include "SDL2/SDL2_rotozoom.h"

static uint64_t mstime(void) {
    struct timeval tv;
    uint64_t mst;

    gettimeofday(&tv, NULL);
    mst = ((uint64_t)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}

void draw() {

    if ((mstime() - Modes.interactive_last_update) < MODES_INTERACTIVE_REFRESH_TIME) {
    	return;
    }

    Modes.interactive_last_update = mstime();

    updateStatus();

    SDL_SetRenderDrawColor( game.renderer, 0, 0, 0, 0);

    SDL_RenderClear(game.renderer);

    if (Modes.map) {
        drawStatus();
        // SDL_RenderCopy(game.renderer,game.texture,NULL,NULL);          
        drawMap();
    } else {
        drawList(10,0);
    }

    SDL_RenderPresent(game.renderer);	
}