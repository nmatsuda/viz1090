#include "input.h"
#include "view1090.h"

void getInput()
{

	#ifdef RPI
		if(!digitalRead(27)) {
			exit(0);
		} 

		if(!digitalRead(22)) {
			Modes.mapLogDist = !Modes.mapLogDist;
		} 	

		if(!digitalRead(23)) {
			Modes.map = !Modes.map;
		} 		
	#endif

	SDL_Event event;
	
	/* Loop through waiting messages and process them */
	
	while (SDL_PollEvent(&event))
	{
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

					case SDLK_l:
						Modes.mapLogDist = !Modes.mapLogDist;
					break;

					case SDLK_m:
						Modes.map = !Modes.map;
					break;		

					default:
					break;
				}

			break;

			case SDL_MOUSEWHEEL:

				Modes.maxDist *= 1.0 + event.wheel.y / 10.0;
				break;
		}
	}
}
