#include "input.h"
#include <wiringPi.h>

void getInput()
{

	if(!digitalRead(23) || !digitalRead(22)) {
		exit(0);
	} 	
	
	SDL_Event event;
	
	/* Loop through waiting messages and process them */
	
	while (SDL_PollEvent(&event))
	{
		fprintf(stderr,"key: %d\n", event.key.keysym.sym);

		switch (event.type)
		{
			/* Closing the Window or pressing Escape will exit the program */
			
			case SDL_QUIT:
				exit(0);
			break;
			
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						exit(0);
					break;
					
					default:
					break;
				}
			break;
		}
	}
}
