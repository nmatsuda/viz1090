#ifndef STRUCTS
#define STRUCTS

#include "defs.h"

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
    int   showList;
    int   mapLogDist;
    float maxDist;

    //display options
    int screen_upscale;
    int screen_uiscale;
    int screen_width;
    int screen_height;
    int screen_depth;
    int fullscreen;

    double centerLon;
    double centerLat;

    uint64_t touchDownTime;
    int touchx;
    int touchy;

    int mapMoved;
    QuadTree *mapContinue;

    uint64_t lastFrameTime;
} AppData;

AppData appData;

// mirrors aircraft struct in dump1090, separating for refactoring 

struct planeObj {	
    uint32_t      	addr;           // ICAO address
    char          	flight[16];     // Flight number
    unsigned char 	signalLevel[8]; // Last 8 Signal Amplitudes
    double        	messageRate;
    int           	altitude;       // Altitude
    int           	speed;          // Velocity
    int           	track;          // Angle of flight
    int           	vert_rate;      // Vertical rate.
    time_t        	seen;           // Time at which the last packet was received
    time_t        	seenLatLon;           // Time at which the last packet was received
    time_t			prev_seen;
    double        	lat, lon;       // Coordinated obtained from CPR encoded data
    
	//history
    double        	oldLon[TRAIL_LENGTH];
    double			oldLat[TRAIL_LENGTH];
    double			oldHeading[TRAIL_LENGTH];
    time_t        	oldSeen[TRAIL_LENGTH];
    uint8_t         oldIdx; 
    uint64_t      	created;
    uint64_t		msSeen;
    uint64_t		msSeenLatLon;
    int			live;

    struct planeObj *next;        // Next aircraft in our linked list

//// label stuff

    int 			x, y, cx, cy, w, h;
    float			ox, oy, dox, doy, ddox, ddoy;
    float			pressure;
};

struct planeObj *planes;


struct planeObj *selectedPlane;

struct {
    double msgRate;
    double avgSig;
    int numPlanes;
    int numVisiblePlanes;
    double maxDist;
    struct aircraft *closeCall;
} Status;

// functions

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
void latLonFromScreenCoords(double *lat, double *lon, int x, int y);


//status.c
void updateStatus();
void drawStatus();

//planeObj.c
void updatePlanes();


#endif