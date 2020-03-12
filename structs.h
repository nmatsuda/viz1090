#ifndef STRUCTS
#define STRUCTS

#include "defs.h"

#include "AircraftData.h"


typedef struct AppData
{
	SDL_Window		*window;
	SDL_Renderer	*renderer;
	SDL_Texture 	*mapTexture;

	TTF_Font		*mapFont;
	TTF_Font		*mapBoldFont;	
	TTF_Font		*listFont;	

	TTF_Font		*messageFont;	
	TTF_Font		*labelFont;		

	int mapFontWidth;
	int mapFontHeight;
	int labelFontWidth;
	int labelFontHeight;	
	int messageFontWidth;
	int messageFontHeight;		

	// map options
    float maxDist;

    //display options
    int screen_upscale;
    int screen_uiscale;
    int screen_width;
    int screen_height;
    int screen_depth;
    int fullscreen;
    int screen_index;

    float centerLon;
    float centerLat;

    uint64_t touchDownTime;
    int touchx;
    int touchy;
    int tapCount;
    int isDragging;

    uint64_t mouseMovedTime;
    int mousex;
    int mousey;

    float mapTargetMaxDist;
    float mapTargetLat;
    float mapTargetLon;

    int mapMoved;

    QuadTree root;

    //PlaneObj *planes;
    //PlaneObj *selectedPlane;

    uint64_t lastFrameTime;
} AppData;


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
extern AppData appData;
extern Style style;

// functions
#ifdef __cplusplus
extern "C" {
#endif

//font.c
TTF_Font *loadFont(char *, int);
void closeFont(TTF_Font *);
void drawString(char *, int, int, TTF_Font *, SDL_Color);
void drawString90(char *, int, int, TTF_Font *, SDL_Color);
void drawStringBG(char *, int, int, TTF_Font *, SDL_Color, SDL_Color);

//init.c
void init(char *);
void cleanup(void);

//mapdata.c
void initMaps();

//list.c
void drawList(int top);


//status.c
void updateStatus();
void drawStatus();

#ifdef __cplusplus
}
#endif

#endif