#include "structs.h"
#include "view1090.h"

#include "Input.h"

static uint64_t mstime(void) {
    struct timeval tv;
    uint64_t mst;

    gettimeofday(&tv, NULL);
    mst = ((uint64_t)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}

void Input::getInput()
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
				appData.mapMoved = 1;
				break;

			case SDL_FINGERMOTION:;					
				appData.isDragging = 1;
				view->moveCenterRelative( appData.screen_width * event.tfinger.dx,  appData.screen_height * event.tfinger.dy);
				break;

			case SDL_FINGERDOWN:
				if(mstime() - appData.touchDownTime > 500) {
					appData.tapCount = 0;
				}
				appData.touchDownTime = mstime();
				break;

			case SDL_FINGERUP:
				if(mstime() - appData.touchDownTime < 120) {
					appData.touchx = appData.screen_width * event.tfinger.x;
					appData.touchy = appData.screen_height * event.tfinger.y;
					appData.tapCount++;
					appData.isDragging = 0;

					view->registerClick();
				} else {
					appData.touchx = 0;
					appData.touchy = 0;
					appData.tapCount = 0;
				}

				break;

			case SDL_MOUSEBUTTONDOWN:
				if(event.button.which != SDL_TOUCH_MOUSEID) {
					if(mstime() - appData.touchDownTime > 500) {
						appData.tapCount = 0;
					}
					appData.touchDownTime = mstime();
				}
				break;

			case SDL_MOUSEBUTTONUP:;
				if(event.button.which != SDL_TOUCH_MOUSEID) {
					appData.touchx = event.motion.x;
					appData.touchy = event.motion.y;
					appData.tapCount = event.button.clicks;
					appData.isDragging = 0;

					view->registerClick();
				}
				break;

			case SDL_MOUSEMOTION:;

				if(event.motion.which != SDL_TOUCH_MOUSEID) {
					appData.mouseMovedTime = mstime();
					appData.mousex = event.motion.x;
					appData.mousey = event.motion.y;
					
					if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
						appData.isDragging = 1;
						view->moveCenterRelative(event.motion.xrel, event.motion.yrel);
					}					
				}
				break;				
		}
	}
}

Input::Input(View *view) {
	this->view = view;
}
