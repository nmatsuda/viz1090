#include "dump1090.h"
#include "init.h"

void init(char *title) {
	// raspberry pi compiler flag enables these options
	#ifdef RPI
		putenv((char*)"FRAMEBUFFER=/dev/fb1");
	    putenv((char*)"SDL_FBDEV=/dev/fb1");
    #endif

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Could not initialize SDL: %s\n", SDL_GetError());		
		exit(1);
	}
	
	
	if (TTF_Init() < 0) {
		printf("Couldn't initialize SDL TTF: %s\n", SDL_GetError());
		exit(1);
	}

	// #ifdef RPI
	//  	const SDL_VideoInfo* vInfo = SDL_GetVideoInfo();

 //        if (!vInfo) {
 //                fprintf(stderr,"ERROR in SDL_GetVideoInfo(): %s\n",SDL_GetError());
 //                exit(1);
 //        }

 //        Modes.screen_width = vInfo->current_w;
 //        Modes.screen_height = vInfo->current_h;
 //        Modes.screen_depth = vInfo->vfmt->BitsPerPixel;

 //        Modes.screen_upscale = 1;
 //    #endifX
    SDL_ShowCursor(SDL_DISABLE);

    Uint32 flags = 0;

    if(Modes.fullscreen) {
    	flags = flags | SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    game.window =  SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Modes.screen_width, Modes.screen_height, flags);		
	game.renderer = SDL_CreateRenderer(game.window, -1, 0);
	game.texture = SDL_CreateTexture(game.renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               Modes.screen_width, Modes.screen_height);

	if(Modes.fullscreen) {
		//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
		SDL_RenderSetLogicalSize(game.renderer, Modes.screen_width, Modes.screen_height);
	}

    game.mapFont = loadFont("font/TerminusTTF-4.46.0.ttf", 12 * Modes.screen_uiscale);
    game.mapBoldFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * Modes.screen_uiscale);    
       
    game.listFont = loadFont("font/TerminusTTF-4.46.0.ttf", 12 * Modes.screen_uiscale);

    game.messageFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * Modes.screen_uiscale);
    game.labelFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * Modes.screen_uiscale);

    game.mapFontWidth = 5 * Modes.screen_uiscale;
    game.mapFontHeight = 12 * Modes.screen_uiscale; 

    game.messageFontWidth = 6 * Modes.screen_uiscale;
    game.messageFontHeight = 12 * Modes.screen_uiscale; 

    game.labelFontWidth = 6 * Modes.screen_uiscale;
    game.labelFontHeight = 12 * Modes.screen_uiscale; 

	initMaps();

}

void cleanup() {
	closeFont(game.mapFont);
	closeFont(game.mapBoldFont);
	closeFont(game.messageFont);
	closeFont(game.labelFont);
	closeFont(game.listFont);

	TTF_Quit();

	SDL_Quit();
}
