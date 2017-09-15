#include "dump1090.h"
#include "draw.h"
#include "parula.h"
#include "SDL/SDL_gfxPrimitives.h"

extern void drawString(char *, int, int, TTF_Font *, SDL_Color);

#define LOGPROJECTION 1

#define MAXDIST 1000.0

void CROSSVP(double *v, double *u, double *w) 
{                                                                       
    v[0] = u[1]*w[2] - u[2]*(w)[1];                             
    v[1] = u[2]*w[0] - u[0]*(w)[2];                             
    v[2] = u[0]*w[1] - u[1]*(w)[0];                             
}

SDL_Color setColor(uint8_t r, uint8_t g, uint8_t b) {
	SDL_Color out;
	out.r = r;
	out.g = g;
	out.b = b;
	return out;
}

int screenDist(double d) {
	if(LOGPROJECTION) {
		return round((double)SCREEN_WIDTH * 0.5 * log(1.0+d) / log(1.0+MAXDIST));    
	} else {
		return round((double)SCREEN_WIDTH * 0.5 * d / MAXDIST);    
	}
}

void screenCoords(int *outX, int *outY, double dx, double dy) {
	*outX = (SCREEN_WIDTH>>1) + screenDist(dx);    
	*outY = (SCREEN_HEIGHT>>1) + screenDist(dy);    	
}

int outOfBounds(int x, int y) {
    if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT ) {
    	return 1;
    } else {
    	return 0;
    }
}


void drawPlaneHeading(double dx, double dy, double heading, int signal, char *flight)
{
	//int planeWidth = 2;
	int x, y;
	screenCoords(&x, &y, dx, dy);

    if(outOfBounds(x,y)) {
    	return;
    }

	if(signal > 127) {
		signal = 127;
	}

	SDL_Color planeColor;

 	if(signal < 0) {
	    planeColor = setColor(96, 96, 96);    	
    } else {
	    planeColor = setColor(parula[signal][0], parula[signal][1], parula[signal][2]);    	
	}

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

    //thickLineRGBA(game.screen,x1,y1,x2,y2,planeWidth,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    aalineRGBA(game.screen,x1,y1,x2,y2,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //wing

	x1 = x + round(-wing*out[0]);
    y1 = y + round(-wing*out[1]);
    x2 = x + round(wing*out[0]);
    y2 = y + round(wing*out[1]);

    //thickLineRGBA(game.screen,x1,y1,x2,y2,planeWidth,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    aalineRGBA(game.screen,x1,y1,x2,y2,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);    

    //tail

	x1 = x + round(-body*vec[0]) + round(-tail*out[0]);
    y1 = y + round(-body*vec[1]) + round(-tail*out[1]);
	x2 = x + round(-body*vec[0]) + round(tail*out[0]);
    y2 = y + round(-body*vec[1]) + round(tail*out[1]);

    //thickLineRGBA(game.screen,x1,y1,x2,y2,planeWidth,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    aalineRGBA(game.screen,x1,y1,x2,y2,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    if(strlen(flight)) {	
	    drawString(flight, x + 10, y + 10, game.font, planeColor);
	}	
}

void drawPlane(double dx, double dy, int signal)
{

	int x, y;
	screenCoords(&x, &y, dx, dy);

    if(outOfBounds(x,y)) {
    	return;
    }

	if(signal > 127) {
		signal = 127;
	}

 	SDL_Color planeColor;

 	if(signal < 0) {
	    planeColor = setColor(96, 96, 96);    	
    } else {
	    planeColor = setColor(parula[signal][0], parula[signal][1], parula[signal][2]);    	
	}

	int length = 3.0;

	rectangleRGBA (game.screen, x - length, y - length, x+length, y + length, planeColor.r, planeColor.g, planeColor.b, SDL_ALPHA_OPAQUE);   
}

void drawTrail(double *oldDx, double *oldDy, time_t * oldSeen, int idx) {

	
	int currentIdx, prevIdx;

	int currentX, currentY, prevX, prevY;

    time_t now = time(NULL);

	for(int i=0; i < (MODES_INTERACTIVE_TRAIL_LENGTH - 1); i++) {
		currentIdx = (idx - i) % MODES_INTERACTIVE_TRAIL_LENGTH;
		currentIdx = currentIdx < 0 ? currentIdx + MODES_INTERACTIVE_TRAIL_LENGTH : currentIdx;		
		prevIdx = (idx - (i + 1)) % MODES_INTERACTIVE_TRAIL_LENGTH;
		prevIdx = prevIdx < 0 ? prevIdx + MODES_INTERACTIVE_TRAIL_LENGTH : prevIdx;		  	

		if(oldDx[currentIdx] == 0 || oldDy[currentIdx] == 0) {
			continue;
		}

		if(oldDx[prevIdx] == 0 || oldDy[prevIdx] == 0) {
			continue;
		}

	    screenCoords(&currentX, &currentY, oldDx[currentIdx], oldDy[currentIdx]);

	    screenCoords(&prevX, &prevY, oldDx[prevIdx], oldDy[prevIdx]);

	    if(outOfBounds(currentX,currentY)) {
	    	return;
	    }

	    if(outOfBounds(prevX,prevY)) {
	    	return;
	    }

		double age = 1.0 - (double)(now - oldSeen[currentIdx]) / MODES_INTERACTIVE_TRAIL_TTL;

		if(age < 0) {
			age = 0;
		}

		uint8_t colorVal = (uint8_t)floor(127.0 * age);

	    aalineRGBA(game.screen, prevX, prevY, currentX, currentY,colorVal, colorVal, colorVal, SDL_ALPHA_OPAQUE);	    
	}
}


void drawGrid()
{
	int p1km = screenDist(1.0);
	int p10km = screenDist(10.0);
	int p100km = screenDist(100.0);

	hlineRGBA (game.screen, 0, SCREEN_WIDTH, SCREEN_HEIGHT>>1, 127, 127, 127, SDL_ALPHA_OPAQUE);
	vlineRGBA (game.screen, SCREEN_WIDTH>>1, 0, SCREEN_HEIGHT, 127, 127, 127, SDL_ALPHA_OPAQUE);

	aacircleRGBA (game.screen, SCREEN_WIDTH>>1, SCREEN_HEIGHT>>1, p1km, 64, 64, 64, SDL_ALPHA_OPAQUE);	
	aacircleRGBA (game.screen, SCREEN_WIDTH>>1, SCREEN_HEIGHT>>1, p10km, 64, 64, 64, SDL_ALPHA_OPAQUE);
	aacircleRGBA (game.screen, SCREEN_WIDTH>>1, SCREEN_HEIGHT>>1, p100km, 64, 64, 64, SDL_ALPHA_OPAQUE);		

    drawString("1km", (SCREEN_WIDTH>>1) + p1km + 5, (SCREEN_HEIGHT>>1) + 5, game.font, setColor(64,64,64));	
    drawString("10km", (SCREEN_WIDTH>>1) + p10km + 5, (SCREEN_HEIGHT>>1) + 5, game.font, setColor(64,64,64));	
    drawString("100km", (SCREEN_WIDTH>>1) + p100km + 5, (SCREEN_HEIGHT>>1) + 5, game.font, setColor(64,64,64));	        
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
