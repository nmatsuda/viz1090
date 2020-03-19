#ifndef STRUCTS
#define STRUCTS

#include "defs.h"

// #include "AircraftData.h"


struct {
    double msgRate;
    double avgSig;
    int numPlanes;
    int numVisiblePlanes;
    double maxDist;
    struct aircraft *closeCall;
} Status;
typedef struct Style {
    SDL_Color backgroundColor;

    SDL_Color selectedColor;
    SDL_Color planeColor;
    SDL_Color planeGoneColor;

    SDL_Color mapInnerColor;
    SDL_Color mapOuterColor;
    SDL_Color scaleBarColor;

    SDL_Color buttonColor;
} Style;

// globals
//extern AppData appData;
extern Style style;

// functions
#ifdef __cplusplus
extern "C" {
#endif

//status.c
void updateStatus();
void drawStatus();

#ifdef __cplusplus
}
#endif

#endif