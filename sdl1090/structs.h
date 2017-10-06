#ifndef STRUCTS
#define STRUCTS

#include "defs.h"

typedef struct Game
{
	SDL_Surface *screen;	
	SDL_Surface *bigScreen;		

	TTF_Font *font;
	TTF_Font *listFont;	
	int fontWidth;
	int fontHeight;	
} Game;

Game game;

// functions

//font.c
TTF_Font *loadFont(char *, int);
void closeFont(TTF_Font *);
void drawString(char *, int, int, TTF_Font *, SDL_Color);
void drawStringBG(char *, int, int, TTF_Font *, SDL_Color, SDL_Color);

//init.c
void init(char *);
void cleanup(void);

//input.c
void getInput(void);

//mapdata.c
void initMaps();

//list.c
void drawList(int rows, int top);

//draw.c
void draw();

//status.c
void updateStatus();
void drawStatus();

#endif