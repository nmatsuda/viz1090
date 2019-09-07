#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "mapdata.h"

#ifdef RPI
	#include <wiringPi.h>
#endif

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

#define UPSCALE 1
#define UISCALE 1

#define AA 0

#define MAGMA 0