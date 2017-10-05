#include "dump1090.h"
#include "init.h"
#include "SDL/SDL_getenv.h"

void mouseSetup() {
	#ifdef RPI
		wiringPiSetupGpio() ;
		pinMode(23, INPUT);
		pullUpDnControl (23, PUD_UP);
		pinMode(22, INPUT);
		pullUpDnControl (22, PUD_UP);	
		pinMode(27, INPUT);
		pullUpDnControl (27, PUD_UP);	

    #endif
}

void init(char *title)
{

	// raspberry pi compiler flag enables these options
	#ifdef RPI
		putenv((char*)"FRAMEBUFFER=/dev/fb1");
	    putenv((char*)"SDL_FBDEV=/dev/fb1");
    #endif

	mouseSetup();

	/* Initialise SDL */
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Could not initialize SDL: %s\n", SDL_GetError());
		
		exit(1);
	}
	
	/* Initialise SDL_TTF */
	
	if (TTF_Init() < 0)
	{
		printf("Couldn't initialize SDL TTF: %s\n", SDL_GetError());

		exit(1);
	}

	SDL_ShowCursor(SDL_DISABLE);

	#ifdef RPI
	 	const SDL_VideoInfo* vInfo = SDL_GetVideoInfo();

        if (!vInfo) {
                fprintf(stderr,"ERROR in SDL_GetVideoInfo(): %s\n",SDL_GetError());
                exit(1);
        }

        Modes.screen_width = vInfo->current_w;
        Modes.screen_height = vInfo->current_h;
        Modes.screen_depth = vInfo->vfmt->BitsPerPixel;
    #endif


    if(Modes.screen_upscale > 1) {
	 	game.bigScreen = SDL_SetVideoMode(Modes.screen_width * Modes.screen_upscale, Modes.screen_height * Modes.screen_upscale, Modes.screen_depth, SDL_HWPALETTE|SDL_DOUBLEBUF);	
	 	game.screen = SDL_CreateRGBSurface(0, Modes.screen_width, Modes.screen_height, Modes.screen_depth, 0, 0, 0, 0);
	} else {
		game.screen = SDL_SetVideoMode(Modes.screen_width, Modes.screen_width, Modes.screen_depth, SDL_HWPALETTE|SDL_DOUBLEBUF);		
	}
	
	if (game.screen == NULL)
	{
		printf("Couldn't set screen mode to %d x %d: %s\n", Modes.screen_width, Modes.screen_height, SDL_GetError());

		exit(1);
	}

    /* Load the font */
    
    game.font = loadFont("TerminusTTF-Bold-4.46.0.ttf", 12);
       
    game.listFont = loadFont("TerminusTTF-Bold-4.46.0.ttf", 18);

	/* Set the screen title */
	
	SDL_WM_SetCaption(title, NULL);

	initMaps();
}

void cleanup()
{
	/* Close the font */
	
	closeFont(game.font);
	
	/* Close SDL_TTF */
	
	TTF_Quit();

	
	/* Shut down SDL */
	
	SDL_Quit();
}
