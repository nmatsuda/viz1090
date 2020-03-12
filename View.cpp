#include "dump1090.h"
#include "structs.h"
#include "SDL2/SDL2_rotozoom.h"
#include "SDL2/SDL2_gfxPrimitives.h"
#include "mapdata.h"
//color schemes
#include "parula.h"
#include "monokai.h"

#include "View.h"

static uint64_t mstime(void) {
    struct timeval tv;
    uint64_t mst;

    gettimeofday(&tv, NULL);
    mst = ((uint64_t)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}

static float sign(float x) {
    return (x > 0) - (x < 0);
}

static float clamp(float in, float min, float max) {
    float out = in;

    if(in < min) {
        out = min;
    }

    if(in > max)  {
        out = max;
    }

    return out;
}

static void CROSSVP(float *v, float *u, float *w) 
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

SDL_Color lerpColor(SDL_Color aColor, SDL_Color bColor, float factor) {
    if(factor > 1.0f) {
        factor = 1.0f;
    }

    if(factor < 0.0f) {
        factor = 0.0f;
    }

    SDL_Color out;
    out.r = (1.0f - factor) * aColor.r + factor * bColor.r;
    out.g = (1.0f - factor) * aColor.g + factor * bColor.g;
    out.b = (1.0f - factor) * aColor.b + factor * bColor.b;

    return out;
}

SDL_Color hsv2SDLColor(float h, float s, float v)
{
    float      hh, p, q, t, ff;
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

int View::screenDist(float d) {
    float scale_factor = (appData.screen_width > appData.screen_height) ? appData.screen_width : appData.screen_height;
    return round(0.95 * scale_factor * 0.5 * fabs(d) / appData.maxDist);        
}

void View::pxFromLonLat(float *dx, float *dy, float lon, float lat) {
    if(!lon || !lat) {
        *dx = 0;
        *dy = 0;
        return;
    }

    *dx = 6371.0 * (lon - appData.centerLon) * M_PI / 180.0f * cos(((lat + appData.centerLat)/2.0f) * M_PI / 180.0f);
    *dy = 6371.0 * (lat - appData.centerLat) * M_PI / 180.0f;
}

void View::latLonFromScreenCoords(float *lat, float *lon, int x, int y) {
    float scale_factor = (appData.screen_width > appData.screen_height) ? appData.screen_width : appData.screen_height;

    float dx = appData.maxDist * (x  - (appData.screen_width>>1)) / (0.95 * scale_factor * 0.5 );       
    float dy = appData.maxDist * (y  - (appData.screen_height * CENTEROFFSET)) / (0.95 * scale_factor * 0.5 );

    *lat = 180.0f * dy / (6371.0 * M_PI) + appData.centerLat;
    *lon = 180.0 * dx / (cos(((*lat + appData.centerLat)/2.0f) * M_PI / 180.0f) * 6371.0 * M_PI) + appData.centerLon;
}


void View::screenCoords(int *outX, int *outY, float dx, float dy) {
    *outX = (appData.screen_width>>1) + ((dx>0) ? 1 : -1) * screenDist(dx);    
    *outY = (appData.screen_height * CENTEROFFSET) + ((dy>0) ? -1 : 1) * screenDist(dy);        
}

int View::outOfBounds(int x, int y) {
    if(x < 0 || x >= appData.screen_width || y < 0 || y >= appData.screen_height ) {
        return 1;
    } else {
        return 0;
    }
}

void View::drawPlaneOffMap(int x, int y, int *returnx, int *returny, SDL_Color planeColor) {

    float arrowWidth = 6.0 * appData.screen_uiscale;

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

    float inmag = sqrt(inx *inx + iny*iny);
    float vec[3];
    vec[0] = inx / inmag;
    vec[1] = iny /inmag;
    vec[2] = 0;

    float up[] = {0,0,1};

    float out[3];

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

void View::drawPlaneIcon(int x, int y, float heading, SDL_Color planeColor)
{
    float body = 8.0 * appData.screen_uiscale;
    float wing = 6.0 * appData.screen_uiscale;
    float tail = 3.0 * appData.screen_uiscale;
    float bodyWidth = 2.0 * appData.screen_uiscale;

    float vec[3];
    vec[0] = sin(heading * M_PI / 180);
    vec[1] = -cos(heading * M_PI / 180);
    vec[2] = 0;

    float up[] = {0,0,1};

    float out[3];

    CROSSVP(out,vec,up);

    int x1, x2, y1, y2;

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

void View::drawTrail(Aircraft *p) {
    int currentX, currentY, prevX, prevY;

    std::list<float>::iterator lon_idx = std::next(p->lonHistory.begin());
    std::list<float>::iterator lat_idx = std::next(p->latHistory.begin());
    std::list<float>::iterator heading_idx = std::next(p->headingHistory.begin());

    int idx = p->lonHistory.size();

    for(; lon_idx != p->lonHistory.end(); ++lon_idx, ++lat_idx, ++heading_idx) {

        float dx, dy;

        pxFromLonLat(&dx, &dy, *lon_idx, *lat_idx);

        screenCoords(&currentX, &currentY, dx, dy);

        pxFromLonLat(&dx, &dy, *(std::prev(lon_idx)), *(std::prev(lat_idx)));

        screenCoords(&prevX, &prevY, dx, dy);

        if(outOfBounds(currentX,currentY)) {
            continue;
        }

        if(outOfBounds(prevX,prevY)) {
            continue;
        }

        // float age = pow(1.0 - (float)idx / (float)p->lonHistory.size(), 2.2);
        float age = 1.0 - (float)idx / (float)p->lonHistory.size();

        uint8_t colorVal = (uint8_t)floor(255.0 * clamp(age,0,0.5));
                   
        thickLineRGBA(appData.renderer, prevX, prevY, currentX, currentY, 2 * appData.screen_uiscale, 255, 255, 255, colorVal); 

        //age = pow(1.0 - (float)idx / 10.0f, 2.2);

        //colorVal = (uint8_t)floor(255.0 * clamp(age,0.1,1));

        // float vec[3];
        // vec[0] = sin(*heading_idx * M_PI / 180);
        // vec[1] = -cos(*heading_idx * M_PI / 180);
        // vec[2] = 0;

        // float up[] = {0,0,1};

        // float out[3];

        // CROSSVP(out,vec,up);

        // int x1, y1, x2, y2;

        // int cross_size = 5 * appData.screen_uiscale * age;

        // // //forward cross
        // x1 = currentX + round(-cross_size*vec[0]);
        // y1 = currentY + round(-cross_size*vec[1]);
        // x2 = currentX + round(cross_size*vec[0]);
        // y2 = currentY + round(cross_size*vec[1]);

        // thickLineRGBA(appData.renderer,x1,y1,x2,y2,appData.screen_uiscale,255,255,255,64);
   
        // //side cross
        // x1 = currentX + round(-cross_size*out[0]);
        // y1 = currentY + round(-cross_size*out[1]);
        // x2 = currentX + round(cross_size*out[0]);
        // y2 = currentY + round(cross_size*out[1]);
        
        // thickLineRGBA(appData.renderer,x1,y1,x2,y2,appData.screen_uiscale,255,255,255,64);

        idx--;
    }
}

void View::drawScaleBars()
{
    int scalePower = 0;
    int scaleBarDist = screenDist((float)pow(10,scalePower));

    char scaleLabel[13] = "";
        
    lineRGBA(appData.renderer,10,10,10,10*appData.screen_uiscale,style.scaleBarColor.r, style.scaleBarColor.g, style.scaleBarColor.b, 255);

    while(scaleBarDist < appData.screen_width) {
        lineRGBA(appData.renderer,10+scaleBarDist,8,10+scaleBarDist,16*appData.screen_uiscale,style.scaleBarColor.r, style.scaleBarColor.g, style.scaleBarColor.b, 255);

        if (aircraftData->modes.metric) {
            snprintf(scaleLabel,13,"%dkm", (int)pow(10,scalePower));
        } else {
            snprintf(scaleLabel,13,"%dmi", (int)pow(10,scalePower));
        }

        drawString(scaleLabel, 10+scaleBarDist, 15*appData.screen_uiscale, appData.mapFont, style.scaleBarColor);

        scalePower++;
        scaleBarDist = screenDist((float)pow(10,scalePower));
    }

    scalePower--;
    scaleBarDist = screenDist((float)pow(10,scalePower));

    lineRGBA(appData.renderer,10,10+5*appData.screen_uiscale,10+scaleBarDist,10+5*appData.screen_uiscale, style.scaleBarColor.r, style.scaleBarColor.g, style.  scaleBarColor.b, 255);
}

void View::drawPolys(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max) {
    if(tree == NULL) {
        return;
    }

    if (tree->lat_min > screen_lat_max || screen_lat_min > tree->lat_max) {
        return; 
    }
  
    if (tree->lon_min > screen_lon_max || screen_lon_min > tree->lon_max) {
        return; 
    }

    drawPolys(tree->nw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    drawPolys(tree->sw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    drawPolys(tree->ne, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    drawPolys(tree->se, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);

    float dx, dy;

    Polygon *currentPolygon = tree->polygons;

    while(currentPolygon != NULL) {
        int x1,y1,x2,y2;

        if(currentPolygon->points == NULL)
            continue;

        Point *prevPoint = currentPolygon->points;
        Point *currentPoint = prevPoint->next;

        while(currentPoint != NULL){
            pxFromLonLat(&dx, &dy, prevPoint->lon, prevPoint->lat); 
            screenCoords(&x1, &y1, dx, dy);

            if(outOfBounds(x1,y1)) {
                prevPoint = currentPoint;
                currentPoint = currentPoint->next;
                continue;
            }

            float d1 = dx* dx + dy * dy;

            pxFromLonLat(&dx, &dy, currentPoint->lon, currentPoint->lat); 
            screenCoords(&x2, &y2, dx, dy);

            if(outOfBounds(x2,y2)) {
                prevPoint = currentPoint;
                currentPoint = currentPoint->next;
                continue;
            }


            if((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1) < MIN_MAP_FEATURE){
                currentPoint = currentPoint->next;
                continue;
            }

            float d2 = dx* dx + dy * dy;
    
            float factor = 1.0 - (d1+d2) / (3* appData.maxDist * appData.maxDist);

            SDL_Color lineColor = lerpColor(style.mapOuterColor, style.mapInnerColor, factor); 

            lineRGBA(appData.renderer, x1, y1, x2, y2, lineColor.r, lineColor.g, lineColor.b, 255);

            prevPoint = currentPoint;
            currentPoint = currentPoint->next;
  
        }

        ////bounding boxes

        // pxFromLonLat(&dx, &dy, currentPolygon->lon_min, currentPolygon->lat_min); 
        // screenCoords(&x, &y, dx, dy);

        // int top = y;
        // int left = x;

        // pxFromLonLat(&dx, &dy, currentPolygon->lon_max, currentPolygon->lat_max); 
        // screenCoords(&x, &y, dx, dy);

        // int bottom = y;
        // int right = x;
          
        // rectangleRGBA(appData.renderer, left, top, right, bottom,  purple.r, purple.g, purple.b, 255);      

        currentPolygon = currentPolygon->next;
    }
}

void View::drawGeography() {
    float screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max;

    latLonFromScreenCoords(&screen_lat_min, &screen_lon_min, 0,  appData.screen_height * -0.2);
    latLonFromScreenCoords(&screen_lat_max, &screen_lon_max, appData.screen_width, appData.screen_height * 1.2);

    drawPolys(&(appData.root), screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);    
}

void View::drawSignalMarks(Aircraft *p, int x, int y) {
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


void View::drawPlaneText(Aircraft *p) {
    int maxCharCount = 0;
    int currentCharCount;

    int currentLine = 0;

    if(p->pressure * appData.screen_width< 0.4f) {
        drawSignalMarks(p, p->x, p->y);

        char flight[10] = " ";
        maxCharCount = snprintf(flight,10," %s", p->flight);

        if(maxCharCount > 1) {
            drawStringBG(flight, p->x, p->y, appData.mapBoldFont, white, black); 
            //roundedRectangleRGBA(appData.renderer, p->x, p->y, p->x + maxCharCount * appData.mapFontWidth, p->y + appData.mapFontHeight, ROUND_RADIUS, white.r, white.g, white.b, SDL_ALPHA_OPAQUE);
            //drawString(flight, p->x, p->y, appData.mapBoldFont, white); 
            currentLine++;             
        }
    }

  if(p->pressure * appData.screen_width < 0.2f) {
        char alt[10] = " ";
        if (aircraftData->modes.metric) {
            currentCharCount = snprintf(alt,10," %dm", (int) (p->altitude / 3.2828)); 
        } else {
            currentCharCount = snprintf(alt,10," %d'", p->altitude); 
        }

        if(currentCharCount > 1) {
            drawStringBG(alt, p->x, p->y + currentLine * appData.mapFontHeight, appData.mapFont, grey, black);   
            currentLine++;                              
        }

        if(currentCharCount > maxCharCount) {
            maxCharCount = currentCharCount;
        }   

        char speed[10] = " ";
        if (aircraftData->modes.metric) {
            currentCharCount = snprintf(speed,10," %dkm/h", (int) (p->speed * 1.852));
        } else {
            currentCharCount = snprintf(speed,10," %dmph", p->speed);
        }

        if(currentCharCount > 1) {
            drawStringBG(speed, p->x, p->y + currentLine * appData.mapFontHeight, appData.mapFont, grey, black);  
            currentLine++;               
        }

        if(currentCharCount > maxCharCount) {
            maxCharCount = currentCharCount;
        }
    }

    if(maxCharCount > 1) {

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

void View::drawSelectedAircraftText(Aircraft *p) {
    if(p == NULL) {
        return;
    }

    int x = p->cx - 20;
    int y = p->cy + 22;

    int maxCharCount = 0;
    int currentCharCount;

    int currentLine = 0;

    drawSignalMarks(p, x, y);

    char flight[10] = " ";
    maxCharCount = snprintf(flight,10," %s", p->flight);

    if(maxCharCount > 1) {
        drawStringBG(flight, x, y, appData.mapBoldFont, white, black); 
        //roundedRectangleRGBA(appData.renderer, p->x, p->y, p->x + maxCharCount * appData.mapFontWidth, p->y + appData.mapFontHeight, ROUND_RADIUS, white.r, white.g, white.b, SDL_ALPHA_OPAQUE);
        //drawString(flight, p->x, p->y, appData.mapBoldFont, white); 
        currentLine++;             
    }

    char alt[10] = " ";
    if (aircraftData->modes.metric) {
        currentCharCount = snprintf(alt,10," %dm", (int) (p->altitude / 3.2828)); 
    } else {
        currentCharCount = snprintf(alt,10," %d'", p->altitude); 
    }

    if(currentCharCount > 1) {
        drawStringBG(alt, x, y + currentLine * appData.mapFontHeight, appData.mapFont, grey, black);   
        currentLine++;                              
    }

    if(currentCharCount > maxCharCount) {
        maxCharCount = currentCharCount;
    }   

    char speed[10] = " ";
    if (aircraftData->modes.metric) {
        currentCharCount = snprintf(speed,10," %dkm/h", (int) (p->speed * 1.852));
    } else {
        currentCharCount = snprintf(speed,10," %dmph", p->speed);
    }

    if(currentCharCount > 1) {
        drawStringBG(speed, x, y + currentLine * appData.mapFontHeight, appData.mapFont, grey, black);  
        currentLine++;               
    }
}

void View::resolveLabelConflicts() {
    Aircraft *p = aircraftData->aircraftList.head;

    while(p) {

        Aircraft *check_p = aircraftData->aircraftList.head;

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

        check_p = aircraftData->aircraftList.head;

        //check against plane icons (include self)

        float plane_force = 0.08f;

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
                check_p->ddox -= plane_force * (float)(check_left - p_right);   
            }

            //right collision
            if(check_right > p_left && check_right < p_right) {
                check_p->ddox -= plane_force * (float)(check_right - p_left);   
            }

            //top collision
            if(check_top > p_top && check_top < p_bottom) {
                check_p->ddoy -= plane_force * (float)(check_top - p_bottom);   
            }

            //bottom collision
            if(check_bottom > p_top && check_bottom < p_bottom) {
                check_p->ddoy -= plane_force * (float)(check_bottom - p_top);   
            }            
        
            check_p = check_p -> next;
        }

        p = p->next;
    }

    //update 

    p = aircraftData->aircraftList.head;

    while(p) {
            //incorporate acceleration from label conflict resolution
      
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


void View::drawPlanes() {
    Aircraft *p = aircraftData->aircraftList.head;
    time_t now = time(NULL);
    SDL_Color planeColor;

    // draw all trails first so they don't cover up planes and text
    // also find closest plane to selection point
    while(p) {
        drawTrail(p);
        p = p->next;
    }

    if(aircraftData->selectedAircraft) {
        appData.mapTargetLon = aircraftData->selectedAircraft->lon;
        appData.mapTargetLat = aircraftData->selectedAircraft->lat;             
    }

    p = aircraftData->aircraftList.head;

    while(p) {
        if (p->lon && p->lat) {
            int x, y;

            float dx, dy;
            pxFromLonLat(&dx, &dy, p->lon, p->lat);
            screenCoords(&x, &y, dx, dy);

            if(p->created == 0) {
                p->created = mstime();
            }

            float age_ms = (float)(mstime() - p->created);
            if(age_ms < 500) {
                circleRGBA(appData.renderer, x, y, 500 - age_ms, 255,255, 255, (uint8_t)(255.0 * age_ms / 500.0));   
            } else {
                if(MODES_ACFLAGS_HEADING_VALID) {
                    int usex = x;   
                    int usey = y;

                    if(p->seenLatLon > p->timestampHistory.back()) {
                        int oldx, oldy;

                        pxFromLonLat(&dx, &dy, p->lonHistory.back(), p->latHistory.back());
                        screenCoords(&oldx, &oldy, dx, dy);

                        float velx = (x - oldx) / (1000.0 * (p->seenLatLon - p->timestampHistory.back()));
                        float vely = (y - oldy) / (1000.0 * (p->seenLatLon - p->timestampHistory.back()));

                        usex = x + (mstime() - p->msSeenLatLon) * velx;
                        usey = y + (mstime() - p->msSeenLatLon) * vely;
                    } 

                    if(p == aircraftData->selectedAircraft) {
                        // this logic should be in input, register a callback for click?
                        float elapsed  = mstime() - appData.touchDownTime;

                        int boxSize;
                        if(elapsed < 300) {
                            boxSize = (int)(20.0 * (1.0 - (1.0 - elapsed / 300.0) * cos(sqrt(elapsed)))); 
                        } else {
                            boxSize = 20;
                        }
                        //rectangleRGBA(appData.renderer, usex - boxSize, usey - boxSize, usex + boxSize, usey + boxSize, pink.r, pink.g, pink.b, 255);                            
                        lineRGBA(appData.renderer, usex - boxSize, usey - boxSize, usex - boxSize/2, usey - boxSize, pink.r, pink.g, pink.b, 255);
                        lineRGBA(appData.renderer, usex - boxSize, usey - boxSize, usex - boxSize, usey - boxSize/2, pink.r, pink.g, pink.b, 255);

                        lineRGBA(appData.renderer, usex + boxSize, usey - boxSize, usex + boxSize/2, usey - boxSize, pink.r, pink.g, pink.b, 255);
                        lineRGBA(appData.renderer, usex + boxSize, usey - boxSize, usex + boxSize, usey - boxSize/2, pink.r, pink.g, pink.b, 255);

                        lineRGBA(appData.renderer, usex + boxSize, usey + boxSize, usex + boxSize/2, usey + boxSize, pink.r, pink.g, pink.b, 255);
                        lineRGBA(appData.renderer, usex + boxSize, usey + boxSize, usex + boxSize, usey + boxSize/2, pink.r, pink.g, pink.b, 255);

                        lineRGBA(appData.renderer, usex - boxSize, usey + boxSize, usex - boxSize/2, usey + boxSize, pink.r, pink.g, pink.b, 255);
                        lineRGBA(appData.renderer, usex - boxSize, usey + boxSize, usex - boxSize, usey + boxSize/2, pink.r, pink.g, pink.b, 255);
                        planeColor = lerpColor(style.selectedColor, style.planeGoneColor, (now - p->seen) / (float) DISPLAY_ACTIVE);
                    } else {
                        planeColor = lerpColor(style.planeColor, style.planeGoneColor, (now - p->seen) / (float) DISPLAY_ACTIVE);
                    }

                    if(outOfBounds(x,y)) {
                        drawPlaneOffMap(x, y, &(p->cx), &(p->cy), planeColor);
                    } else {
                        drawPlaneIcon(usex, usey, p->track, planeColor);
                        p->cx = usex;
                        p->cy = usey;
                    }
                      
                    if(p != aircraftData->selectedAircraft) {
                        drawPlaneText(p);
                    }

                }
            }
        }
        p = p->next;
    }

    drawSelectedAircraftText(aircraftData->selectedAircraft);

    if(appData.touchx && appData.touchy) {

        int radius = .25 * (mstime() - appData.touchDownTime);
        int alpha = 128 - (int)(0.5 * (mstime() - appData.touchDownTime));
        if(alpha < 0 ) {
            alpha = 0;
            appData.touchx = 0;
            appData.touchy = 0;
        }

        filledCircleRGBA(appData.renderer, appData.touchx, appData.touchy, radius,  white.r, white.g, white.b, alpha);      

    }    
}

void View::animateCenterAbsolute(float x, float y) {
    float scale_factor = (appData.screen_width > appData.screen_height) ? appData.screen_width : appData.screen_height;

    float dx = -1.0 * (0.75*(double)appData.screen_width / (double)appData.screen_height) * (x - appData.screen_width/2) * appData.maxDist / (0.95 * scale_factor * 0.5);
    float dy = 1.0 * (y - appData.screen_height/2) * appData.maxDist / (0.95 * scale_factor * 0.5);

    float outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

    float outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((appData.centerLat)/2.0f) * M_PI / 180.0f);

    //double outLon, outLat;
    //latLonFromScreenCoords(&outLat, &outLon, event.tfinger.dx, event.tfinger.dy);

    appData.mapTargetLon = appData.centerLon - outLon;
    appData.mapTargetLat = appData.centerLat - outLat;

    appData.mapTargetMaxDist = 0.25 * appData.maxDist;

    appData.mapMoved = 1;
}


void View::moveCenterAbsolute(float x, float y) {
    float scale_factor = (appData.screen_width > appData.screen_height) ? appData.screen_width : appData.screen_height;

    float dx = -1.0 * (0.75*(double)appData.screen_width / (double)appData.screen_height) * (x - appData.screen_width/2) * appData.maxDist / (0.95 * scale_factor * 0.5);
    float dy = 1.0 * (y - appData.screen_height/2) * appData.maxDist / (0.95 * scale_factor * 0.5);

    float outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

    float outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((appData.centerLat)/2.0f) * M_PI / 180.0f);

    //double outLon, outLat;
    //latLonFromScreenCoords(&outLat, &outLon, event.tfinger.dx, event.tfinger.dy);

    appData.centerLon += outLon;
    appData.centerLat += outLat;

    appData.mapTargetLon = 0;
    appData.mapTargetLat = 0;

    appData.mapMoved = 1;
}

void View::moveCenterRelative(float dx, float dy) {
    //
    // need to make lonlat to screen conversion class - this is just the inverse of the stuff in draw.c, without offsets
    //
        
    float scale_factor = (appData.screen_width > appData.screen_height) ? appData.screen_width : appData.screen_height;

    dx = -1.0 * (0.75*(double)appData.screen_width / (double)appData.screen_height) * dx * appData.maxDist / (0.95 * scale_factor * 0.5);
    dy = 1.0 * dy * appData.maxDist / (0.95 * scale_factor * 0.5);

    float outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

    float outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((appData.centerLat)/2.0f) * M_PI / 180.0f);

    //double outLon, outLat;
    //latLonFromScreenCoords(&outLat, &outLon, event.tfinger.dx, event.tfinger.dy);

    appData.centerLon += outLon;
    appData.centerLat += outLat;

    appData.mapTargetLon = 0;
    appData.mapTargetLat = 0;

    appData.mapMoved = 1;
}

void View::zoomMapToTarget() {
    if(appData.mapTargetMaxDist) {
        if(fabs(appData.mapTargetMaxDist - appData.maxDist) > 0.0001) {
            appData.maxDist += 0.1 * (appData.mapTargetMaxDist - appData.maxDist);
        } else {
            appData.mapTargetMaxDist = 0;
        }
    }
}

void View::moveMapToTarget() {
    if(appData.mapTargetLon && appData.mapTargetLat) {
        if(fabs(appData.mapTargetLon - appData.centerLon) > 0.0001 || fabs(appData.mapTargetLat - appData.centerLat) > 0.0001) {
            appData.centerLon += 0.1 * (appData.mapTargetLon- appData.centerLon);
            appData.centerLat += 0.1 * (appData.mapTargetLat - appData.centerLat);

            appData.mapMoved = 1;    
        } else {
            appData.mapTargetLon = 0;
            appData.mapTargetLat = 0;
        }     
    }
}

void View::drawMouse() {
    if(appData.mouseMovedTime == 0  || (mstime() - appData.mouseMovedTime) > 1000) {
        appData.mouseMovedTime = 0;
        return;
    }

    int alpha =  (int)(255.0f - 255.0f * (float)(mstime() - appData.mouseMovedTime) / 1000.0f);

    lineRGBA(appData.renderer, appData.mousex - 10 * appData.screen_uiscale, appData.mousey,  appData.mousex + 10 * appData.screen_uiscale, appData.mousey, white.r, white.g, white.b, alpha);
    lineRGBA(appData.renderer, appData.mousex,  appData.mousey - 10 * appData.screen_uiscale,  appData.mousex,  appData.mousey + 10 * appData.screen_uiscale, white.r, white.g, white.b, alpha);
}

void View::registerClick() {
    if(appData.tapCount == 1 && appData.isDragging  == 0) {
        Aircraft *p = aircraftData->aircraftList.head;
        Aircraft *selection = NULL;

        while(p) {
            if(appData.touchx && appData.touchy) {
                if((p->cx - appData.touchx) * (p->cx - appData.touchx) + (p->cy - appData.touchy) * (p->cy - appData.touchy) < 900) {
                    if(selection) {
                        if((p->cx - appData.touchx) * (p->cx - appData.touchx) + (p->cy - appData.touchy) * (p->cy - appData.touchy) < 
                            (selection->cx - appData.touchx) * (selection->cx - appData.touchx) + (selection->cy - appData.touchy) * (selection->cy - appData.touchy)) {
                            selection = p;
                        }
                    } else {
                        selection = p;
                    }    
                }
            }

            p = p->next;
        }

        //if(aircraftData->selectedAircraft == NULL) {
            aircraftData->selectedAircraft = selection;
        //}
    } else if(appData.tapCount == 2) {
        appData.mapTargetMaxDist = 0.25 * appData.maxDist;
        animateCenterAbsolute(appData.touchx, appData.touchy);
    }
}

//
// 
//

void View::draw() {
    uint64_t drawStartTime = mstime();
    
    moveMapToTarget();
    zoomMapToTarget();

    //updatePlanes();

    updateStatus();

    if(appData.mapMoved) {
        SDL_SetRenderTarget(appData.renderer, appData.mapTexture);
        
        SDL_SetRenderDrawColor(appData.renderer, style.backgroundColor.r, style.backgroundColor.g, style.backgroundColor.b, 255);

        SDL_RenderClear(appData.renderer);
        
        drawGeography();

        drawScaleBars();

        SDL_SetRenderTarget(appData.renderer, NULL );   

        appData.mapMoved = 0; 
    }
    
    for(int i = 0; i < 4; i++) {
        resolveLabelConflicts();
    }

    //SDL_SetRenderDrawColor( appData.renderer, 0, 15, 30, 0);
    SDL_SetRenderDrawColor(appData.renderer, style.backgroundColor.r, style.backgroundColor.g, style.backgroundColor.b, 255);

    SDL_RenderClear(appData.renderer);

    SDL_RenderCopy(appData.renderer, appData.mapTexture, NULL, NULL);

    drawPlanes();  
    drawStatus();
    drawMouse();

    char fps[13] = " ";
    snprintf(fps,13," %.1ffps", 1000.0 / (mstime() - appData.lastFrameTime));
    drawStringBG(fps, 0,0, appData.mapFont, grey, black);  

    SDL_RenderPresent(appData.renderer);  

    appData.lastFrameTime = mstime(); 

    if ((mstime() - drawStartTime) < FRAMETIME) {
        usleep(1000 * (FRAMETIME - (mstime() - drawStartTime)));
    } 
}

View::View(AircraftData *aircraftData){
    this->aircraftData = aircraftData;
}