
#include "dump1090.h"
#include "structs.h"
#include "parula.h"
#include "magma.h"
#include "monokai.h"
#include "SDL2/SDL2_gfxPrimitives.h"

#define CENTEROFFSET .375

static uint64_t mstime(void) {
    struct timeval tv;
    uint64_t mst;

    gettimeofday(&tv, NULL);
    mst = ((uint64_t)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}

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

SDL_Color signalToColor(int signal) {
    SDL_Color planeColor;

    if(signal > 127) {
        signal = 127;
    }

    if(signal < 0) {
        planeColor = setColor(96, 96, 96);      
    } else {
        if(MAGMA) {
            planeColor = setColor(magma[signal][0], magma[signal][1], magma[signal][2]);                    
        } else {
            planeColor = setColor(parula[signal][0], parula[signal][1], parula[signal][2]);                 
        }
    }

    return planeColor;
}

int screenDist(double d) {

    double scale_factor = (Modes.screen_width > Modes.screen_height) ? Modes.screen_width : Modes.screen_height;

    if(Modes.mapLogDist) {
        return round(0.95 * scale_factor * 0.5 * log(1.0+fabs(d)) / log(1.0+Modes.maxDist));    
    } else {
        return round(0.95 * scale_factor * 0.5 * fabs(d) / Modes.maxDist);    
    }
}

void pxFromLonLat(double *dx, double *dy, double lon, double lat) {
    if(!lon || !lat) {
        *dx = 0;
        *dy = 0;
        return;
    }

    *dx = 6371.0 * (lon - Modes.fUserLon) * M_PI / 180.0f * cos(((lat + Modes.fUserLat)/2.0f) * M_PI / 180.0f);
    *dy = 6371.0 * (lat - Modes.fUserLat) * M_PI / 180.0f;
}


void screenCoords(int *outX, int *outY, double dx, double dy) {
    *outX = (Modes.screen_width>>1) + ((dx>0) ? 1 : -1) * screenDist(dx);    
    *outY = (Modes.screen_height * CENTEROFFSET) + ((dy>0) ? 1 : -1) * screenDist(dy);        
}

int outOfBounds(int x, int y) {
    if(x < 0 || x >= Modes.screen_width || y < 0 || y >= Modes.screen_height ) {
        return 1;
    } else {
        return 0;
    }
}

void drawPlaneOffMap(int x, int y, int *returnx, int *returny, SDL_Color planeColor) {

    double arrowWidth = 6.0 * Modes.screen_uiscale;

    float inx = x - (Modes.screen_width>>1);
    float iny = y - Modes.screen_height * CENTEROFFSET;
    
    float outx, outy;
    outx = inx;
    outy = iny;

    if(abs(inx) > abs(y - (Modes.screen_height>>1)) * (float)(Modes.screen_width>>1) / (float)(Modes.screen_height * CENTEROFFSET)) { //left / right quadrants
        outx = (Modes.screen_width>>1) * ((inx > 0) ? 1.0 : -1.0);
        outy = (outx) * iny / (inx);
    } else { // up / down quadrants
        outy = Modes.screen_height * ((iny > 0) ? 1.0-CENTEROFFSET : -CENTEROFFSET );
        outx = (outy) * inx / (iny);
    }

    // circleRGBA (game.renderer,(Modes.screen_width>>1) + outx, Modes.screen_height * CENTEROFFSET + outy,50,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    // thickLineRGBA(game.renderer,Modes.screen_width>>1,Modes.screen_height * CENTEROFFSET, (Modes.screen_width>>1) + outx, Modes.screen_height * CENTEROFFSET + outy,arrowWidth,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    double inmag = sqrt(inx *inx + iny*iny);
    double vec[3];
    vec[0] = inx / inmag;
    vec[1] = iny /inmag;
    vec[2] = 0;

    double up[] = {0,0,1};

    double out[3];

    CROSSVP(out,vec,up);

    int x1, x2, x3, y1, y2, y3;

    // arrow 1
    x1 = (Modes.screen_width>>1) + outx - 2.0 * arrowWidth * vec[0] + round(-arrowWidth*out[0]);
    y1 = (Modes.screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1] + round(-arrowWidth*out[1]);
    x2 = (Modes.screen_width>>1) + outx - 2.0 * arrowWidth * vec[0] + round(arrowWidth*out[0]);
    y2 = (Modes.screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1] + round(arrowWidth*out[1]);
    x3 = (Modes.screen_width>>1) +  outx - arrowWidth * vec[0];
    y3 = (Modes.screen_height * CENTEROFFSET) + outy - arrowWidth * vec[1];
    filledTrigonRGBA(game.renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    // arrow 2
    x1 = (Modes.screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(-arrowWidth*out[0]);
    y1 = (Modes.screen_height * CENTEROFFSET) + outy - 3.0 * arrowWidth * vec[1] + round(-arrowWidth*out[1]);
    x2 = (Modes.screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(arrowWidth*out[0]);
    y2 = (Modes.screen_height * CENTEROFFSET) + outy - 3.0 * arrowWidth * vec[1] + round(arrowWidth*out[1]);
    x3 = (Modes.screen_width>>1) +  outx - 2.0 * arrowWidth * vec[0];
    y3 = (Modes.screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1];
    filledTrigonRGBA(game.renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    *returnx = x3;
    *returny = y3;
}

void drawPlaneHeading(int x, int y, double heading, SDL_Color planeColor)
{
    double body = 8.0 * Modes.screen_uiscale;
    double wing = 6.0 * Modes.screen_uiscale;
    double tail = 3.0 * Modes.screen_uiscale;
    double bodyWidth = 2.0 * Modes.screen_uiscale;

    double vec[3];
    vec[0] = sin(heading * M_PI / 180);
    vec[1] = cos(heading * M_PI / 180);
    vec[2] = 0;

    double up[] = {0,0,1};

    double out[3];

    CROSSVP(out,vec,up);

    int x1, x2, y1, y2;


    // tempCenter

    // circleRGBA(game.renderer, x, y, 10, 255, 0, 0, 255);   

    //body
    x1 = x + round(-body*vec[0]);
    y1 = y + round(-body*vec[1]);
    x2 = x + round(body*vec[0]);
    y2 = y + round(body*vec[1]);

    if(AA) {
        aalineRGBA(game.renderer,x1,y1,x2,y2,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
        aatrigonRGBA(game.renderer, x + round(-wing*.35*out[0]), y + round(-wing*.35*out[1]), x + round(wing*.35*out[0]), y + round(wing*.35*out[1]), x1, y1,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);        
        aacircleRGBA(game.renderer, x2,y2,1,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    } else {
        thickLineRGBA(game.renderer,x,y,x2,y2,bodyWidth,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
        filledTrigonRGBA(game.renderer, x + round(-wing*.35*out[0]), y + round(-wing*.35*out[1]), x + round(wing*.35*out[0]), y + round(wing*.35*out[1]), x1, y1,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);        
        filledCircleRGBA(game.renderer, x2,y2,Modes.screen_uiscale,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    }

    //wing
    x1 = x + round(-wing*out[0]);
    y1 = y + round(-wing*out[1]);
    x2 = x + round(wing*out[0]);
    y2 = y + round(wing*out[1]);

    if(AA) {
        aatrigonRGBA(game.renderer, x1, y1, x2, y2, x+round(body*.35*vec[0]), y+round(body*.35*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    } else {
        filledTrigonRGBA(game.renderer, x1, y1, x2, y2, x+round(body*.35*vec[0]), y+round(body*.35*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    }

    //tail
    x1 = x + round(-body*.75*vec[0]) + round(-tail*out[0]);
    y1 = y + round(-body*.75*vec[1]) + round(-tail*out[1]);
    x2 = x + round(-body*.75*vec[0]) + round(tail*out[0]);
    y2 = y + round(-body*.75*vec[1]) + round(tail*out[1]);

    if(AA) {
        aatrigonRGBA (game.renderer, x1, y1, x2, y2, x+round(-body*.5*vec[0]), y+round(-body*.5*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    } else {
        filledTrigonRGBA (game.renderer, x1, y1, x2, y2, x+round(-body*.5*vec[0]), y+round(-body*.5*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    }
}

void drawPlane(int x, int y, SDL_Color planeColor)
{
    int length = 3.0;

    rectangleRGBA (game.renderer, x - length, y - length, x+length, y + length, planeColor.r, planeColor.g, planeColor.b, SDL_ALPHA_OPAQUE);   
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

        double dx, dy;

        pxFromLonLat(&dx, &dy, oldDx[currentIdx], oldDy[currentIdx]);

        screenCoords(&currentX, &currentY, dx, dy);

        pxFromLonLat(&dx, &dy, oldDx[prevIdx], oldDy[prevIdx]);

        screenCoords(&prevX, &prevY, dx, dy);

        if(outOfBounds(currentX,currentY)) {
            return;
        }

        if(outOfBounds(prevX,prevY)) {
            return;
        }

        double age = pow(1.0 - (double)(now - oldSeen[currentIdx]) / MODES_INTERACTIVE_TRAIL_TTL, 2.2);

        if(age < 0) {
            age = 0;
        }

        uint8_t colorVal = (uint8_t)floor(255.0 * age);
  
        if(AA) {
            aalineRGBA(game.renderer, prevX, prevY, currentX, currentY,colorVal, colorVal, colorVal, SDL_ALPHA_OPAQUE);       
        } else {
            //thickLineRGBA(game.renderer, prevX, prevY, currentX, currentY, 2, colorVal, colorVal, colorVal, SDL_ALPHA_OPAQUE);                  
            thickLineRGBA(game.renderer, prevX, prevY, currentX, currentY, 2 * Modes.screen_uiscale, colorVal, colorVal, colorVal, SDL_ALPHA_OPAQUE);                    
        }   
    }
}

void drawGrid()
{
    int p1km = screenDist(1.0);
    int p10km = screenDist(10.0);
    int p100km = screenDist(100.0);

    hlineRGBA (game.renderer, (Modes.screen_width>>1) - p100km, (Modes.screen_width>>1) + p100km, Modes.screen_height * CENTEROFFSET, grey.r, grey.g, grey.b, SDL_ALPHA_OPAQUE);
    vlineRGBA (game.renderer, Modes.screen_width>>1, (Modes.screen_height * CENTEROFFSET) - p100km, (Modes.screen_height * CENTEROFFSET) + p100km, grey.r, grey.g, grey.b, SDL_ALPHA_OPAQUE);

    if(AA) {
        aacircleRGBA (game.renderer, Modes.screen_width>>1, Modes.screen_height>>1, p1km, pink.r, pink.g, pink.b, 255);
        aacircleRGBA (game.renderer, Modes.screen_width>>1, Modes.screen_height>>1, p10km, pink.r, pink.g, pink.b, 195);
        aacircleRGBA (game.renderer, Modes.screen_width>>1, Modes.screen_height>>1, p100km, pink.r, pink.g, pink.b, 127);
    } else {
        circleRGBA (game.renderer, Modes.screen_width>>1, Modes.screen_height * CENTEROFFSET, p1km, pink.r, pink.g, pink.b, 255);
        circleRGBA (game.renderer, Modes.screen_width>>1, Modes.screen_height * CENTEROFFSET, p10km, pink.r, pink.g, pink.b, 195);
        circleRGBA (game.renderer, Modes.screen_width>>1, Modes.screen_height * CENTEROFFSET, p100km, pink.r, pink.g, pink.b, 127);
    }

    drawString("1km", (Modes.screen_width>>1) + (0.707 * p1km) + 5, (Modes.screen_height * CENTEROFFSET) + (0.707 * p1km) + 5, game.mapFont, pink);   
    drawString("10km", (Modes.screen_width>>1) + (0.707 * p10km) + 5, (Modes.screen_height * CENTEROFFSET) + (0.707 * p10km) + 5, game.mapFont, pink);  
    drawString("100km", (Modes.screen_width>>1) + (0.707 * p100km) + 5, (Modes.screen_height * CENTEROFFSET) + (0.707 * p100km) + 5, game.mapFont, pink);            
}

void drawGeography() {
    int x1, y1, x2, y2;

    for(int i=1; i<mapPoints_count/2; i++) {

        double dx, dy;

        pxFromLonLat(&dx, &dy, mapPoints_relative[(i - 1) * 2], mapPoints_relative[(i - 1) * 2 + 1]);   

        if(!dx || !dy) {
            continue;
        }

        screenCoords(&x1, &y1, dx, dy);

        if(outOfBounds(x1,y1)) {
            continue;
        }

        double d1 = sqrt(dx * dx + dy * dy);

        pxFromLonLat(&dx, &dy, mapPoints_relative[i * 2], mapPoints_relative[i * 2 + 1]);   
        
        if(!dx || !dy) {
            continue;
        }
        
        screenCoords(&x2, &y2, dx, dy);

        if(outOfBounds(x2,y2)) {
            continue;
        }
        
        double d2 = sqrt(dx* dx + dy * dy);


        //double alpha = 255.0 * (d1+d2) / 2;
        //alpha =  255.0 - alpha / Modes.maxDist;    
        double alpha = 1.0 - (d1+d2) / (2 * Modes.maxDist);


        alpha = (alpha < 0) ? 0 : alpha;

        if(AA) {
            aalineRGBA(game.renderer, x1, y1, x2, y2,purple.r,purple.g,purple.b, (alpha < 0) ? 0 : (uint8_t) alpha);
        } else {
            //thickLineRGBA(game.renderer, x1, y1, x2, y2, Modes.screen_uiscale, purple.r,purple.g,purple.b, (alpha < 0) ? 0 : (uint8_t) alpha);
            //thickLineRGBA(game.renderer, x1, y1, x2, y2, Modes.screen_uiscale, alpha * purple.r + (1.0-alpha) * blue.r, alpha * purple.g + (1.0-alpha) * blue.g, alpha * purple.b + (1.0-alpha) * blue.b, 255 * alpha);
            lineRGBA(game.renderer, x1, y1, x2, y2, alpha * purple.r + (1.0-alpha) * blue.r, alpha * purple.g + (1.0-alpha) * blue.g, alpha * purple.b + (1.0-alpha) * blue.b, 255 * alpha);
        }
    }
    
    // int *screen_x = (int *) malloc(mapPoints_count * sizeof(int));
    // int *screen_y = (int *) malloc(mapPoints_count * sizeof(int));

    // initializeMap(screen_x, screen_y);

    // filledPolygonRGBA(game.renderer,screen_x, screen_y, mapPoints_count, 100, 100, 100, 255);
}

void drawMap(void) {
    struct aircraft *a = Modes.aircrafts;
    time_t now = time(NULL);

    drawGeography();

    //drawGrid(); 

    while(a) {
        if ((now - a->seen) < Modes.interactive_display_ttl) {
            if (a->bFlags & MODES_ACFLAGS_LATLON_VALID) {

                unsigned char * pSig       = a->signalLevel;
                unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
                                              pSig[4] + pSig[5] + pSig[6] + pSig[7] + 3) >> 3; 

                drawTrail(a->oldDx, a->oldDy, a->oldSeen, a->oldIdx);

                int colorIdx;
                if((int)(now - a->seen) > MODES_INTERACTIVE_DISPLAY_ACTIVE) {
                    colorIdx = -1;
                } else {
                    colorIdx = signalAverage;
                }

                SDL_Color planeColor = signalToColor(colorIdx);
                int x, y;
                //screenCoords(&x, &y, a->dx, a->dy);

                double dx, dy;
                pxFromLonLat(&dx, &dy, a->lon, a->lat);
                screenCoords(&x, &y, dx, dy);

                if(outOfBounds(x,y)) {
                    int outx, outy;
                    drawPlaneOffMap(x, y, &outx, &outy, planeColor); 

                    drawStringBG(a->flight, outx + 5, outy + game.mapFontHeight, game.mapBoldFont, white, black);                    

                    char alt[10] = " ";
                    snprintf(alt,10,"%dm", a->altitude);
                    drawStringBG(alt, outx + 5, outy + 2*game.mapFontHeight, game.mapFont, grey, black);                    

                    char speed[10] = " ";
                    snprintf(speed,10,"%dkm/h", a->speed);
                    drawStringBG(speed, outx + 5, outy + 3*game.mapFontHeight, game.mapFont, grey, black);                    
       
                    // continue;
                }


                if(a->created == 0) {
                    a->created = mstime();
                }

                double age_ms = (double)(mstime() - a->created);
                if(age_ms < 500) {
                    circleRGBA(game.renderer, x, y, 500 - age_ms, 255,255, 255, (uint8_t)(255.0 * age_ms / 500.0));   
                } else {
                    if(MODES_ACFLAGS_HEADING_VALID) {
                        drawPlaneHeading(x, y,a->track, planeColor);

                        //char flight[11] = " ";
                        //snprintf(flight,11," %s ", a->flight);
                        //drawStringBG(flight, x, y + game.mapFontHeight, game.mapBoldFont, black, planeColor);
                        drawStringBG(a->flight, x + 5, y + game.mapFontHeight, game.mapBoldFont, white, black);                    

                        char alt[10] = " ";
                        snprintf(alt,10,"%dm", a->altitude);
                        drawStringBG(alt, x + 5, y + 2*game.mapFontHeight, game.mapFont, grey, black);                    

                        char speed[10] = " ";
                        snprintf(speed,10,"%dkm/h", a->speed);
                        drawStringBG(speed, x + 5, y + 3*game.mapFontHeight, game.mapFont, grey, black);                    

                        lineRGBA(game.renderer, x, y, x, y + 4*game.mapFontHeight, grey.r, grey.g, grey.b, SDL_ALPHA_OPAQUE);
                    } else {
                        drawPlane(x, y, planeColor);
                    }  
                }
            }
        }
        a = a->next;
    }

    // int mx, my;
    // SDL_GetMouseState(&mx, &my);
    // mx /= Modes.screen_upscale;
    // my /= Modes.screen_upscale;

    // char mousepos[10] = " ";
    // snprintf(mousepos,10,"%d %d", mx, my);

    // int outx, outy;
    // drawPlaneOffMap(mx, my, &outx, &outy, white); 


    // char linepos[10] = " ";
    // snprintf(linepos,10,"%2.2f %2.2f", outx, outy);
    // int shiftedx, shiftedy;
    // if(outx > (Modes.screen_width>>1)) {
    //     shiftedx = outx - 5 * game.mapFontHeight;
    // } else {
    //     shiftedx = outx + game.mapFontHeight;
    // }

    // if(outy > (Modes.screen_height>>1)) {
    //     shiftedy = outy - game.mapFontHeight;
    // } else {
    //     shiftedy = outy + game.mapFontHeight;
    // }


    // drawStringBG(linepos, shiftedx, shiftedy, game.mapBoldFont, white, black);                    

    // // drawPlane(mx, my, signalToColor(100));
    // drawStringBG(mousepos, mx + 5, my - game.mapFontHeight, game.mapBoldFont, white, black);                    

}

// void initializeMap(short *screen_x, short *screen_y) {
//     int out_x, out_y;
//     for(int i=0; i<mapPoints_count; i++) {
//         screenCoords(&out_x, &out_y, (double) mapPoints_x[i], (double) -mapPoints_y[i]); 

//         // if(out_x < 0) {
//         //     out_x = 0;
//         // }     

//         // if(out_y < 0) {
//         //     out_y = 0;
//         // }


//         // if(out_x >= Modes.screen_width) {
//         //     out_x = Modes.screen_width;
//         // }     

//         // if(out_y >= Modes.screen_height) {
//         //     out_y = Modes.screen_height;
//         // }

//         screen_x[i] = out_x;
//         screen_y[i] = out_y;
//     }
// }


