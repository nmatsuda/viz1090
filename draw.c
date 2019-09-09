#include "dump1090.h"
#include "structs.h"
#include "SDL2/SDL2_rotozoom.h"
#include "SDL2/SDL2_gfxPrimitives.h"

//color schemes
#include "parula.h"
#include "monokai.h"

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
        planeColor = setColor(parula[signal][0], parula[signal][1], parula[signal][2]);                 
    }

    return planeColor;
}

int screenDist(double d) {

    double scale_factor = (appData.screen_width > appData.screen_height) ? appData.screen_width : appData.screen_height;

    if(appData.mapLogDist) {
        return round(0.95 * scale_factor * 0.5 * log(1.0+fabs(d)) / log(1.0+appData.maxDist));    
    } else {
        return round(0.95 * scale_factor * 0.5 * fabs(d) / appData.maxDist);    
    }
}

void pxFromLonLat(double *dx, double *dy, double lon, double lat) {
    if(!lon || !lat) {
        *dx = 0;
        *dy = 0;
        return;
    }

    *dx = 6371.0 * (lon - appData.centerLon) * M_PI / 180.0f * cos(((lat + appData.centerLat)/2.0f) * M_PI / 180.0f);
    *dy = 6371.0 * (lat - appData.centerLat) * M_PI / 180.0f;
}


void screenCoords(int *outX, int *outY, double dx, double dy) {
    *outX = (appData.screen_width>>1) + ((dx>0) ? 1 : -1) * screenDist(dx);    
    *outY = (appData.screen_height * CENTEROFFSET) + ((dy>0) ? -1 : 1) * screenDist(dy);        
}

int outOfBounds(int x, int y) {
    if(x < 0 || x >= appData.screen_width || y < 0 || y >= appData.screen_height ) {
        return 1;
    } else {
        return 0;
    }
}

void drawPlaneOffMap(int x, int y, int *returnx, int *returny, SDL_Color planeColor) {

    double arrowWidth = 6.0 * appData.screen_uiscale;

    float inx = x - (appData.screen_width>>1);
    float iny = y - appData.screen_height * CENTEROFFSET;
    
    float outx, outy;
    outx = inx;
    outy = iny;

    if(abs(inx) > abs(y - (appData.screen_height>>1)) * (float)(appData.screen_width>>1) / (float)(appData.screen_height * CENTEROFFSET)) { //left / right quadrants
        outx = (appData.screen_width>>1) * ((inx > 0) ? 1.0 : -1.0);
        outy = (outx) * iny / (inx);
    } else { // up / down quadrants
        outy = appData.screen_height * ((iny > 0) ? 1.0-CENTEROFFSET : -CENTEROFFSET );
        outx = (outy) * inx / (iny);
    }

    // circleRGBA (appData.renderer,(appData.screen_width>>1) + outx, appData.screen_height * CENTEROFFSET + outy,50,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    // thickLineRGBA(appData.renderer,appData.screen_width>>1,appData.screen_height * CENTEROFFSET, (appData.screen_width>>1) + outx, appData.screen_height * CENTEROFFSET + outy,arrowWidth,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

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
    x1 = (appData.screen_width>>1) + outx - 2.0 * arrowWidth * vec[0] + round(-arrowWidth*out[0]);
    y1 = (appData.screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1] + round(-arrowWidth*out[1]);
    x2 = (appData.screen_width>>1) + outx - 2.0 * arrowWidth * vec[0] + round(arrowWidth*out[0]);
    y2 = (appData.screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1] + round(arrowWidth*out[1]);
    x3 = (appData.screen_width>>1) +  outx - arrowWidth * vec[0];
    y3 = (appData.screen_height * CENTEROFFSET) + outy - arrowWidth * vec[1];
    filledTrigonRGBA(appData.renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    // arrow 2
    x1 = (appData.screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(-arrowWidth*out[0]);
    y1 = (appData.screen_height * CENTEROFFSET) + outy - 3.0 * arrowWidth * vec[1] + round(-arrowWidth*out[1]);
    x2 = (appData.screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(arrowWidth*out[0]);
    y2 = (appData.screen_height * CENTEROFFSET) + outy - 3.0 * arrowWidth * vec[1] + round(arrowWidth*out[1]);
    x3 = (appData.screen_width>>1) +  outx - 2.0 * arrowWidth * vec[0];
    y3 = (appData.screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1];
    filledTrigonRGBA(appData.renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    *returnx = x3;
    *returny = y3;
}

void drawPlaneHeading(int x, int y, double heading, SDL_Color planeColor)
{
    double body = 8.0 * appData.screen_uiscale;
    double wing = 6.0 * appData.screen_uiscale;
    double tail = 3.0 * appData.screen_uiscale;
    double bodyWidth = 2.0 * appData.screen_uiscale;

    double vec[3];
    vec[0] = sin(heading * M_PI / 180);
    vec[1] = -cos(heading * M_PI / 180);
    vec[2] = 0;

    double up[] = {0,0,1};

    double out[3];

    CROSSVP(out,vec,up);

    int x1, x2, y1, y2;


    // tempCenter

    // circleRGBA(appData.renderer, x, y, 10, 255, 0, 0, 255);   

    //body
    x1 = x + round(-body*vec[0]);
    y1 = y + round(-body*vec[1]);
    x2 = x + round(body*vec[0]);
    y2 = y + round(body*vec[1]);

    thickLineRGBA(appData.renderer,x,y,x2,y2,bodyWidth,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    filledTrigonRGBA(appData.renderer, x + round(-wing*.35*out[0]), y + round(-wing*.35*out[1]), x + round(wing*.35*out[0]), y + round(wing*.35*out[1]), x1, y1,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);        
    filledCircleRGBA(appData.renderer, x2,y2,appData.screen_uiscale,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //wing
    x1 = x + round(-wing*out[0]);
    y1 = y + round(-wing*out[1]);
    x2 = x + round(wing*out[0]);
    y2 = y + round(wing*out[1]);

    filledTrigonRGBA(appData.renderer, x1, y1, x2, y2, x+round(body*.35*vec[0]), y+round(body*.35*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //tail
    x1 = x + round(-body*.75*vec[0]) + round(-tail*out[0]);
    y1 = y + round(-body*.75*vec[1]) + round(-tail*out[1]);
    x2 = x + round(-body*.75*vec[0]) + round(tail*out[0]);
    y2 = y + round(-body*.75*vec[1]) + round(tail*out[1]);

    filledTrigonRGBA (appData.renderer, x1, y1, x2, y2, x+round(-body*.5*vec[0]), y+round(-body*.5*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
}

void drawPlane(int x, int y, SDL_Color planeColor)
{
    int length = 3.0;

    rectangleRGBA (appData.renderer, x - length, y - length, x+length, y + length, planeColor.r, planeColor.g, planeColor.b, SDL_ALPHA_OPAQUE);   
}


void drawTrail(double *oldDx, double *oldDy, double *oldHeading, time_t * oldSeen, int idx) {

    int currentIdx, prevIdx;

    int currentX, currentY, prevX, prevY;

    time_t now = time(NULL);

    for(int i=0; i < (TRAIL_LENGTH - 1); i++) {
        currentIdx = (idx - i) % TRAIL_LENGTH;
        currentIdx = currentIdx < 0 ? currentIdx + TRAIL_LENGTH : currentIdx;     
        prevIdx = (idx - (i + 1)) % TRAIL_LENGTH;
        prevIdx = prevIdx < 0 ? prevIdx + TRAIL_LENGTH : prevIdx;         

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

        double age = pow(1.0 - (double)(now - oldSeen[currentIdx]) / TRAIL_TTL, 2.2);

        if(age < 0) {
            age = 0;
        }

        uint8_t colorVal = (uint8_t)floor(255.0 * age);
                   
        thickLineRGBA(appData.renderer, prevX, prevY, currentX, currentY, 4 * appData.screen_uiscale, colorVal, colorVal, colorVal, 127);                    

        //tick marks

        double vec[3];
        vec[0] = sin(oldHeading[currentIdx] * M_PI / 180);
        vec[1] = -cos(oldHeading[currentIdx] * M_PI / 180);
        vec[2] = 0;

        double up[] = {0,0,1};

        double out[3];

        CROSSVP(out,vec,up);


        int x1, y1, x2, y2;

        int cross_size = 8 * appData.screen_uiscale;

        //forward cross
        x1 = currentX + round(-cross_size*vec[0]);
        y1 = currentY + round(-cross_size*vec[1]);
        x2 = currentX + round(cross_size*vec[0]);
        y2 = currentY + round(cross_size*vec[1]);

        lineRGBA(appData.renderer,x1,y1,x2,y2,colorVal,colorVal,colorVal,127);
   
        //side cross
        x1 = currentX + round(-cross_size*out[0]);
        y1 = currentY + round(-cross_size*out[1]);
        x2 = currentX + round(cross_size*out[0]);
        y2 = currentY + round(cross_size*out[1]);
        
        lineRGBA(appData.renderer,x1,y1,x2,y2,colorVal,colorVal,colorVal,127);
    }
}

void drawGrid()
{
    int p1km = screenDist(1.0);
    int p10km = screenDist(10.0);
    int p100km = screenDist(100.0);

    circleRGBA (appData.renderer, appData.screen_width>>1, appData.screen_height * CENTEROFFSET, p1km, pink.r, pink.g, pink.b, 255);
    circleRGBA (appData.renderer, appData.screen_width>>1, appData.screen_height * CENTEROFFSET, p10km, pink.r, pink.g, pink.b, 195);
    circleRGBA (appData.renderer, appData.screen_width>>1, appData.screen_height * CENTEROFFSET, p100km, pink.r, pink.g, pink.b, 127);

    drawString("1km", (appData.screen_width>>1) + (0.707 * p1km) + 5, (appData.screen_height * CENTEROFFSET) + (0.707 * p1km) + 5, appData.mapFont, pink);   
    drawString("10km", (appData.screen_width>>1) + (0.707 * p10km) + 5, (appData.screen_height * CENTEROFFSET) + (0.707 * p10km) + 5, appData.mapFont, pink);  
    drawString("100km", (appData.screen_width>>1) + (0.707 * p100km) + 5, (appData.screen_height * CENTEROFFSET) + (0.707 * p100km) + 5, appData.mapFont, pink);            
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
        //alpha =  255.0 - alpha / appData.maxDist;    
        double alpha = 1.0 - (d1+d2) / (2 * appData.maxDist);


        alpha = (alpha < 0) ? 0 : alpha;
        lineRGBA(appData.renderer, x1, y1, x2, y2, alpha * purple.r + (1.0-alpha) * blue.r, alpha * purple.g + (1.0-alpha) * blue.g, alpha * purple.b + (1.0-alpha) * blue.b, 255 * alpha);
        
    }
}

void drawMap() {
    struct planeObj *p = planes;
    time_t now = time(NULL);

    drawGeography();

    drawGrid(); 

    while(p) {
        if ((now - p->seen) < Modes.interactive_display_ttl) {
            if (p->lon && p->lat) {

                unsigned char * pSig       = p->signalLevel;
                unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
                                              pSig[4] + pSig[5] + pSig[6] + pSig[7] + 3) >> 3; 

                drawTrail(p->oldLon, p->oldLat, p->oldHeading, p->oldSeen, p->oldIdx);

                int colorIdx;
                if((int)(now - p->seen) > DISPLAY_ACTIVE) {
                    colorIdx = -1;
                } else {
                    colorIdx = signalAverage;
                }

                SDL_Color planeColor = signalToColor(colorIdx);
                int x, y;
                //screenCoords(&x, &y, p->dx, p->dy);

                double dx, dy;
                pxFromLonLat(&dx, &dy, p->lon, p->lat);
                screenCoords(&x, &y, dx, dy);

                if(outOfBounds(x,y)) {
                    int outx, outy;
                    drawPlaneOffMap(x, y, &outx, &outy, planeColor); 

                    drawStringBG(p->flight, outx + 5, outy + appData.mapFontHeight, appData.mapBoldFont, white, black);                    

                    char alt[10] = " ";
                    snprintf(alt,10,"%dm", p->altitude);
                    drawStringBG(alt, outx + 5, outy + 2*appData.mapFontHeight, appData.mapFont, grey, black);                    

                    char speed[10] = " ";
                    snprintf(speed,10,"%dkm/h", p->speed);
                    drawStringBG(speed, outx + 5, outy + 3*appData.mapFontHeight, appData.mapFont, grey, black);                    
       
                    // continue;
                }


                if(p->created == 0) {
                    p->created = mstime();
                }

                double age_ms = (double)(mstime() - p->created);
                if(age_ms < 500) {
                    circleRGBA(appData.renderer, x, y, 500 - age_ms, 255,255, 255, (uint8_t)(255.0 * age_ms / 500.0));   
                } else {
                    if(MODES_ACFLAGS_HEADING_VALID) {
                        drawPlaneHeading(x, y,p->track, planeColor);

                        //char flight[11] = " ";
                        //snprintf(flight,11," %s ", p->flight);
                        //drawStringBG(flight, x, y + appData.mapFontHeight, appData.mapBoldFont, black, planeColor);
                        drawStringBG(p->flight, x + 5, y + appData.mapFontHeight, appData.mapBoldFont, white, black);                    

                        char alt[10] = " ";
                        snprintf(alt,10,"%dm", p->altitude);
                        drawStringBG(alt, x + 5, y + 2*appData.mapFontHeight, appData.mapFont, grey, black);                    

                        char speed[10] = " ";
                        snprintf(speed,10,"%dkm/h", p->speed);
                        drawStringBG(speed, x + 5, y + 3*appData.mapFontHeight, appData.mapFont, grey, black);                    

                        lineRGBA(appData.renderer, x, y, x, y + 4*appData.mapFontHeight, grey.r, grey.g, grey.b, SDL_ALPHA_OPAQUE);
                    } else {
                        drawPlane(x, y, planeColor);
                    }  
                }
            }
        }
        p = p->next;
    }
}

//
// 
//

void draw() {

    if ((mstime() - appData.lastFrameTime) < FRAMETIME) {
        return;
    }

    appData.lastFrameTime = mstime();

    updatePlanes();

    updateStatus();

    SDL_SetRenderDrawColor( appData.renderer, 0, 0, 0, 0);

    SDL_RenderClear(appData.renderer);

    drawMap();  
    drawStatus();
    if(appData.showList) {
       drawList(0);
    }

    SDL_RenderPresent(appData.renderer);    
}