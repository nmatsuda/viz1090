#include "structs.h"
#include "view1090.h"

void getInput()
{
	SDL_Event event;
		
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
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
						appData.mapLogDist = !appData.mapLogDist;
					break;

					case SDLK_m:
						appData.showList = !appData.showList;
					break;		

					default:
					break;
				}

			break;

			case SDL_MOUSEWHEEL:

				appData.maxDist *= 1.0 + event.wheel.y / 10.0;
				break;

			case SDL_MULTIGESTURE:
				appData.maxDist /=1.0 + 4.0*event.mgesture.dDist;
				break;

			case SDL_FINGERMOTION:;

				//
				// need to make lonlat to screen conversion class - this is just the inverse of the stuff in draw.c, without offsets
				//
					
	    		double scale_factor = (appData.screen_width > appData.screen_height) ? appData.screen_width : appData.screen_height;

	    		double dx = -1.0 * (0.75*(double)appData.screen_width / (double)appData.screen_height) * appData.screen_width * event.tfinger.dx * appData.maxDist / (0.95 * scale_factor * 0.5);
	    		double dy = 1.0 * appData.screen_height * event.tfinger.dy * appData.maxDist / (0.95 * scale_factor * 0.5);

	    		double outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

	    		double outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((appData.centerLat)/2.0f) * M_PI / 180.0f);


				appData.centerLon += outLon;
				appData.centerLat += outLat;
				break;
		}
	}
}
