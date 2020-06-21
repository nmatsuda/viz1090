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

#ifndef VIEW_H
#define VIEW_H

#include "AppData.h"
#include "Map.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h" 
#include <chrono>
#include <string>

//defs - should all move to config file setup
#define ROUND_RADIUS 3 //radius of text box corners

#define CENTEROFFSET .5 //vertical offset for middle of screen

#define TRAIL_LENGTH 120
#define TRAIL_TTL   240.0 
#define DISPLAY_ACTIVE   30 
#define TRAIL_TTL_STEP   2

#define MIN_MAP_FEATURE 2

#define FRAMETIME 33

#define PAD 5

#define LATLONMULT 111.195 // 6371.0 * M_PI / 180.0




//
// This should go to a full theming class
//
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



class View {

	private:
		AppData *appData;

		//for cursor drawing
	    std::chrono::high_resolution_clock::time_point mouseMovedTime;
	    bool mouseMoved;
	    int mousex;
	    int mousey;

	    std::chrono::high_resolution_clock::time_point clickTime;
	    bool clicked;
	    int clickx;
	    int clicky;

	    int lineCount;

	    float dx_mult;
	    float dy_mult;

	    TTF_Font* loadFont(const char *name, int size);
	    void closeFont(TTF_Font *font);
		void drawString(std::string text, int x, int y, TTF_Font *font, SDL_Color color);
		void drawStringBG(std::string text, int x, int y, TTF_Font *font, SDL_Color color, SDL_Color bgColor);
		void drawStatusBox(int *left, int *top, std::string label, std::string message, SDL_Color color);
		void drawStatus();

		Aircraft *selectedAircraft;

		Style style;

	public:
		int screenDist(float d);
		void pxFromLonLat(float *dx, float *dy, float lon, float lat);
		void latLonFromScreenCoords(float *lat, float *lon, int x, int y);
		void screenCoords(int *outX, int *outY, float dx, float dy);
		int outOfBounds(int x, int y);
		int outOfBounds(int x, int y, int left, int top, int right, int bottom);
		void drawPlaneOffMap(int x, int y, int *returnx, int *returny, SDL_Color planeColor);
		void drawPlaneIcon(int x, int y, float heading, SDL_Color planeColor);
		void drawTrails(int left, int top, int right, int bottom);
		void drawScaleBars();
		void drawLinesRecursive(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max, SDL_Color color);
		void drawLines(int left, int top, int right, int bottom, int bailTime);
		void drawPlaceNames();		
		void drawGeography();
		void drawSignalMarks(Aircraft *p, int x, int y);
		void drawPlaneText(Aircraft *p);
		void drawSelectedAircraftText(Aircraft *p);
		void resolveLabelConflicts();
		void drawPlanes();
		void animateCenterAbsolute(float x, float y);
		void moveCenterAbsolute(float x, float y);
		void moveCenterRelative(float dx, float dy);
		void zoomMapToTarget();
		void moveMapToTarget();
		void drawMouse();
		void drawClick();
		void registerClick(int tapcount, int x, int y);
		void registerMouseMove(int x, int y);
		void draw();
		
		void SDL_init();
		void font_init();

		View(AppData *appData);
		~View();


	////////////////
		bool metric;

	    float maxDist;
	    float currentMaxDist;

	    float centerLon;
	    float centerLat;
	   
	    float mapTargetMaxDist;
	    float mapTargetLat;
	    float mapTargetLon;

	    int mapMoved;
	    int mapRedraw;
	    int mapAnimating;

	    float currentLon;
	    float currentLat;
	    std::chrono::high_resolution_clock::time_point lastFrameTime;
		std::chrono::high_resolution_clock::time_point drawStartTime;
		std::chrono::high_resolution_clock::time_point lastRedraw;

	    Map map;

	    int screen_upscale;
	    int screen_uiscale;
	    int screen_width;
	    int screen_height;
	    int screen_depth;
	    int fullscreen;
	    int screen_index;

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
};

#endif
