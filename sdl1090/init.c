#include "init.h"

extern void closeFont(TTF_Font *);


void init(char *title)
{
	/* Initialise SDL */
	
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) < 0)
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
	

    if (SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &(game.window), &(game.renderer)) < 0) 
    {
		printf("Couldn't initialize Renderer: %s\n", SDL_GetError());

		exit(1);
	}
		
	/* Set the screen title */
	
	SDL_SetWindowTitle(game.window,title);
}

void cleanup()
{
	/* Close the font */
	
	closeFont(game.font);
	
	/* Close SDL_TTF */
	
	TTF_Quit();
	

    SDL_DestroyWindow(game.window);


	/* Shut down SDL */
	
	SDL_Quit();
}
