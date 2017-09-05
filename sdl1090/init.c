#include "init.h"
#include "SDL/SDL_getenv.h"
#include <wiringPi.h>

extern void closeFont(TTF_Font *);


void init(char *title)
{

	wiringPiSetupGpio() ;

	pinMode(23, INPUT);
	pullUpDnControl (23, PUD_UP);
	pinMode(22, INPUT);
	pullUpDnControl (22, PUD_UP);	

	putenv((char*)"FRAMEBUFFER=/dev/fb1");
        putenv((char*)"SDL_FBDEV=/dev/fb1");
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

 	game.screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWPALETTE|SDL_DOUBLEBUF);
	
	if (game.screen == NULL)
	{
		printf("Couldn't set screen mode to %d x %d: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());

		exit(1);
	}
		
	/* Set the screen title */
	
	SDL_WM_SetCaption(title, NULL);
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
