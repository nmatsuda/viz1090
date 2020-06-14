#include "Input.h"

static std::chrono::high_resolution_clock::time_point now() {
    return std::chrono::high_resolution_clock::now();
}

// static uint64_t now() {
//     return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch).count();
// }

static uint64_t elapsed(std::chrono::high_resolution_clock::time_point ref) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(now() - ref).count();
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

				if(elapsed(touchDownTime) > 100) {
						//touchDownTime = 0;
				}
				break;

			case SDL_FINGERMOTION:;	
				if(elapsed(touchDownTime) > 150) {
					tapCount = 0;
					//touchDownTime = 0;
				}		
				view->moveCenterRelative( view->screen_width * event.tfinger.dx,  view->screen_height * event.tfinger.dy);
				break;

			case SDL_FINGERDOWN:
				if(elapsed(touchDownTime) > 500) {
					tapCount = 0;
				} 


				//this finger number is always 1 for down and 0 for up an rpi+hyperpixel??
				if(SDL_GetNumTouchFingers(event.tfinger.touchId) <= 1) {
					touchDownTime = now();	
				}
				break;

			case SDL_FINGERUP:
				if(elapsed(touchDownTime) < 150 && SDL_GetNumTouchFingers(event.tfinger.touchId) == 0) {
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
					if(elapsed(touchDownTime) > 500) {
						tapCount = 0;
					}
					touchDownTime = now();
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








