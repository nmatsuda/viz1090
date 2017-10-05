#include "init.h"
#include "SDL/SDL_getenv.h"

void init(char *title)
{

	#ifdef RPI
		wiringPiSetupGpio() ;

		pinMode(23, INPUT);
		pullUpDnControl (23, PUD_UP);
		pinMode(22, INPUT);
		pullUpDnControl (22, PUD_UP);	
		pinMode(27, INPUT);
		pullUpDnControl (27, PUD_UP);	

		putenv((char*)"FRAMEBUFFER=/dev/fb1");
	    putenv((char*)"SDL_FBDEV=/dev/fb1");
    #endif

	/* Initialise SDL */
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Could not initialize SDL: %s\n", SDL_GetError());
		
		exit(1);
	}
	
	/* Initialise SDL_TTF */
	
	if (TTF_Init() < 0)
	{
		printf("Couldn't initialize SDL TTF: %s\n", SDL_GetError());

		exit(1);
	}

	SDL_ShowCursor(SDL_DISABLE);

	#ifndef RPI
	 	game.bigScreen = SDL_SetVideoMode(SCREEN_WIDTH * UPSCALE, SCREEN_HEIGHT * UPSCALE, 32, SDL_HWPALETTE|SDL_DOUBLEBUF);	
	 	game.screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
 	#else
 	 	game.screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWPALETTE|SDL_DOUBLEBUF);
 	#endif



	
	if (game.screen == NULL)
	{
		printf("Couldn't set screen mode to %d x %d: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());

		exit(1);
	}

    /* Load the font */
    
    game.font = loadFont("TerminusTTF-Bold-4.46.0.ttf", 12);
       
    game.listFont = loadFont("TerminusTTF-Bold-4.46.0.ttf", 18);

	/* Set the screen title */
	
	SDL_WM_SetCaption(title, NULL);

	initMaps();
}

void cleanup()
{
	/* Close the font */
	
	closeFont(game.font);
	
	/* Close SDL_TTF */
	
	TTF_Quit();

	
	/* Shut down SDL */
	
	SDL_Quit();
}
