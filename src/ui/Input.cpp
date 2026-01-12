// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// Copyright (C) 2014, Malcolm Robb <Support@ATTAvionics.com>
// Copyright (C) 2012, Salvatore Sanfilippo <antirez at gmail dot com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "ui/Input.h"
#include "viz1090/Profiler.h"

static std::chrono::high_resolution_clock::time_point
now() {
  return std::chrono::high_resolution_clock::now();
}

// static uint64_t now() {
//     return
//     std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch).count();
// }

static uint64_t
elapsed(std::chrono::high_resolution_clock::time_point ref) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(now() - ref).count();
}

template <typename T>
int
sgn(T val) {
  return (T(0) < val) - (val < T(0));
}

void
Input::getInput() {
  PROFILE_SCOPE("Input::getInput");
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        exit(0);
        break;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            exit(0);
            break;

          case SDLK_MINUS:
            view->getMapView().maxDist *= 1.0 + 0.5 * sgn(1);
            if (view->getMapView().maxDist < 0.001f) {
              view->getMapView().maxDist = 0.001f;
            }

            view->getMapView().mapTargetMaxDist = 0;
            view->getMapView().setMoved();
            break;

          case SDLK_EQUALS:
            view->getMapView().maxDist *= 1.0 + 0.5 * sgn(-1);
            if (view->getMapView().maxDist < 0.001f) {
              view->getMapView().maxDist = 0.001f;
            }

            view->getMapView().mapTargetMaxDist = 0;
            view->getMapView().setMoved();
            break;

          // toggle origin marker
          case SDLK_o:
            view->getMapView().setDrawCenterOrigin(!view->getMapView().getDrawCenterOrigin());
            break;

          default:
            break;
        }

        break;

      case SDL_MOUSEWHEEL:
        view->getMapView().maxDist *= 1.0 + 0.5 * sgn(event.wheel.y);
        if (view->getMapView().maxDist < 0.001f) {
          view->getMapView().maxDist = 0.001f;
        }

        view->getMapView().mapTargetMaxDist = 0;
        view->getMapView().setMoved();
        break;

      case SDL_MULTIGESTURE:
        view->getMapView().maxDist /= 1.0 + 4.0 * event.mgesture.dDist;
        view->getMapView().mapTargetMaxDist = 0;
        view->getMapView().setMoved();

        if (elapsed(touchDownTime) > 100) {
          // touchDownTime = 0;
        }
        break;

      case SDL_FINGERMOTION: {
        if (elapsed(touchDownTime) > 150) {
          tapCount = 0;
          // touchDownTime = 0;
        }
        float dx = event.tfinger.dx;
        float dy = event.tfinger.dy;
        if (flipTouch) {
          dx = -dx;
          dy = -dy;
        }
        view->moveCenterRelative(view->screen_width * dx,
                                 view->screen_height * dy);
        break;
      }

      case SDL_FINGERDOWN:
        if (elapsed(touchDownTime) > 500) {
          tapCount = 0;
        }

        // this finger number is always 1 for down and 0 for up an rpi+hyperpixel??
        if (SDL_GetNumTouchFingers(event.tfinger.touchId) <= 1) {
          touchDownTime = now();
        }
        break;

      case SDL_FINGERUP: {
        if (elapsed(touchDownTime) < 150 && SDL_GetNumTouchFingers(event.tfinger.touchId) == 0) {
          float fx = event.tfinger.x;
          float fy = event.tfinger.y;
          if (flipTouch) {
            fx = 1.0f - fx;
            fy = 1.0f - fy;
          }
          touchx = view->screen_width * fx;
          touchy = view->screen_height * fy;
          tapCount++;
          view->registerClick(tapCount, touchx, touchy);
        } else {
          touchx = 0;
          touchy = 0;
          tapCount = 0;
        }

        break;
      }

      case SDL_MOUSEBUTTONDOWN:
        if (event.button.which != SDL_TOUCH_MOUSEID &&
            event.button.button == SDL_BUTTON_LEFT) {
          if (elapsed(touchDownTime) > 500) {
            tapCount = 0;
          }
          touchDownTime = now();
          mouseDragging_ = false;
          mouseDownX_ = event.button.x;
          mouseDownY_ = event.button.y;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (event.button.which != SDL_TOUCH_MOUSEID &&
            event.button.button == SDL_BUTTON_LEFT) {
          // Only register click if we weren't dragging
          if (!mouseDragging_) {
            touchx = event.button.x;
            touchy = event.button.y;
            // event.button.clicks contains the click count (1 for single, 2 for double, etc.)
            // Ensure we have at least 1 for a valid click
            tapCount = (event.button.clicks > 0) ? event.button.clicks : 1;
            view->registerClick(tapCount, touchx, touchy);
          } else {
            // Reset state after drag ends
            tapCount = 0;
          }
          mouseDragging_ = false;
        }
        break;

      case SDL_MOUSEMOTION:
        if (event.motion.which != SDL_TOUCH_MOUSEID) {
          view->registerMouseMove(event.motion.x, event.motion.y);

          if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            // Check if we've moved enough to be considered a drag
            if (!mouseDragging_) {
              int dx = event.motion.x - mouseDownX_;
              int dy = event.motion.y - mouseDownY_;
              if (dx * dx + dy * dy > DRAG_THRESHOLD * DRAG_THRESHOLD) {
                mouseDragging_ = true;
              }
            }
            view->moveCenterRelative(event.motion.xrel, event.motion.yrel);
          }
        }
        break;
    }
  }
}

Input::Input(AppData* appData, viz1090::View* view) {
  this->view = view;
  this->appData = appData;
}
