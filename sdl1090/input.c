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

			case SDL_MULTIGESTURE:
				Modes.maxDist /=1.0 + 4.0*event.mgesture.dDist;
				break;

			case SDL_FINGERMOTION:;

				//
				// need to make lonlat to screen conversion class - this is just the inverse of the stuff in draw.c, without offsets
				//

	    		double scale_factor = (Modes.screen_width > Modes.screen_height) ? Modes.screen_width : Modes.screen_height;

	    		double dx = -1.0 * (0.75*(double)Modes.screen_width / (double)Modes.screen_height) * Modes.screen_width * event.tfinger.dx * Modes.maxDist / (0.95 * scale_factor * 0.5);
	    		double dy = -1.0 * Modes.screen_height * event.tfinger.dy * Modes.maxDist / (0.95 * scale_factor * 0.5);

	    		double outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

	    		double outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((Modes.fUserLat)/2.0f) * M_PI / 180.0f);


				Modes.fUserLon += outLon;
				Modes.fUserLat += outLat;
				break;
		}
	}
}
