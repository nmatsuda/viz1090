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

    updateStatus();

    SDL_FillRect(game.screen, NULL, 0);

    if (Modes.map) {
        drawMap();
        drawStatus();            
    } else {
        drawList(10,0);
    }

	if(Modes.screen_upscale > 1) {
        SDL_Surface *temp = zoomSurface(game.screen, Modes.screen_upscale, Modes.screen_upscale, 0);
        SDL_Surface *temp2 =  SDL_DisplayFormat(temp);
	    SDL_Rect clip;
        clip.x = 0;
        clip.y = 0;
        clip.w = temp2->w;
        clip.h = temp2->h;

        SDL_BlitSurface(temp2, 0, game.bigScreen, 0);

        SDL_Flip(game.bigScreen);

        SDL_FreeSurface(temp);
        SDL_FreeSurface(temp2);
    } else {
        SDL_Flip(game.screen);    
    }	
}