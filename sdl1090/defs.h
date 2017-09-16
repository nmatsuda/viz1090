#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"

#ifdef RPI
	#include <wiringPi.h>
#endif

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
