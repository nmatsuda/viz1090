#ifndef STRUCTS
#define STRUCTS

#include "defs.h"


// mirrors aircraft struct in dump1090, separating for refactoring 

typedef struct PlaneObj {   
    uint32_t        addr;           // ICAO address
    char            flight[16];     // Flight number
    unsigned char   signalLevel[8]; // Last 8 Signal Amplitudes
    double          messageRate;
    int             altitude;       // Altitude
    int             speed;          // Velocity
    int             track;          // Angle of flight
    int             vert_rate;      // Vertical rate.
    time_t          seen;           // Time at which the last packet was received
    time_t          seenLatLon;           // Time at which the last packet was received
    time_t          prev_seen;
    double          lat, lon;       // Coordinated obtained from CPR encoded data
    
    //history
    float           oldLon[TRAIL_LENGTH];
    float           oldLat[TRAIL_LENGTH];
    float           oldHeading[TRAIL_LENGTH];
    time_t          oldSeen[TRAIL_LENGTH];
    uint8_t         oldIdx; 
    uint64_t        created;
    uint64_t        msSeen;
    uint64_t        msSeenLatLon;
    int               live;

    struct PlaneObj *next;        // Next aircraft in our linked list

//// label stuff

    int             x, y, cx, cy, w, h;
    float           ox, oy, dox, doy, ddox, ddoy;
    float           pressure;
} PlaneObj;


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

    PlaneObj *planes;
    PlaneObj *selectedPlane;

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

//input.c
void getInput(void);

//mapdata.c
void initMaps();

//list.c
void drawList(int top);

//draw.c
void draw();
void latLonFromScreenCoords(float *lat, float *lon, int x, int y);
void moveCenterAbsolute(float x, float y);
void moveCenterRelative(float dx, float dy);
void registerClick();

//status.c
void updateStatus();
void drawStatus();

//planeObj.c
void updatePlanes();
#ifdef __cplusplus
}
#endif

#endif