#include "structs.h"
#include "view1090.h"

static uint64_t mstime(void) {
    struct timeval tv;
    uint64_t mst;

    gettimeofday(&tv, NULL);
    mst = ((uint64_t)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}

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

					default:
					break;
				}

			break;

			case SDL_MOUSEWHEEL:

				appData.maxDist *= 1.0 + event.wheel.y / 10.0;
				appData.mapMoved = 1;
				break;

			case SDL_MULTIGESTURE:
				appData.maxDist /=1.0 + 4.0*event.mgesture.dDist;
				appData.mapMoved = 1;
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

				//double outLon, outLat;
				//latLonFromScreenCoords(&outLat, &outLon, event.tfinger.dx, event.tfinger.dy);

				appData.centerLon += outLon;
				appData.centerLat += outLat;

				appData.mapMoved = 1;
				break;


			case SDL_FINGERDOWN:
				appData.touchDownTime = mstime();
				break;

			case SDL_FINGERUP:
				if(mstime() - appData.touchDownTime < 30) {
					appData.touchx = appData.screen_width * event.tfinger.x;
					appData.touchy = appData.screen_height * event.tfinger.y;
					selectedPlane = NULL;

				} else {
					appData.touchx = 0;
					appData.touchy = 0;
				}
				break;
		}
	}
}
