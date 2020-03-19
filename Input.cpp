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

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
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
				view->maxDist *= 1.0 + 0.5 * sgn(event.wheel.y);
				if(view->maxDist < 0.001f) {
					view->maxDist = 0.001f;
				}

				view->mapTargetMaxDist = 0;
				view->mapMoved = 1;
				break;

			case SDL_MULTIGESTURE:
				view->maxDist /=1.0 + 4.0*event.mgesture.dDist;
				view->mapTargetMaxDist = 0;
				view->mapMoved = 1;

				if(mstime() - touchDownTime > 100) {
						touchDownTime = 0;
				}
				break;

			case SDL_FINGERMOTION:;					
				touchDownTime = 0;
				view->moveCenterRelative( view->screen_width * event.tfinger.dx,  view->screen_height * event.tfinger.dy);
				break;

			case SDL_FINGERDOWN:
				if(mstime() - touchDownTime > 500) {
					tapCount = 0;
				}
				touchDownTime = mstime();
				break;

			case SDL_FINGERUP:
				if(mstime() - touchDownTime < 120) {
					touchx = view->screen_width * event.tfinger.x;
					touchy = view->screen_height * event.tfinger.y;
					tapCount++;
					view->registerClick(tapCount, touchx, touchy);
				} else {
					touchx = 0;
					touchy = 0;
					tapCount = 0;
				}

				break;

			case SDL_MOUSEBUTTONDOWN:
				if(event.button.which != SDL_TOUCH_MOUSEID) {
					if(mstime() - touchDownTime > 500) {
						tapCount = 0;
					}
					touchDownTime = mstime();
				}
				break;

			case SDL_MOUSEBUTTONUP:;
				if(event.button.which != SDL_TOUCH_MOUSEID) {
					touchx = event.motion.x;
					touchy = event.motion.y;
					tapCount = event.button.clicks;

					view->registerClick(tapCount, touchx, touchy);
				}
				break;

			case SDL_MOUSEMOTION:;

				if(event.motion.which != SDL_TOUCH_MOUSEID) {
					view->registerMouseMove(event.motion.x, event.motion.y);
					
					if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
						view->moveCenterRelative(event.motion.xrel, event.motion.yrel);
					}					
				}
				break;				
		}
	}
}

Input::Input(AppData *appData, View *view) {
	this->view = view;
	this->appData = appData;
}








