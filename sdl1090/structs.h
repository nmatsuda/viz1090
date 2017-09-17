#include "defs.h"

typedef struct Game
{
	SDL_Surface *screen;	
	TTF_Font *font;
} Game;

// functions

//font.c
TTF_Font *loadFont(char *, int);
void closeFont(TTF_Font *);
void drawString(char *, int, int, TTF_Font *, SDL_Color);

//init.c
void init(char *);
void cleanup(void);

//input.c
void getInput(void);

//draw.c
void drawGeography();
void drawPlaneHeading(double , double , double, int, char *);
void drawPlane(double , double, int);
void drawTrail(double *, double *, time_t *, int);
void drawGrid();

//mapdata.c
void initMaps();