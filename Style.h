#ifndef STYLE_H
#define STYLE_H

#include "SDL2/SDL.h"


//
// This should go to a full theming class
//
typedef struct Style {
    SDL_Color backgroundColor;

    SDL_Color selectedColor;
    SDL_Color planeColor;
    SDL_Color planeGoneColor;
    SDL_Color trailColor;

    SDL_Color geoColor;
    SDL_Color airportColor;

    SDL_Color labelColor;
    SDL_Color labelLineColor;    
	SDL_Color subLabelColor;   
    SDL_Color labelBackground;

    SDL_Color scaleBarColor;
    SDL_Color buttonColor;
    SDL_Color buttonBackground;
    SDL_Color buttonOutline;

    SDL_Color clickColor;

	SDL_Color black;
	SDL_Color white;
	SDL_Color red;
	SDL_Color green;
	SDL_Color blue;

    //
    // todo separate style stuff
    //
	
    Style() {

    	SDL_Color pink 		= {249,38,114,255};

		SDL_Color purple 	= {85, 0, 255,255};
		SDL_Color purple_dark 	= {33, 0, 122,255};

		SDL_Color blue 		= {102,217,239,255};
		SDL_Color blue_dark 		= {102,217,239,255};

		SDL_Color green 	= {0,255,234,255};
		SDL_Color green_dark 	= {24,100,110,255};

		SDL_Color yellow	= {216,255,0,255};
		SDL_Color yellow_dark	= {90,133,50,255};

		SDL_Color orange	= {253,151,31,255};
		SDL_Color grey_light	= {196,196,196,255};
		SDL_Color grey 		= {127,127,127,255};
		SDL_Color grey_dark 	= {64,64,64,255};

		black		= {0,0,0,255};
		white	    = {255,255,255,255};
		red			= {255,0,0,255};
		green		= {0,255,0,255};
		blue		= {0,0,255,255};


	    backgroundColor = black;

	    selectedColor = pink;
	    planeColor = yellow;
	    planeGoneColor = grey;
	    trailColor = yellow_dark;

	    geoColor = purple_dark;
	    airportColor = purple;

	    labelColor = white;
	    labelLineColor = grey_dark;
	    subLabelColor = grey;
	   	labelBackground = black;
	    scaleBarColor = grey_light;
	    buttonColor = grey_light;
   		buttonBackground = black;
    	buttonOutline = grey_light;	    

    	clickColor = grey;
    }
} Style;

#endif
