#include "dump1090.h"
#include "structs.h"
#include "SDL2/SDL2_rotozoom.h"
#include "SDL2/SDL2_gfxPrimitives.h"
#include "mapdata.h"
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

SDL_Color hsv2SDLColor(double h, double s, double v)
{
    double      hh, p, q, t, ff;
    long        i;
    SDL_Color         out;

    if(s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = (uint8_t)v;
        out.g = (uint8_t)v;
        out.b = (uint8_t)v;
        return out;
    }
    hh = h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = (uint8_t)v;
        out.g = (uint8_t)t;
        out.b = (uint8_t)p;
        break;
    case 1:
        out.r = (uint8_t)q;
        out.g = (uint8_t)v;
        out.b = (uint8_t)p;
        break;
    case 2:
        out.r = (uint8_t)p;
        out.g = (uint8_t)v;
        out.b = (uint8_t)t;
        break;

    case 3:
        out.r = (uint8_t)p;
        out.g = (uint8_t)q;
        out.b = (uint8_t)v;
        break;
    case 4:
        out.r = (uint8_t)t;
        out.g = (uint8_t)p;
        out.b = (uint8_t)v;
        break;
    case 5:
    default:
        out.r = (uint8_t)v;
        out.g = (uint8_t)p;
        out.b = (uint8_t)q;
        break;
    }
    return out;     
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

void latLonFromScreenCoords(double *lat, double *lon, int x, int y) {
    double scale_factor = (appData.screen_width > appData.screen_height) ? appData.screen_width : appData.screen_height;

    double dx = appData.maxDist * (x  - (appData.screen_width>>1)) / (0.95 * scale_factor * 0.5 );       
    double dy = appData.maxDist * (y  - (appData.screen_height * CENTEROFFSET)) / (0.95 * scale_factor * 0.5 );

    *lat = 180.0f * dy / (6371.0 * M_PI) + appData.centerLat;
    *lon = 180.0 * dx / (cos(((*lat + appData.centerLat)/2.0f) * M_PI / 180.0f) * 6371.0 * M_PI) + appData.centerLon;
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
            continue;
        }

        if(outOfBounds(prevX,prevY)) {
            continue;
        }

        double age = pow(1.0 - (double)(now - oldSeen[currentIdx]) / TRAIL_TTL, 2.2);

        if(age < 0) {
            age = 0;
        }

        uint8_t colorVal = (uint8_t)floor(255.0 * age);
                   
        thickLineRGBA(appData.renderer, prevX, prevY, currentX, currentY, 2 * appData.screen_uiscale, colorVal, colorVal, colorVal, 64);                    

        //tick marks

        double vec[3];
        vec[0] = sin(oldHeading[currentIdx] * M_PI / 180);
        vec[1] = -cos(oldHeading[currentIdx] * M_PI / 180);
        vec[2] = 0;

        double up[] = {0,0,1};

        double out[3];

        CROSSVP(out,vec,up);


        int x1, y1, x2, y2;

        int cross_size = 5 * appData.screen_uiscale;

        //forward cross
        x1 = currentX + round(-cross_size*vec[0]);
        y1 = currentY + round(-cross_size*vec[1]);
        x2 = currentX + round(cross_size*vec[0]);
        y2 = currentY + round(cross_size*vec[1]);

        thickLineRGBA(appData.renderer,x1,y1,x2,y2,appData.screen_uiscale,colorVal,colorVal,colorVal,127);
   
        //side cross
        x1 = currentX + round(-cross_size*out[0]);
        y1 = currentY + round(-cross_size*out[1]);
        x2 = currentX + round(cross_size*out[0]);
        y2 = currentY + round(cross_size*out[1]);
        
        thickLineRGBA(appData.renderer,x1,y1,x2,y2,appData.screen_uiscale,colorVal,colorVal,colorVal,127);
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

void drawPolys(QuadTree *tree, double screen_lat_min, double screen_lat_max, double screen_lon_min, double screen_lon_max) {

    if(tree == NULL) {
        return;
    }

    double dx, dy;
    int x, y;

    if (!(tree->lat_min < screen_lat_min &&
        tree->lat_max > screen_lat_max &&
        tree->lon_min < screen_lon_min &&
        tree->lon_max > screen_lon_max)) {
        return;
    }


    Polygon *currentPolygon = tree->polygons;

    while(currentPolygon != NULL) {
        // Sint16 *px = (Sint16*)malloc(sizeof(Sint16*)*currentPolygon->numPoints);
        // Sint16 *py = (Sint16*)malloc(sizeof(Sint16*)*currentPolygon->numPoints);

        // for(int i=0; i<currentPolygon->numPoints; i++) {

        //     pxFromLonLat(&dx, &dy, currentPolygon->points[i].lat, currentPolygon->points[i].lon); 
        //     screenCoords(&x, &y, dx, dy);

        //     px[i] = x;
        //     py[i] = y;
        // }

        // double alpha = 1.0;

        pxFromLonLat(&dx, &dy, currentPolygon->lat_min, currentPolygon->lon_min); 
        screenCoords(&x, &y, dx, dy);

        int top = y;
        int left = x;

        pxFromLonLat(&dx, &dy, currentPolygon->lat_max, currentPolygon->lon_max); 
        screenCoords(&x, &y, dx, dy);

        int bottom = y;
        int right = x;
  
        //polygonRGBA (appData.renderer, px, py, currentPolygon->numPoints, alpha * purple.r + (1.0-alpha) * blue.r, alpha * purple.g + (1.0-alpha) * blue.g, alpha * purple.b + (1.0-alpha) * blue.b, 255 * alpha);      
        
        rectangleRGBA(appData.renderer, left, top, right, bottom,  purple.r, purple.g, purple.b, 255);      
        

        currentPolygon = currentPolygon->next;
    }

    //drawPolys(tree->nw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    //drawPolys(tree->sw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    //drawPolys(tree->ne, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    //drawPolys(tree->se, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
}

void drawGeography() {

    double screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max;

    latLonFromScreenCoords(&screen_lon_min, &screen_lat_min, 0, 0);
    latLonFromScreenCoords(&screen_lon_max, &screen_lat_max, appData.screen_width, appData.screen_height);

    drawPolys(&root, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);


    return; 
    int x1, y1, x2, y2;

    int skip = (int)(appData.maxDist / 25.0f);
    if(skip < 1) {
        skip = 1;
    }

    for(int i=skip; i<mapPoints_count/2; i+=skip) {

        double dx, dy;

        dx = 1;
        dy = 1;
        for(int j = 0; j < skip; j++) {
            if(!mapPoints[(i - skip + j) * 2]) {
                dx = 0;
            }

            if(!mapPoints[(i - skip + j) * 2 + 1]) {
                dy = 0;
            }
        }

        if(!dx || !dy) {
            continue;
        }

        pxFromLonLat(&dx, &dy, mapPoints[(i - skip) * 2], mapPoints[(i - skip) * 2 + 1]);   

        if(!dx || !dy) {
            continue;
        }

        screenCoords(&x1, &y1, dx, dy);

        if(outOfBounds(x1,y1)) {
            continue;
        }

        double d1 = sqrt(dx * dx + dy * dy);

        pxFromLonLat(&dx, &dy, mapPoints[i * 2], mapPoints[i * 2 + 1]);   
        
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
        thickLineRGBA(appData.renderer, x1, y1, x2, y2, appData.screen_uiscale, alpha * purple.r + (1.0-alpha) * blue.r, alpha * purple.g + (1.0-alpha) * blue.g, alpha * purple.b + (1.0-alpha) * blue.b, 255 * alpha);
        
    }
}

void drawSignalMarks(struct planeObj *p, int x, int y) {
    unsigned char * pSig       = p->signalLevel;
    unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
                                              pSig[4] + pSig[5] + pSig[6] + pSig[7] + 3) >> 3; 

    SDL_Color barColor = signalToColor(signalAverage);

    Uint8 seenFade = (Uint8) (255.0 - (mstime() - p->msSeen) / 4.0);

    circleRGBA(appData.renderer, x + appData.mapFontWidth, y - 5, 2 * appData.screen_uiscale, barColor.r, barColor.g, barColor.b, seenFade);

    seenFade = (Uint8) (255.0 - (mstime() - p->msSeenLatLon) / 4.0);

    hlineRGBA(appData.renderer, x + appData.mapFontWidth + 5 * appData.screen_uiscale, x + appData.mapFontWidth + 9 * appData.screen_uiscale, y - 5, barColor.r, barColor.g, barColor.b, seenFade);
    vlineRGBA(appData.renderer, x + appData.mapFontWidth + 7 * appData.screen_uiscale, y - 2 * appData.screen_uiscale - 5, y + 2 * appData.screen_uiscale - 5, barColor.r, barColor.g, barColor.b, seenFade);
}


void drawPlaneText(struct planeObj *p) {
    int maxCharCount = 0;
    int currentCharCount;

    int currentLine = 0;

    if(p->pressure * appData.screen_width< 0.4f) {
        drawSignalMarks(p, p->x, p->y);

        char flight[10] = " ";
        maxCharCount = snprintf(flight,10," %s", p->flight);

        if(maxCharCount > 0) {
            drawStringBG(flight, p->x, p->y, appData.mapBoldFont, white, black); 
            //roundedRectangleRGBA(appData.renderer, p->x, p->y, p->x + maxCharCount * appData.mapFontWidth, p->y + appData.mapFontHeight, ROUND_RADIUS, white.r, white.g, white.b, SDL_ALPHA_OPAQUE);
            //drawString(flight, p->x, p->y, appData.mapBoldFont, white); 
            currentLine++;             
        }
    }

  if(p->pressure * appData.screen_width < 0.2f) {
        char alt[10] = " ";
        if (Modes.metric) {
            currentCharCount = snprintf(alt,10," %dm", (int) (p->altitude / 3.2828)); 
        } else {
            currentCharCount = snprintf(alt,10," %d'", p->altitude); 
        }

        if(currentCharCount > 0) {
            drawStringBG(alt, p->x, p->y + currentLine * appData.mapFontHeight, appData.mapFont, grey, black);   
            currentLine++;                              
        }

        if(currentCharCount > maxCharCount) {
            maxCharCount = currentCharCount;
        }   

        char speed[10] = " ";
        if (Modes.metric) {
            currentCharCount = snprintf(speed,10," %dkm/h", (int) (p->speed * 1.852));
        } else {
            currentCharCount = snprintf(speed,10," %dmph", p->speed);
        }

        if(currentCharCount > 0) {
            drawStringBG(speed, p->x, p->y + currentLine * appData.mapFontHeight, appData.mapFont, grey, black);  
            currentLine++;               
        }

        if(currentCharCount > maxCharCount) {
            maxCharCount = currentCharCount;
        }
    }

    if(maxCharCount > 0) {

        Sint16 vx[4] = {p->cx, p->cx + (p->x - p->cx) / 2, p->x, p->x};
        Sint16 vy[4] = {p->cy, p->cy + (p->y - p->cy) / 2, p->y - appData.mapFontHeight, p->y};
        
        if(p->cy > p->y + currentLine * appData.mapFontHeight) {
            vy[2] = p->y + currentLine * appData.mapFontHeight + appData.mapFontHeight;
            vy[3] = p->y + currentLine * appData.mapFontHeight;
        } 

        bezierRGBA(appData.renderer,vx,vy,4,2,200,200,200,SDL_ALPHA_OPAQUE);


        thickLineRGBA(appData.renderer,p->x,p->y,p->x,p->y+currentLine*appData.mapFontHeight,appData.screen_uiscale,200,200,200,SDL_ALPHA_OPAQUE);
    }
    
    p->w = maxCharCount * appData.mapFontWidth;
    p->h = currentLine * appData.mapFontHeight;         
}

float sign(float x) {
    return (x > 0) - (x < 0);
}


void resolveLabelConflicts() {
    struct planeObj *p = planes;

    while(p) {

        struct planeObj *check_p = planes;

        int p_left = p->x - 10 * appData.screen_uiscale;
        int p_right = p->x + p->w + 10 * appData.screen_uiscale;
        int p_top = p->y - 10 * appData.screen_uiscale;
        int p_bottom = p->y + p->h + 10 * appData.screen_uiscale;

        //debug box 
        //rectangleRGBA(appData.renderer, p->x, p->y, p->x + p->w, p->y + p->h, 255,0,0, SDL_ALPHA_OPAQUE);
        //lineRGBA(appData.renderer, p->cx, p->cy, p->x, p->y, 0,255,0, SDL_ALPHA_OPAQUE);

        //apply damping

        p->ddox -= 0.07f * p->dox;
        p->ddoy -= 0.07f * p->doy;

        //spring back to origin
        p->ddox -= 0.005f * p->ox;
        p->ddoy -= 0.005f * p->oy;
        
        // // //screen edge 

        if(p_left < 10 * appData.screen_uiscale) {
            p->ox += (float)(10 * appData.screen_uiscale - p_left);
        }

        if(p_right > (appData.screen_width - 10 * appData.screen_uiscale)) {
            p->ox -= (float)(p_right - (appData.screen_width - 10 * appData.screen_uiscale));
        }

        if(p_top < 10 * appData.screen_uiscale) {
            p->oy += (float)(10 * appData.screen_uiscale - p_top);
        }

        if(p_bottom > (appData.screen_height - 10 * appData.screen_uiscale)) {
            p->oy -= (float)(p_bottom - (appData.screen_height - 10 * appData.screen_uiscale));
        }

        p->pressure = 0;


        //check against other labels
        while(check_p) {
            if(check_p->addr != p->addr) {

                int check_left = check_p->x - 5 * appData.screen_uiscale;
                int check_right = check_p->x + check_p->w + 5 * appData.screen_uiscale;
                int check_top = check_p->y - 5 * appData.screen_uiscale; 
                int check_bottom = check_p->y + check_p->h + 5 * appData.screen_uiscale;


                p->pressure += 1.0f / ((check_p->cx - p->cx) * (check_p->cx - p->cx) + (check_p->cy - p->cy) * (check_p->cy - p->cy));


                //if(check_left > (p_right + 10) || check_right < (p_left - 10)) {
                if(check_left > p_right || check_right < p_left) {
                    check_p = check_p -> next;
                    continue;
                }

                //if(check_top > (p_bottom + 10) || check_bottom < (p_top - 10)) {
                if(check_top > p_bottom || check_bottom < p_top) {
                    check_p = check_p -> next;
                    continue;
                }

                //left collision
                if(check_left > p_left && check_left < p_right) {
                    check_p->ddox -= 0.01f * (float)(check_left - p_right);   
                }

                //right collision
                if(check_right > p_left && check_right < p_right) {
                    check_p->ddox -= 0.01f * (float)(check_right - p_left);   
                }

                //top collision
                if(check_top > p_top && check_top < p_bottom) {
                    check_p->ddoy -= 0.01f * (float)(check_top - p_bottom);   
                }

                //bottom collision
                if(check_bottom > p_top && check_bottom < p_bottom) {
                    check_p->ddoy -= 0.01f * (float)(check_bottom - p_top);   
                }    
            }
            check_p = check_p -> next;
        }

        check_p = planes;

        //check against plane icons (include self)

        p_left = p->x - 5 * appData.screen_uiscale;
        p_right = p->x + 5 * appData.screen_uiscale;
        p_top = p->y - 5 * appData.screen_uiscale;
        p_bottom = p->y + 5 * appData.screen_uiscale;

        while(check_p) {

            int check_left = check_p->x - 5 * appData.screen_uiscale;
            int check_right = check_p->x + check_p->w + 5 * appData.screen_uiscale;
            int check_top = check_p->y - 5 * appData.screen_uiscale; 
            int check_bottom = check_p->y + check_p->h + 5 * appData.screen_uiscale;

            if(check_left > p_right || check_right < p_left) {
                check_p = check_p -> next;
                continue;
            }

            if(check_top > p_bottom || check_bottom < p_top) {
                check_p = check_p -> next;
                continue;
            }

            //left collision
            if(check_left > p_left && check_left < p_right) {
                check_p->ddox -= 0.04f * (float)(check_left - p_right);   
            }

            //right collision
            if(check_right > p_left && check_right < p_right) {
                check_p->ddox -= 0.04f * (float)(check_right - p_left);   
            }

            //top collision
            if(check_top > p_top && check_top < p_bottom) {
                check_p->ddoy -= 0.04f * (float)(check_top - p_bottom);   
            }

            //bottom collision
            if(check_bottom > p_top && check_bottom < p_bottom) {
                check_p->ddoy -= 0.04f * (float)(check_bottom - p_top);   
            }            
        
            check_p = check_p -> next;
        }

        p = p->next;
    }

    //update 

    p = planes;

    while(p) {
            //incorporate accelerate from label conflict resolution
      
        p->dox += p->ddox;
        p->doy += p->ddoy;

        if(fabs(p->dox) > 10.0f) {
            p->dox = sign(p->dox) * 10.0f;
        }

        if(fabs(p->doy) > 10.0f) {
            p->doy = sign(p->doy) * 10.0f;
        }

        if(fabs(p->dox) < 1.0f) {
            p->dox = 0;
        }

        if(fabs(p->doy) < 1.0f) {
            p->doy = 0;
        }

        p->ox += p->dox;
        p->oy += p->doy;

        //printf("p_ox: %f, p_oy %f\n",p->ox, p->oy);

        p->ddox = 0;
        p->ddoy = 0;

        p->x = p->cx + (int)round(p->ox);
        p->y = p->cy + (int)round(p->oy);

        p = p->next;
    }
}


void drawMap() {
    struct planeObj *p = planes;
    time_t now = time(NULL);
    SDL_Color planeColor;
    drawGeography();

    drawGrid();     

    for(int i = 0; i < 4; i++) {
        resolveLabelConflicts();
    }

    //draw all trails first so they don't cover up planes and text

    while(p) {
        if ((now - p->seen) < Modes.interactive_display_ttl) {
            drawTrail(p->oldLon, p->oldLat, p->oldHeading, p->oldSeen, p->oldIdx);
        }

        p = p->next;
    }

    p = planes;

    while(p) {
        if ((now - p->seen) < Modes.interactive_display_ttl) {
            if (p->lon && p->lat) {


                int x, y;

                double dx, dy;
                pxFromLonLat(&dx, &dy, p->lon, p->lat);
                screenCoords(&x, &y, dx, dy);

                if((int)(now - p->seen) > DISPLAY_ACTIVE) {
                    planeColor = grey;
                } else {
                    planeColor = green;
                    //srand(p->addr);
                    // planeColor = hsv2SDLColor(255.0 * (double)rand()/(double)RAND_MAX, 255.0, 200.0);
                    //planeColor = signalToColor((int)(255.0f * (float)rand()/(float)RAND_MAX));
                    //fprintf("%d %d %d\n", planeColor.r, planeColor.g, planeColor.b);
                }

                if(p->created == 0) {
                    p->created = mstime();
                }

                double age_ms = (double)(mstime() - p->created);
                if(age_ms < 500) {
                    circleRGBA(appData.renderer, x, y, 500 - age_ms, 255,255, 255, (uint8_t)(255.0 * age_ms / 500.0));   
                } else {
                    if(MODES_ACFLAGS_HEADING_VALID) {
                        int usex = x;
                        int usey = y;

                        if(p->seenLatLon > p->oldSeen[p->oldIdx]) {
                            int oldx, oldy;
                            int idx = (p->oldIdx - 1) % TRAIL_LENGTH;

                            pxFromLonLat(&dx, &dy, p->oldLon[idx], p->oldLat[idx]);
                            screenCoords(&oldx, &oldy, dx, dy);

                            double velx = (x - oldx) / (1000.0 * (p->seenLatLon - p->oldSeen[idx]));
                            double vely = (y - oldy) / (1000.0 * (p->seenLatLon - p->oldSeen[idx]));

                            usex = x + (mstime() - p->msSeenLatLon) * velx;
                            usey = y + (mstime() - p->msSeenLatLon) * vely;
                        } 


                        if(outOfBounds(x,y)) {
                            drawPlaneOffMap(x, y, &(p->cx), &(p->cy), planeColor);

                            //lineRGBA(appData.renderer, p->cx, p->cy, p->x+(p->w/2), p->y, 200,200,200,SDL_ALPHA_OPAQUE);
                            
                        } else {
                            drawPlaneHeading(usex, usey, p->track, planeColor);

                            p->cx = usex;// + 5;
                            p->cy = usey;// + 10 * appData.screen_uiscale;
                    
                            //lineRGBA(appData.renderer, usex, usey, p->x+(p->w/2), p->y, 200,200,200, SDL_ALPHA_OPAQUE);
                            
                        }
                          

                        drawPlaneText(p);

                        
                    } else {
                        //drawPlane(x, y, planeColor);
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