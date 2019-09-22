#include "dump1090.h"
#include "structs.h"

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

    SDL_ShowCursor(SDL_DISABLE);

    Uint32 flags = 0;

    if(appData.fullscreen) {
    	flags = flags | SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    if(appData.screen_width == 0) {
	    SDL_DisplayMode DM;
		SDL_GetCurrentDisplayMode(0, &DM);
		appData.screen_width = DM.w;
		appData.screen_height= DM.h;
    }

    appData.window =  SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, appData.screen_width, appData.screen_height, flags);		
	appData.renderer = SDL_CreateRenderer(appData.window, -1, 0);
	appData.texture = SDL_CreateTexture(appData.renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               appData.screen_width, appData.screen_height);

	if(appData.fullscreen) {
		//\SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
		SDL_RenderSetLogicalSize(appData.renderer, appData.screen_width, appData.screen_height);
	}

    appData.mapFont = loadFont("font/TerminusTTF-4.46.0.ttf", 12 * appData.screen_uiscale);
    appData.mapBoldFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * appData.screen_uiscale);    
       
    appData.listFont = loadFont("font/TerminusTTF-4.46.0.ttf", 12 * appData.screen_uiscale);

    appData.messageFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * appData.screen_uiscale);
    appData.labelFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * appData.screen_uiscale);

    appData.mapFontWidth = 5 * appData.screen_uiscale;
    appData.mapFontHeight = 12 * appData.screen_uiscale; 

    appData.messageFontWidth = 6 * appData.screen_uiscale;
    appData.messageFontHeight = 12 * appData.screen_uiscale; 

    appData.labelFontWidth = 6 * appData.screen_uiscale;
    appData.labelFontHeight = 12 * appData.screen_uiscale; 

	initMaps();

}

void cleanup() {
	closeFont(appData.mapFont);
	closeFont(appData.mapBoldFont);
	closeFont(appData.messageFont);
	closeFont(appData.labelFont);
	closeFont(appData.listFont);

	TTF_Quit();

	SDL_Quit();
}
