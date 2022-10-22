// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// Copyright (C) 2014, Malcolm Robb <Support@ATTAvionics.com>
// Copyright (C) 2012, Salvatore Sanfilippo <antirez at gmail dot com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "SDL2/SDL2_rotozoom.h"
#include "SDL2/SDL2_gfxPrimitives.h"

#include "View.h"

#include "AircraftLabel.h"

#include <iostream>
#include <thread>

using fmilliseconds = std::chrono::duration<float, std::milli>;
using fseconds = std::chrono::duration<float>;

static std::chrono::high_resolution_clock::time_point now() {
    return std::chrono::high_resolution_clock::now();
}

static float elapsed(std::chrono::high_resolution_clock::time_point ref) {
            return (fmilliseconds {now() - ref}).count();
}

static  float elapsed_s(std::chrono::high_resolution_clock::time_point ref) {
    return  (fseconds {    now() - ref}).count();
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

float lerp(float a, float b, float factor) {
    if(factor > 1.0f) {
        factor = 1.0f;
    }

    if(factor < 0.0f) {
        factor = 0.0f;
    }

    return (1.0f - factor) * a + factor * b;
}

float lerpAngle(float a, float b, float factor) {
    float diff = fabs(b - a);
    if (diff > 180.0f)
    {
        if (b > a)
        {
            a += 360.0f;
        }
        else
        {
            b += 360.0f;
        }
    }

    float value = (a + ((b - a) * factor));

    if (value >= 0.0f && value <= 360.0f)
        return value;

    return fmod(value,360.0f);
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

int View::screenDist(float d) {
    float scale_factor = (screen_width > screen_height) ? screen_width : screen_height;
    // return round(0.95 * scale_factor * 0.5 * fabs(d) / maxDist);        
    return round(scale_factor * 0.5 * fabs(d) / maxDist);        
}

void View::pxFromLonLat(float *dx, float *dy, float lon, float lat) {
    if(!lon || !lat) {
        *dx = 0;
        *dy = 0;
        return;
    }

    // for accurate reprojection use the extra cos term
    *dx = LATLONMULT * (lon - centerLon) * cos(((lat + centerLat)/2.0f) * M_PI / 180.0f);
    *dy = LATLONMULT * (lat - centerLat);
}

void View::latLonFromScreenCoords(float *lat, float *lon, int x, int y) {
    float scale_factor = (screen_width > screen_height) ? screen_width : screen_height;

    float dx = maxDist * (x  - (screen_width>>1)) / (0.95 * scale_factor * 0.5 );       
    float dy = maxDist * (y  - (screen_height>>1)) / (0.95 * scale_factor * 0.5 );

    *lat = 180.0f * dy / (6371.0 * M_PI) + centerLat;
    *lon = 180.0 * dx / (cos(((*lat + centerLat)/2.0f) * M_PI / 180.0f) * 6371.0 * M_PI) + centerLon;
}


void View::screenCoords(int *outX, int *outY, float dx, float dy) {
    *outX = (screen_width>>1) + ((dx>0) ? 1 : -1) * screenDist(dx);    
    *outY = (screen_height>>1) + ((dy>0) ? -1 : 1) * screenDist(dy);        
}

int View::outOfBounds(int x, int y) {
    return outOfBounds(x, y, 0, 0, screen_width, screen_height);
}

int View::outOfBounds(int x, int y, int left, int top, int right, int bottom) {
    if(x < left || x >= right || y < top || y >= bottom ) {
        return 1;
    } else {
        return 0;
    }
}

//
// Fonts should probably go in Style
//

TTF_Font* View::loadFont(const char *name, int size)
{
    TTF_Font *font = TTF_OpenFont(name, size);

    if (font == NULL)
    {
        printf("Failed to open Font %s: %s\n", name, TTF_GetError());

        exit(1);
    }

    return font;
}

void View::closeFont(TTF_Font *font)
{   
    if (font != NULL)
    {
        TTF_CloseFont(font);
    }
}


void View::font_init() {
    mapFont = loadFont("font/TerminusTTF-4.46.0.ttf", 12 * screen_uiscale);
    mapBoldFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * screen_uiscale);    
       
    listFont = loadFont("font/TerminusTTF-4.46.0.ttf", 12 * screen_uiscale);

    messageFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * screen_uiscale);
    labelFont = loadFont("font/TerminusTTF-Bold-4.46.0.ttf", 12 * screen_uiscale);

    mapFontWidth = 5 * screen_uiscale;
    mapFontHeight = 12 * screen_uiscale; 

    messageFontWidth = 6 * screen_uiscale;
    messageFontHeight = 12 * screen_uiscale; 

    labelFontWidth = 6 * screen_uiscale;
    labelFontHeight = 12 * screen_uiscale; 
}



//
// SDL Utils
//

void View::SDL_init() {
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

    if(fullscreen) {
        flags = flags | SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    if(screen_width == 0) {
        SDL_DisplayMode DM;
        SDL_GetCurrentDisplayMode(0, &DM);
        screen_width = DM.w;
        screen_height= DM.h;
    }

    window =  SDL_CreateWindow("viz1090",  SDL_WINDOWPOS_CENTERED_DISPLAY(screen_index),  SDL_WINDOWPOS_CENTERED_DISPLAY(screen_index), screen_width, screen_height, flags);        
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    mapTexture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_TARGET,
                               screen_width, screen_height);

    mapMoved = 1;
    mapTargetLon = 0;
    mapTargetLat = 0;
    mapTargetMaxDist = 0;

    if(fullscreen) {
        //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
        SDL_RenderSetLogicalSize(renderer, screen_width, screen_height);
    }
}


//
// Status boxes -> move to separate class 
//

void View::drawStatusBox(int *left, int *top, std::string label, std::string message, SDL_Color color) {
    int labelWidth = (label.length() + ((label.length() > 0 ) ? 1 : 0)) * labelFontWidth;
    int messageWidth = (message.length() + ((message.length() > 0 ) ? 1 : 0)) * messageFontWidth;

    if(*left + labelWidth + messageWidth + PAD > screen_width) {
        *left = PAD;
        *top = *top - messageFontHeight - PAD;
    }

    // filled black background
    if(messageWidth) {
        roundedBoxRGBA(renderer, *left, *top, *left + labelWidth + messageWidth, *top + messageFontHeight, ROUND_RADIUS, style.buttonBackground.r, style.buttonBackground.g, style.buttonBackground.b, SDL_ALPHA_OPAQUE);
    }

    // filled label box
    if(labelWidth) {
        roundedBoxRGBA(renderer, *left, *top, *left + labelWidth, *top + messageFontHeight, ROUND_RADIUS,color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
    }

    // outline message box
    if(messageWidth) {
        roundedRectangleRGBA(renderer, *left, *top, *left + labelWidth + messageWidth, *top + messageFontHeight, ROUND_RADIUS,color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
    }

    Label currentLabel;
    currentLabel.setFont(labelFont);
    currentLabel.setColor(style.buttonBackground);
    currentLabel.setPosition(*left + labelFontWidth/2, *top);
    currentLabel.setText(label);
    currentLabel.draw(renderer);
    
    currentLabel.setFont(messageFont);
    currentLabel.setColor(color);
    currentLabel.setPosition(*left + labelWidth + messageFontWidth/2, *top);
    currentLabel.setText(message);
    currentLabel.draw(renderer);

    *left = *left + labelWidth + messageWidth + PAD;
}

void View::drawCenteredStatusBox(std::string label, std::string message, SDL_Color color) {

    int labelWidth = (label.length() + ((label.length() > 0 ) ? 1 : 0)) * labelFontWidth;
    int messageWidth = (message.length() + ((message.length() > 0 ) ? 1 : 0)) * messageFontWidth;

    int left = (screen_width - (labelWidth + messageWidth)) / 2;
    int top = (screen_height - labelFontHeight) / 2;
	
    drawStatusBox(&left, &top, label, message, color);
}


void View::drawStatus() {

    int left = PAD; 
    int top = screen_height - messageFontHeight - PAD;

    if(fps) {
        char fps[60] = " ";
        snprintf(fps,40,"%.1f", 1000.0 / lastFrameTime);

        drawStatusBox(&left, &top, "fps", fps, style.white);
    }


    if(!appData->connected) {
        drawStatusBox(&left,&top,"init", "connecting", style.white);
    } else {    
        char strLoc[20] = " ";
        snprintf(strLoc, 20, "%3.3fN %3.3f%c", centerLat, fabs(centerLon),(centerLon > 0) ? 'E' : 'W');
        drawStatusBox(&left, &top, "loc", strLoc, style.buttonColor);   

        char strPlaneCount[10] = " ";
        snprintf(strPlaneCount, 10,"%d/%d", appData->numVisiblePlanes, appData->numPlanes);
        drawStatusBox(&left, &top, "disp", strPlaneCount, style.buttonColor);

        char strMsgRate[18] = " ";
        snprintf(strMsgRate, 18,"%.0f/s", appData->msgRate);
        drawStatusBox(&left, &top, "rate", strMsgRate, style.buttonColor);

        char strSig[18] = " ";
        snprintf(strSig, 18, "%.0f%%", 100.0 * appData->avgSig / 1024.0);
        drawStatusBox(&left, &top, "sAvg", strSig, style.buttonColor);
    }

    if(map.loaded < 100) {
	char loaded[20] = " ";
        snprintf(loaded, 20, "loading map %d%%", map.loaded);	
        drawStatusBox(&left,&top,"init", loaded, style.white);
    }
}

//
// Main drawing
//

void View::drawPlaneOffMap(int x, int y, int *returnx, int *returny, SDL_Color planeColor) {

    float arrowWidth = 6.0 * screen_uiscale;

    float inx = x - (screen_width>>1);
    float iny = y - (screen_height>>1);
    
    float outx, outy;
    outx = inx;
    outy = iny;

    if(abs(inx) > abs(y - (screen_height>>1)) *  static_cast<float>(screen_width>>1) / static_cast<float>(screen_height>>1)) { //left / right quadrants
        outx = (screen_width>>1) * ((inx > 0) ? 1.0 : -1.0);
        outy = (outx) * iny / (inx);
    } else { // up / down quadrants
        outy = screen_height * ((iny > 0) ? 0.5 : -0.5 );
        outx = (outy) * inx / (iny);
    }

    // circleRGBA (renderer,(screen_width>>1) + outx, screen_height * CENTEROFFSET + outy,50,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    // thickLineRGBA(renderer,screen_width>>1,screen_height * CENTEROFFSET, (screen_width>>1) + outx, screen_height * CENTEROFFSET + outy,arrowWidth,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

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
    x1 = (screen_width>>1) + outx - 2.0 * arrowWidth * vec[0] + round(-arrowWidth*out[0]);
    y1 = (screen_height>>1) + outy - 2.0 * arrowWidth * vec[1] + round(-arrowWidth*out[1]);
    x2 = (screen_width>>1) + outx - 2.0 * arrowWidth * vec[0] + round(arrowWidth*out[0]);
    y2 = (screen_height>>1) + outy - 2.0 * arrowWidth * vec[1] + round(arrowWidth*out[1]);
    x3 = (screen_width>>1) +  outx - arrowWidth * vec[0];
    y3 = (screen_height>>1) + outy - arrowWidth * vec[1];
    trigonRGBA(renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    // arrow 2
    x1 = (screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(-arrowWidth*out[0]);
    y1 = (screen_height>>1) + outy - 3.0 * arrowWidth * vec[1] + round(-arrowWidth*out[1]);
    x2 = (screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(arrowWidth*out[0]);
    y2 = (screen_height>>1) + outy - 3.0 * arrowWidth * vec[1] + round(arrowWidth*out[1]);
    x3 = (screen_width>>1) +  outx - 2.0 * arrowWidth * vec[0];
    y3 = (screen_height>>1) + outy - 2.0 * arrowWidth * vec[1];
    trigonRGBA(renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    *returnx = x3;
    *returny = y3;
}

void View::drawPlaneIcon(int x, int y, float heading, SDL_Color planeColor)
{
    float body = 8.0 * screen_uiscale;
    float wing = 6.0 * screen_uiscale;
    float wingThick = 0.5;
    float tail = 3.0 * screen_uiscale;
    float tailThick = 0.35;
    float bodyWidth = screen_uiscale;

    float vec[3];
    vec[0] = sin(heading * M_PI / 180);
    vec[1] = -cos(heading * M_PI / 180);
    vec[2] = 0;

    float up[] = {0,0,1};

    float out[3];

    CROSSVP(out,vec,up);

    int x1, x2, y1, y2;

    //body
    x1 = x + round(-bodyWidth*out[0]);
    y1 = y + round(-bodyWidth*out[1]);
    x2 = x + round(bodyWidth*out[0]);
    y2 = y + round(bodyWidth*out[1]);

    filledTrigonRGBA (renderer, x1, y1, x2, y2, x+round(-body * vec[0]), y+round(-body*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    filledTrigonRGBA (renderer, x1, y1, x2, y2, x+round(body * vec[0]), y+round(body*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //x1 = x + round(8*vec[0]);
    //y1 = y + round(8*vec[1]);
    //x2 = x + round(16*vec[0]);
    //y2 = y + round(16*vec[1]);

    //lineRGBA(renderer,x1,y1,x2,y2, style.white.r, style.white.g, style.white.b, SDL_ALPHA_OPAQUE);

    // x1 = x + round(-body*vec[0] + bodyWidth*out[0]);
    // y1 = y + round(-body*vec[1] + bodyWidth*out[1]);
    // x2 = x + round(body*vec[0] + bodyWidth*out[0]);
    // y2 = y + round(body*vec[1] + bodyWidth*out[1]);

    // lineRGBA(renderer,x,y,x2,y2,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //trigonRGBA(renderer, x + round(-wing*.35*out[0]), y + round(-wing*.35*out[1]), x + round(wing*.35*out[0]), y + round(wing*.35*out[1]), x1, y1,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);        
    // circleRGBA(renderer, x2,y2,screen_uiscale,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //wing
    x1 = x + round(-wing*out[0]);
    y1 = y + round(-wing*out[1]);
    x2 = x + round(wing*out[0]);
    y2 = y + round(wing*out[1]);

    filledTrigonRGBA(renderer, x1, y1, x2, y2, x+round(body*wingThick*vec[0]), y+round(body*wingThick*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //tail
    x1 = x + round(-body*.75*vec[0] - tail*out[0]);
    y1 = y + round(-body*.75*vec[1] - tail*out[1]);
    x2 = x + round(-body*.75*vec[0] + tail*out[0]);
    y2 = y + round(-body*.75*vec[1] + tail*out[1]);

    filledTrigonRGBA (renderer, x1, y1, x2, y2, x+round(-body*tailThick*vec[0]), y+round(-body*tailThick*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
}

void View::drawTrails(int left, int top, int right, int bottom) {
    int currentX, currentY, prevX, prevY;
    float dx, dy;   

    Aircraft *p = appData->aircraftList.head;

    while(p) {
        if (p->lon && p->lat) {
            if(p->lonHistory.empty()) {
                    return;
                }

                std::vector<float>::iterator lon_idx = p->lonHistory.begin();
                std::vector<float>::iterator lat_idx = p->latHistory.begin();
                std::vector<float>::iterator heading_idx = p->headingHistory.begin();

                float age = 0;

                for(; std::next(lon_idx) != p->lonHistory.end(); ++lon_idx, ++lat_idx, ++heading_idx, age += 1.0) {

                    pxFromLonLat(&dx, &dy, *(std::next(lon_idx)), *(std::next(lat_idx)));
                    screenCoords(&currentX, &currentY, dx, dy);

                    pxFromLonLat(&dx, &dy, *lon_idx, *lat_idx);

                    screenCoords(&prevX, &prevY, dx, dy);
                    if(outOfBounds(currentX,currentY,left,top,right,bottom) && outOfBounds(prevX,prevY,left,top,right,bottom)) {
                        continue;
                    }


                    SDL_Color color = lerpColor({255,0,0,255}, {255,200,0,255}, age / static_cast<float>(p->lonHistory.size()));

                    uint8_t colorVal = (uint8_t)floor(127.0 * (age / static_cast<float>(p->lonHistory.size())));
                               
                    //thickLineRGBA(renderer, prevX, prevY, currentX, currentY, 2 * screen_uiscale, 255, 255, 255, colorVal); 
                    lineRGBA(renderer, prevX, prevY, currentX, currentY, color.r, color.g, color.b, colorVal); 

                }
        }
        p = p->next;
    }
}

void View::drawScaleBars()
{
    int scalePower = 0;
    int scaleBarDist = screenDist((float)pow(10,scalePower));

    char scaleLabel[13] = "";
        
    lineRGBA(renderer,10,10,10,10*screen_uiscale,style.scaleBarColor.r, style.scaleBarColor.g, style.scaleBarColor.b, 255);

    while(scaleBarDist < screen_width) {
        lineRGBA(renderer,10+scaleBarDist,8,10+scaleBarDist,16*screen_uiscale,style.scaleBarColor.r, style.scaleBarColor.g, style.scaleBarColor.b, 255);

        if (metric) {
            snprintf(scaleLabel,13,"%dkm", static_cast<int>(pow(10,scalePower)));
        } else {
            snprintf(scaleLabel,13,"%dmi", static_cast<int>(pow(10,scalePower)));
        }

        Label currentLabel;
        currentLabel.setFont(mapFont);
        currentLabel.setColor(style.scaleBarColor);
        currentLabel.setPosition(10+scaleBarDist, 15*screen_uiscale);
        currentLabel.setText(scaleLabel);
        currentLabel.draw(renderer);
        
        scalePower++;
        scaleBarDist = screenDist(powf(10,scalePower));
    }

    scalePower--;
    scaleBarDist = screenDist(powf(10,scalePower));

    lineRGBA(renderer,10,10+5*screen_uiscale,10+scaleBarDist,10+5*screen_uiscale, style.scaleBarColor.r, style.scaleBarColor.g, style.  scaleBarColor.b, 255);
}


void View::drawLines(int left, int top, int right, int bottom, int bailTime) {
    float screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max;

    latLonFromScreenCoords(&screen_lat_min, &screen_lon_min, left, top);
    latLonFromScreenCoords(&screen_lat_max, &screen_lon_max, right, bottom);

    drawLinesRecursive(&(map.root), screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max, style.geoColor);

    drawLinesRecursive(&(map.airport_root), screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max, style.airportColor);

    drawTrails(left, top, right, bottom);
}

void View::drawLinesRecursive(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max, SDL_Color color) {
    

    if(tree == NULL) {
        return;
    }

    if (tree->lat_min > screen_lat_max || screen_lat_min > tree->lat_max) {
        return; 
    }

    if (tree->lon_min > screen_lon_max || screen_lon_min > tree->lon_max) {
        return; 
    }
    
    drawLinesRecursive(tree->nw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max, color);
    
    drawLinesRecursive(tree->sw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max, color);
    
    drawLinesRecursive(tree->ne, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max, color);
    
    drawLinesRecursive(tree->se, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max, color);

    std::vector<Line*>::iterator currentLine;

    for (currentLine = tree->lines.begin(); currentLine != tree->lines.end(); ++currentLine) {
        int x1,y1,x2,y2;
        float dx,dy;
    
        pxFromLonLat(&dx, &dy, (*currentLine)->start.lon, (*currentLine)->start.lat); 
        screenCoords(&x1, &y1, dx, dy);

        pxFromLonLat(&dx, &dy, (*currentLine)->end.lon, (*currentLine)->end.lat); 
        screenCoords(&x2, &y2, dx, dy);

        lineCount++;

        if(outOfBounds(x1,y1) && outOfBounds(x2,y2)) {
            continue;
        }

        if(x1 == x2 && y1 == y2) {
            continue;
        }

        lineRGBA(renderer, x1, y1, x2, y2, color.r, color.g, color.b, 255);     
    }

    // //Debug quadtree
    // int tl_x,tl_y,tr_x,tr_y,bl_x,bl_y,br_x,br_y;
    // float dx,dy;

    // pxFromLonLat(&dx, &dy, tree->lon_min, tree->lat_min); 
    // screenCoords(&tl_x, &tl_y, dx, dy);

    // pxFromLonLat(&dx, &dy, tree->lon_max, tree->lat_min); 
    // screenCoords(&tr_x, &tr_y, dx, dy);

    // pxFromLonLat(&dx, &dy, tree->lon_min, tree->lat_max); 
    // screenCoords(&bl_x, &bl_y, dx, dy);

    // pxFromLonLat(&dx, &dy, tree->lon_max, tree->lat_max); 
    // screenCoords(&br_x, &br_y, dx, dy);    

    // lineRGBA(renderer, tl_x, tl_y, tr_x, tr_y, 50, 50, 50, 255);     
    // lineRGBA(renderer, tr_x, tr_y, br_x, br_y, 50, 50, 50, 255);     
    // lineRGBA(renderer, bl_x, bl_y, br_x, br_y, 50, 50, 50, 255);     
    // lineRGBA(renderer, tl_x, tl_y, bl_x, bl_y, 50, 50, 50, 255);  

    // // pixelRGBA(renderer,tl_x, tl_y,255,0,0,255);
    // pixelRGBA(renderer,tr_x, tr_y,0,255,0,255);
    // pixelRGBA(renderer,bl_x, bl_y,0,0,255,255);
    // pixelRGBA(renderer,br_x, br_y,255,255,0,255);

   
}

void View::drawPlaceNames() {

    //pre-generating labels in map will trade memory for TTF calls - need to compare when there are a lot of labels on screen

    Label currentLabel;    
    currentLabel.setFont(mapFont);
    currentLabel.setColor(style.geoColor);

    for(std::vector<MapLabel*>::iterator label = map.mapnames.begin(); label != map.mapnames.end(); ++label) {
        float dx, dy;
        int x,y;

        pxFromLonLat(&dx, &dy, (*label)->location.lon, (*label)->location.lat);
        screenCoords(&x, &y, dx, dy);

        if(outOfBounds(x,y)) {
            continue;
        }

        currentLabel.setText((*label)->text);
        currentLabel.setPosition(x,y);
        currentLabel.draw(renderer);
    }

    for(std::vector<MapLabel*>::iterator label = map.airportnames.begin(); label != map.airportnames.end(); ++label) {
        float dx, dy;
        int x,y;

        pxFromLonLat(&dx, &dy, (*label)->location.lon, (*label)->location.lat);
        screenCoords(&x, &y, dx, dy);

        if(outOfBounds(x,y)) {
            continue;
        }

        currentLabel.setText((*label)->text);
        currentLabel.setPosition(x,y);
        currentLabel.draw(renderer);  
    }
}

void View::drawGeography() {

    if((mapRedraw && !mapMoved) || (mapAnimating && elapsed(lastRedraw) > 8 * FRAMETIME) ||  elapsed(lastRedraw) > 2000) {

        SDL_SetRenderTarget(renderer, mapTexture);
        
        SDL_SetRenderDrawColor(renderer, style.backgroundColor.r, style.backgroundColor.g, style.backgroundColor.b, 255);

        SDL_RenderClear(renderer);
        
        drawLines(0, 0, screen_width, screen_height, 0);
        drawPlaceNames();

        SDL_SetRenderTarget(renderer, NULL );   

        mapMoved = 0;
        mapRedraw = 0;
        mapAnimating = 0;

        lastRedraw = now();

        currentLon = centerLon;
        currentLat = centerLat;
        currentMaxDist = maxDist;
    }
    
    SDL_SetRenderDrawColor(renderer, style.backgroundColor.r, style.backgroundColor.g, style.backgroundColor.b, 255);

    SDL_RenderClear(renderer);

    int shiftx = 0;
    int shifty = 0;

    if(mapMoved) {
        float dx, dy;
        int x1,y1, x2, y2;
        pxFromLonLat(&dx, &dy, currentLon, currentLat);
        screenCoords(&x1, &y1, dx, dy);
        pxFromLonLat(&dx, &dy, centerLon, centerLat);
        screenCoords(&x2, &y2, dx, dy);

        shiftx = x1-x2; 
        shifty = y1-y2;

        SDL_Rect dest;

        dest.x = shiftx + (screen_width / 2) * (1 - currentMaxDist / maxDist);
        dest.y = shifty + (screen_height / 2) * (1 - currentMaxDist / maxDist);
        dest.w = screen_width * currentMaxDist / maxDist;
        dest.h = screen_height * currentMaxDist / maxDist;

        //left
        if(dest.x > 0) {
           drawLines(0, 0, dest.x, screen_height, FRAMETIME / 4);    
        }        

        //top
        if(dest.y > 0) {
            drawLines(0, screen_height - dest.y, screen_width, screen_height, FRAMETIME / 4);    
        }

        //right
        if(dest.x + dest.w < screen_width) {
           drawLines(dest.x + dest.w, 0, screen_width, screen_height, FRAMETIME / 4);    
        }        

        //bottom
        if(dest.y + dest.h < screen_height) {
            drawLines(0, 0, screen_width, screen_height - dest.y - dest.h, FRAMETIME / 4);    
        }        

        //attempt rest before bailing
        //drawGeography(dest.x, screen_height - dest.y, dest.x + dest.w, screen_height - dest.y - dest.h, 1);    

        SDL_RenderCopy(renderer, mapTexture, NULL, &dest);

        mapRedraw = 1;
        mapMoved = 0;
    } else {
        SDL_RenderCopy(renderer, mapTexture, NULL, NULL);
    }
}

void View::drawPlaneText(Aircraft *p) {
    if(!p->label) {
        p->label = new AircraftLabel(p,metric,screen_width, screen_height, mapFont);
    }

    p->label->update();
    p->label->draw(renderer, (p == selectedAircraft));
}

void View::moveLabels(float dx, float dy) {
	Aircraft *p = appData->aircraftList.head;

	while(p) { 
		if(p->label) {
			p->label->move(dx,dy);
		}

		p = p->next;
	}
}


void View::resolveLabelConflicts() {
    Aircraft *p = appData->aircraftList.head;

    while(p) {
        if(p->label) {
            p->label->clearAcceleration();
        }

        p = p->next;
    }

    p = appData->aircraftList.head;

    while(p) {
        if(p->label) {
            p->label->calculateForces(appData->aircraftList.head);
        }

        p = p->next;
    }

    p = appData->aircraftList.head;

    while(p) {

        if(p->label) {
            p->label->applyForces();

            // if(p->label->getIsChanging()) {
            //     highFramerate = true;
            // }
        }
        
        p = p->next;
    }
}

void View::drawPlanes() {
    Aircraft *p = appData->aircraftList.head;
    SDL_Color planeColor;

    if(selectedAircraft) {
        mapTargetLon = selectedAircraft->lon;
        mapTargetLat = selectedAircraft->lat;             
    }

    p = appData->aircraftList.head;

    while(p) {
        if (p->lon && p->lat) {

            // if lon lat argments were not provided, start by snapping to the first plane we see
            if(centerLon == 0 && centerLat == 0) {
                mapTargetLon = p->lon;
                mapTargetLat = p->lat;
            }

            int x, y;

            float dx, dy;
            pxFromLonLat(&dx, &dy, p->lon, p->lat);
            screenCoords(&x, &y, dx, dy);

            float age_ms = elapsed(p->created);
            if(age_ms < 500) {
                //highFramerate = true;
                float ratio = age_ms / 500.0f;
                float radius = (1.0f - ratio * ratio) * screen_width / 8;
                for(float theta = 0; theta < 2*M_PI; theta += M_PI / 4) {
                    pixelRGBA(renderer, x + radius * cos(theta), y + radius * sin(theta), style.planeColor.r, style.planeColor.g, style.planeColor.b, 255 * ratio);
                }
                // circleRGBA(renderer, x, y, 500 - age_ms, 255,255, 255, (uint8_t)(255.0 * age_ms / 500.0));   
            } else {
                if(MODES_ACFLAGS_HEADING_VALID) {
                    int usex = x;   
                    int usey = y;
                    float useHeading = static_cast<float>(p->track);

                    p->x = usex;
                    p->y = usey;

                    planeColor = lerpColor(style.planeColor, style.planeGoneColor, elapsed_s(p->msSeen) / DISPLAY_ACTIVE);
                    
                    if(p == selectedAircraft) {
                        planeColor = style.selectedColor;
                    }


                    if(outOfBounds(x,y)) {
                        drawPlaneOffMap(x, y, &(p->x), &(p->y), planeColor);
                    } else {
                        if(elapsed(p->msSeenLatLon) < 500) {
                            //highFramerate = true;
                            circleRGBA(renderer, p->x, p->y, elapsed(p->msSeenLatLon) * screen_width / (8192), 127,127, 127, 255 - (uint8_t)(255.0 * elapsed(p->msSeenLatLon) / 500.0));   
                        
                            pxFromLonLat(&dx, &dy, p->getLastLon(), p->getLastLat());
                            screenCoords(&x, &y, dx, dy);

                            usex = lerp(x,usex,elapsed(p->msSeenLatLon) / 500.0);
                            usey = lerp(y,usey,elapsed(p->msSeenLatLon) / 500.0);
                            useHeading = lerpAngle(p->getLastHeading(),useHeading,elapsed(p->msSeenLatLon) / 500.0);
                        }

                        drawPlaneIcon(usex, usey, useHeading, planeColor);
                    }

                    drawPlaneText(p);            
                }
            }
        }
        p = p->next;
    }
}

void View::animateCenterAbsolute(float x, float y) {
    float scale_factor = (screen_width > screen_height) ? screen_width : screen_height;

    float dx = -1.0 * (0.75*(double)screen_width / (double)screen_height) * (x - screen_width/2) * maxDist / (0.95 * scale_factor * 0.5);
    float dy = 1.0 * (y - screen_height/2) * maxDist / (0.95 * scale_factor * 0.5);

    moveLabels(x,y);

    float outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

    float outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((centerLat)/2.0f) * M_PI / 180.0f);

    mapTargetLon = centerLon - outLon;
    mapTargetLat = centerLat - outLat;

    mapTargetMaxDist = 0.25 * maxDist;

    mapMoved = 1;
    highFramerate = true;

}

void View::moveCenterAbsolute(float x, float y) {
    float scale_factor = (screen_width > screen_height) ? screen_width : screen_height;

    float dx = -1.0 * (0.75*(double)screen_width / (double)screen_height) * (x - screen_width/2) * maxDist / (0.95 * scale_factor * 0.5);
    float dy = 1.0 * (y - screen_height/2) * maxDist / (0.95 * scale_factor * 0.5);

    moveLabels(x,y);

    float outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

    float outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((centerLat)/2.0f) * M_PI / 180.0f);

    centerLon += outLon;
    centerLat += outLat;

    mapTargetLon = 0;
    mapTargetLat = 0;

    mapMoved = 1;
    highFramerate = true;
}

void View::moveCenterRelative(float dx, float dy) {
    //
    // need to make lonlat to screen conversion class - this is just the inverse of the stuff in draw.c, without offsets
    //
        
    moveLabels(dx,dy);

    float scale_factor = (screen_width > screen_height) ? screen_width : screen_height;

    dx = -1.0 * dx * maxDist / (0.95 * scale_factor * 0.5);
    dy =  1.0 * dy * maxDist / (0.95 * scale_factor * 0.5);

    float outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

    float outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((centerLat)/2.0f) * M_PI / 180.0f);
    
    centerLon += outLon;
    centerLat += outLat;

    mapTargetLon = 0;
    mapTargetLat = 0;

    mapMoved = 1;
    highFramerate = true;
}

void View::zoomMapToTarget() {
    if(mapTargetMaxDist) {
        if(fabs(mapTargetMaxDist - maxDist) > 0.0001) {
            maxDist += 0.1 * (mapTargetMaxDist - maxDist);
            mapAnimating = 1;
            mapMoved = 1;
            highFramerate = true;
        } else {
            mapTargetMaxDist = 0;
        }
    }
}

void View::moveMapToTarget() {
    if(mapTargetLon && mapTargetLat) {
        if(fabs(mapTargetLon - centerLon) > 0.0001 || fabs(mapTargetLat - centerLat) > 0.0001) {
            centerLon += 0.1 * (mapTargetLon- centerLon);
            centerLat += 0.1 * (mapTargetLat - centerLat);

            mapAnimating = 1;
            mapMoved = 1;    
            highFramerate = true;
        } else {
            mapTargetLon = 0;
            mapTargetLat = 0;
        }     
    }
}

// void View::drawMouse() {
//     if(!mouseMoved) {
//         return;
//     }
    
//     if(elapsed(mouseMovedTime) > 1000) {
//         mouseMoved = false;
//         return;
//     }

//     int alpha =  (int)(255.0f - 255.0f * (float)elapsed(mouseMovedTime) / 1000.0f);

//     lineRGBA(renderer, mousex - 10 * screen_uiscale, mousey,  mousex + 10 * screen_uiscale, mousey, white.r, white.g, white.b, alpha);
//     lineRGBA(renderer, mousex,  mousey - 10 * screen_uiscale,  mousex,  mousey + 10 * screen_uiscale, white.r, white.g, white.b, alpha);
// }

void View::drawClick() {
    if(clickx && clicky) {
        highFramerate = true;

        int radius = .25 * elapsed(clickTime);
        int alpha = 128 - static_cast<int>(0.5 * elapsed(clickTime));
        if(alpha < 0 ) {
            alpha = 0;
            clickx = 0;
            clicky = 0;
        }

        filledCircleRGBA(renderer, clickx, clicky, radius,  style.clickColor.r, style.clickColor.g, style.clickColor.b, alpha);      
    }


    if(selectedAircraft) {
        // this logic should be in input, register a callback for click?

        int boxSize;
        if(elapsed(clickTime) < 300) {
            boxSize = static_cast<int>(20.0 * (1.0 - (1.0 - elapsed(clickTime) / 300.0) * cos(sqrt(elapsed(clickTime))))); 
        } else {
            boxSize = 20;
        }
        //rectangleRGBA(renderer, selectedAircraft->x - boxSize, selectedAircraft->y - boxSize, selectedAircraft->x + boxSize, selectedAircraft->y + boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);                            
        lineRGBA(renderer, selectedAircraft->x - boxSize, selectedAircraft->y - boxSize, selectedAircraft->x - boxSize/2, selectedAircraft->y - boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
        lineRGBA(renderer, selectedAircraft->x - boxSize, selectedAircraft->y - boxSize, selectedAircraft->x - boxSize, selectedAircraft->y - boxSize/2, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);

        lineRGBA(renderer, selectedAircraft->x + boxSize, selectedAircraft->y - boxSize, selectedAircraft->x + boxSize/2, selectedAircraft->y - boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
        lineRGBA(renderer, selectedAircraft->x + boxSize, selectedAircraft->y - boxSize, selectedAircraft->x + boxSize, selectedAircraft->y - boxSize/2, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);

        lineRGBA(renderer, selectedAircraft->x + boxSize, selectedAircraft->y + boxSize, selectedAircraft->x + boxSize/2, selectedAircraft->y + boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
        lineRGBA(renderer, selectedAircraft->x + boxSize, selectedAircraft->y + boxSize, selectedAircraft->x + boxSize, selectedAircraft->y + boxSize/2, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);

        lineRGBA(renderer, selectedAircraft->x - boxSize, selectedAircraft->y + boxSize, selectedAircraft->x - boxSize/2, selectedAircraft->y + boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
        lineRGBA(renderer, selectedAircraft->x - boxSize, selectedAircraft->y + boxSize, selectedAircraft->x - boxSize, selectedAircraft->y + boxSize/2, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
    } 
}

void View::registerClick(int tapcount, int x, int y) {
    if(tapcount == 1) {
        Aircraft *p = appData->aircraftList.head;
        Aircraft *selection = NULL;

        while(p) {
            if(x && y) {
                if((p->x - x) * (p->x - x) + (p->y - y) * (p->y - y) < 900) {
                    if(selection) {
                        if((p->x - x) * (p->x - x) + (p->y - y) * (p->y - y) < 
                            (selection->x - x) * (selection->x - x) + (selection->y - y) * (selection->y - y)) {
                            selection = p;
                        }
                    } else {
                        selection = p;
                    }    
                }
            }

            p = p->next;
        }

        selectedAircraft = selection;
    } else if(tapcount == 2) {
        mapTargetMaxDist = 0.25 * maxDist;
        animateCenterAbsolute(x, y);
    }

    clickx = x;
    clicky = y;
    clickTime = now();
}

void View::registerMouseMove(int x, int y) {
    mouseMoved = true;
    mouseMovedTime = now();
    this->mousex = x;
    this->mousey = y;

    // aircraft debug
    // Aircraft *mouse = appData->aircraftList.find(1);
    // if(mouse != NULL) {
    //     latLonFromScreenCoords(&(mouse->lat), &(mouse->lon), x, screen_height-y);
    //     mouse->live = 1;
    // }    
    highFramerate = true;
}

//
// 
//

void View::draw() {
    drawStartTime = now();

    int targetFrameTime = 15;

    // if(highFramerate) {
    //     targetFrameTime = 15;
    // }
    // highFramerate = false;

    if (lastFrameTime < targetFrameTime) {
         SDL_Delay(static_cast<Uint32>(targetFrameTime - lastFrameTime));
    }
    
    moveMapToTarget();
    zoomMapToTarget();

    drawGeography();

    for(int i = 0; i < 8; i++) {
        // if(resolveLabelConflicts() <  0.001f) {            
        //     break;
        // }
        resolveLabelConflicts();
    }

    lineCount = 0;

    drawScaleBars();
    drawPlanes();  
    drawStatus();
    //drawMouse();
    drawClick();

    SDL_RenderPresent(renderer);  
    
    lastFrameTime = elapsed(drawStartTime); 
}

View::View(AppData *appData){
    this->appData = appData;

    startupState = 0;

    // Display options
    screen_uiscale          = 1;
    screen_width            = 0;
    screen_height           = 0;    
    screen_depth            = 32;
    fps                     = 0;
    fullscreen              = 0;
    screen_index              = 0;

    highFramerate = false;

    centerLon   = 0;
    centerLat   = 0;

    maxDist                 = 25.0;

    mapMoved         = 1;
    mapRedraw        = 1;

    selectedAircraft =  NULL;

    std::thread t1(&Map::load, &map);
    t1.detach();
}

View::~View() {
    closeFont(mapFont);
    closeFont(mapBoldFont);
    closeFont(messageFont);
    closeFont(labelFont);
    closeFont(listFont);

    TTF_Quit();
    SDL_Quit();
}
