#include "dump1090.h"
#include "structs.h"
#include "SDL/SDL_rotozoom.h"

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

    SDL_FillRect(game.screen, NULL, 0);

    if (Modes.map) {
        drawMap();
        //drawList(3,320);            
    } else {
        drawList(10,0);
    }

	if(Modes.screen_upscale > 1) {
        SDL_Rect clip;
        SDL_Surface *temp = SDL_DisplayFormat(zoomSurface(game.screen, Modes.screen_upscale, Modes.screen_upscale, 0));

        clip.x = 0;
        clip.y = 0;
        clip.w = temp->w;
        clip.h = temp->h;

        SDL_BlitSurface(temp, &clip, game.bigScreen, 0);

        SDL_Flip(game.bigScreen);
    } else {
        SDL_Flip(game.screen);    
    }	
}