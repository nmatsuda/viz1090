#include "draw.h"
#include "parula.h"

extern void drawString(char *, int, int, TTF_Font *, SDL_Color);

void CROSSVP(double *v, double *u, double *w) 
{                                                                       
    v[0] = u[1]*w[2] - u[2]*(w)[1];                             
    v[1] = u[2]*w[0] - u[0]*(w)[2];                             
    v[2] = u[0]*w[1] - u[1]*(w)[0];                             
}

void drawPlaneHeading(int x, int y, double heading, int signal, char *flight)
{

	if(signal > 127) {
		signal = 127;
	}

    SDL_SetRenderDrawColor(game.renderer, parula[signal][0], parula[signal][1], parula[signal][2], SDL_ALPHA_OPAQUE);

	double body = 8.0;
	double wing = 6.0;
	double tail = 3.0;

	double vec[3];
	vec[0] = sin(heading * M_PI / 180);
    vec[1] = cos(heading * M_PI / 180);
    vec[2] = 0;

    double up[] = {0,0,1};

    double out[3];

    CROSSVP(out,vec,up);

    int x1, x2, y1, y2;

    //body

	x1 = x + round(-body*vec[0]);
    y1 = y + round(-body*vec[1]);
    x2 = x + round(body*vec[0]);
    y2 = y + round(body*vec[1]);

    SDL_RenderDrawLine(game.renderer, x1, y1, x2, y2);

    //wing

	x1 = x + round(-wing*out[0]);
    y1 = y + round(-wing*out[1]);
    x2 = x + round(wing*out[0]);
    y2 = y + round(wing*out[1]);

    SDL_RenderDrawLine(game.renderer, x1, y1, x2, y2);

    //tail

	x1 = x + round(-body*vec[0]) + round(-tail*out[0]);
    y1 = y + round(-body*vec[1]) + round(-tail*out[1]);
	x2 = x + round(-body*vec[0]) + round(tail*out[0]);
    y2 = y + round(-body*vec[1]) + round(tail*out[1]);

    SDL_RenderDrawLine(game.renderer, x1, y1, x2, y2);


    SDL_Color color = { parula[signal][0], parula[signal][1], parula[signal][2]};

    drawString(flight, x, y - 10, game.font, color);
}

void drawPlane(int x, int y, int signal)
{
    SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

	int length = 3.0;

	double vec[3];

    SDL_RenderDrawLine(game.renderer, x-length	, y 		, x+length 	, y 		);
    SDL_RenderDrawLine(game.renderer, x 		, y-length	, x 		, y+length 	);
}


void drawGrid()
{
    SDL_SetRenderDrawColor(game.renderer, 40, 40, 40, SDL_ALPHA_OPAQUE);

    SDL_RenderDrawLine(game.renderer, 0, 120, 320, 120);
    SDL_RenderDrawLine(game.renderer, 160, 0, 160, 240);
}


void delay(unsigned int frameLimit)
{
	unsigned int ticks = SDL_GetTicks();

	if (frameLimit < ticks)
	{
		return;
	}
	
	if (frameLimit > ticks + 16)
	{
		SDL_Delay(16);
	}
	
	else
	{
		SDL_Delay(frameLimit - ticks);
	}
}
