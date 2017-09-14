#include "draw.h"
#include "parula.h"

extern void drawString(char *, int, int, TTF_Font *, SDL_Color);

void CROSSVP(double *v, double *u, double *w) 
{                                                                       
    v[0] = u[1]*w[2] - u[2]*(w)[1];                             
    v[1] = u[2]*w[0] - u[0]*(w)[2];                             
    v[2] = u[0]*w[1] - u[1]*(w)[0];                             
}


SDL_Point screenCoords(double dx, double dy) {
	SDL_Point out;
    out.x = round(320.0 * (0.5 + (dx / 64.0)));
    out.y = round(240.0 * (0.5 + (dy / 48.0)));

    return out;
}

void drawPlaneHeading(double dx, double dy, double heading, int signal, char *flight)
{

	SDL_Point center = screenCoords(dx,dy);

    if(center.x < 0 || center.x >= 320 || center.y < 0 || center.y >= 240 ) {
    	return;
    }

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

	x1 = center.x + round(-body*vec[0]);
    y1 = center.y + round(-body*vec[1]);
    x2 = center.x + round(body*vec[0]);
    y2 = center.y + round(body*vec[1]);

    SDL_RenderDrawLine(game.renderer, x1, y1, x2, y2);

    //wing

	x1 = center.x + round(-wing*out[0]);
    y1 = center.y + round(-wing*out[1]);
    x2 = center.x + round(wing*out[0]);
    y2 = center.y + round(wing*out[1]);

    SDL_RenderDrawLine(game.renderer, x1, y1, x2, y2);

    //tail

	x1 = center.x + round(-body*vec[0]) + round(-tail*out[0]);
    y1 = center.y + round(-body*vec[1]) + round(-tail*out[1]);
	x2 = center.x + round(-body*vec[0]) + round(tail*out[0]);
    y2 = center.y + round(-body*vec[1]) + round(tail*out[1]);

    SDL_RenderDrawLine(game.renderer, x1, y1, x2, y2);

    SDL_Color color = { parula[signal][0], parula[signal][1], parula[signal][2], 255};

    drawString(flight, center.x + 10, center.y + 10, game.font, color);
}

void drawPlane(double dx, double dy, int signal)
{

	SDL_Point center = screenCoords(dx,dy);

    if(center.x < 0 || center.x >= 320 || center.y < 0 || center.y >= 240 ) {
    	return;
    }

	if(signal > 127) {
		signal = 127;
	}

    SDL_SetRenderDrawColor(game.renderer, parula[signal][0], parula[signal][1], parula[signal][2], SDL_ALPHA_OPAQUE);

	int length = 3.0;

    SDL_RenderDrawLine(game.renderer, center.x-length	, center.y 		, center.x+length 	, center.y 		);
    SDL_RenderDrawLine(game.renderer, center.x 		, center.y-length	, center.x 		, center.y+length 	);
}

void drawTrail(double *oldDx, double *oldDy, int idx) {
	 int currentIdx, prevIdx;

	 SDL_Point current, prev;

	for(int i=1; i < 32; i++) {
		currentIdx = (idx - (i - 1)) % 32;
		prevIdx = (idx - (i - 2)) % 32;

		if(oldDx[currentIdx] == 0 && oldDy[currentIdx] == 0) {
			continue;
		}

	    //SDL_SetRenderDrawColor(game.renderer, (i<<3)-1, (i<<3)-1, (i<<3)-1, SDL_ALPHA_OPAQUE);
	    SDL_SetRenderDrawColor(game.renderer, 0,60,60, SDL_ALPHA_OPAQUE);

	    current = screenCoords(oldDx[currentIdx], oldDy[currentIdx]);
	    prev = screenCoords(oldDx[prevIdx], oldDy[prevIdx]);

        if(current.x < 0 || current.x >= 320 || current.y < 0 || current.y >= 240 ) {
    		continue;
	    }

	    if(prev.x < 0 || prev.x >= 320 || prev.y < 0 || prev.y >= 240 ) {
    		continue;
   		 }

	    SDL_RenderDrawLine(game.renderer, prev.x, prev.y, current.x, current.y);

	    SDL_SetRenderDrawColor(game.renderer, 255,0,0, SDL_ALPHA_OPAQUE);

		SDL_Rect spot = {.x = current.x-1, .y = current.y-1, .w = 2, .h = 2};
	    SDL_RenderFillRect(game.renderer, &spot);
	}
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
