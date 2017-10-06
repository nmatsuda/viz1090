#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#include "mapdata.h"

#ifdef RPI
	#include <wiringPi.h>
#endif

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

#define UPSCALE 3

#define LOGMAXDIST 1000.0
#define MAXDIST 50.0

#define AA 0

#define MAGMA 0