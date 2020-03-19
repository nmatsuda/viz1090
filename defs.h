#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#define ROUND_RADIUS 3 //radius of text box corners

#define CENTEROFFSET .375 //vertical offset for middle of screen

#define TRAIL_LENGTH 120
#define TRAIL_TTL   240.0 
#define DISPLAY_ACTIVE   30 
#define TRAIL_TTL_STEP   2

#define	MIN_MAP_FEATURE 2

#define	FRAMETIME 33

#define PAD 5
