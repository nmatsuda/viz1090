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
				appData.mapTargetMaxDist = 0;
				appData.mapMoved = 1;
				break;

			case SDL_MULTIGESTURE:
				appData.maxDist /=1.0 + 4.0*event.mgesture.dDist;
				appData.mapTargetMaxDist = 0;
				//moveCenterRelative((appData.screen_width/2) * event.mgesture.x, (appData.screen_height/2) * event.mgesture.y);
				//moveCenterRelative(event.mgesture.x, event.mgesture.y);

				appData.mapMoved = 1;
				break;

			// case SDL_FINGERMOTION:;
			// 	moveCenterRelative(appData.screen_width * event.tfinger.dx, appData.screen_height * event.tfinger.dy);
			// 	break;

			case SDL_FINGERDOWN:
				if(mstime() - appData.touchDownTime > 500) {
					appData.tapCount = 0;
				}
				appData.touchDownTime = mstime();
				break;

			// case SDL_FINGERUP:
			// 	if(mstime() - appData.touchDownTime < 30) {
			// 		appData.touchx = appData.screen_width * event.tfinger.x;
			// 		appData.touchy = appData.screen_height * event.tfinger.y;
			// 		selectedPlane = NULL;
			// 		//appData.tapCount++;

			// 		registerClick();
			// 	} else {
			// 		appData.touchx = 0;
			// 		appData.touchy = 0;
			// 		appData.tapCount = 0;
			// 	}
			// 	break;

			// case SDL_MOUSEBUTTONDOWN:;
			// 	appData.tapCount = 0;
			// 	break;

			case SDL_MOUSEBUTTONUP:;
				appData.touchx = event.motion.x;
				appData.touchy = event.motion.y;
				appData.tapCount = event.button.clicks;

				registerClick();
				break;

			case SDL_MOUSEMOTION:;
				appData.mouseMovedTime = mstime();
				appData.mousex = event.motion.x;
				appData.mousey = event.motion.y;
				
				if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
					moveCenterRelative(event.motion.xrel, event.motion.yrel);
				}
				break;				
		}
	}
}
