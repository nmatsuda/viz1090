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

//color schemes
#include "parula.h"

#include "View.h"

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

    if(s <= 0.0) {       
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
    float dy = maxDist * (y  - (screen_height * CENTEROFFSET)) / (0.95 * scale_factor * 0.5 );

    *lat = 180.0f * dy / (6371.0 * M_PI) + centerLat;
    *lon = 180.0 * dx / (cos(((*lat + centerLat)/2.0f) * M_PI / 180.0f) * 6371.0 * M_PI) + centerLon;
}


void View::screenCoords(int *outX, int *outY, float dx, float dy) {
    *outX = (screen_width>>1) + ((dx>0) ? 1 : -1) * screenDist(dx);    
    *outY = (screen_height * CENTEROFFSET) + ((dy>0) ? -1 : 1) * screenDist(dy);        
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
// Font stuff
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
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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

SDL_Rect View::drawString(std::string text, int x, int y, TTF_Font *font, SDL_Color color)
{
    SDL_Rect dest = {0,0,0,0};

    if(!text.length()) { 
        return dest;
    }

    SDL_Surface *surface;

    surface = TTF_RenderUTF8_Solid(font, text.c_str(), color);

    if (surface == NULL)
    {
        printf("Couldn't create String %s: %s\n", text.c_str(), SDL_GetError());

        return dest;
    }
    
    dest.x = x;
    dest.y = y;
    dest.w = surface->w;
    dest.h = surface->h;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);

    return dest;
}

SDL_Rect View::drawStringBG(std::string text, int x, int y, TTF_Font *font, SDL_Color color, SDL_Color bgColor) {
    SDL_Rect dest = {0,0,0,0};

    if(!text.length()) { 
        return dest;
    }
        
    SDL_Surface *surface;

    surface = TTF_RenderUTF8_Shaded(font, text.c_str(), color, bgColor);

    if (surface == NULL)
    {
        printf("Couldn't create String %s: %s\n", text.c_str(), SDL_GetError());

        return dest;
    }
    
    dest.x = x;
    dest.y = y;
    dest.w = surface->w;
    dest.h = surface->h;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);

    return dest;
}

//
// Status boxes
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

    drawString(label, *left + labelFontWidth/2, *top, labelFont, style.buttonBackground);

    //message
    drawString(message, *left + labelWidth + messageFontWidth/2, *top, messageFont, color);

    *left = *left + labelWidth + messageWidth + PAD;
}

void View::drawStatus() {

    int left = PAD; 
    int top = screen_height - messageFontHeight - PAD;

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

//

//
// Main drawing
//

void View::drawPlaneOffMap(int x, int y, int *returnx, int *returny, SDL_Color planeColor) {

    float arrowWidth = 6.0 * screen_uiscale;

    float inx = x - (screen_width>>1);
    float iny = y - screen_height * CENTEROFFSET;
    
    float outx, outy;
    outx = inx;
    outy = iny;

    if(abs(inx) > abs(y - (screen_height>>1)) * (float)(screen_width>>1) / (float)(screen_height * CENTEROFFSET)) { //left / right quadrants
        outx = (screen_width>>1) * ((inx > 0) ? 1.0 : -1.0);
        outy = (outx) * iny / (inx);
    } else { // up / down quadrants
        outy = screen_height * ((iny > 0) ? 1.0-CENTEROFFSET : -CENTEROFFSET );
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
    y1 = (screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1] + round(-arrowWidth*out[1]);
    x2 = (screen_width>>1) + outx - 2.0 * arrowWidth * vec[0] + round(arrowWidth*out[0]);
    y2 = (screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1] + round(arrowWidth*out[1]);
    x3 = (screen_width>>1) +  outx - arrowWidth * vec[0];
    y3 = (screen_height * CENTEROFFSET) + outy - arrowWidth * vec[1];
    trigonRGBA(renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    // arrow 2
    x1 = (screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(-arrowWidth*out[0]);
    y1 = (screen_height * CENTEROFFSET) + outy - 3.0 * arrowWidth * vec[1] + round(-arrowWidth*out[1]);
    x2 = (screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(arrowWidth*out[0]);
    y2 = (screen_height * CENTEROFFSET) + outy - 3.0 * arrowWidth * vec[1] + round(arrowWidth*out[1]);
    x3 = (screen_width>>1) +  outx - 2.0 * arrowWidth * vec[0];
    y3 = (screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1];
    trigonRGBA(renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    *returnx = x3;
    *returny = y3;
}

void View::drawPlaneIcon(int x, int y, float heading, SDL_Color planeColor)
{
    float body = 8.0 * screen_uiscale;
    float wing = 6.0 * screen_uiscale;
    float wingThick = 0.35;
    float tail = 3.0 * screen_uiscale;
    float tailThick = 0.5;
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

    trigonRGBA (renderer, x1, y1, x2, y2, x+round(-body * vec[0]), y+round(-body*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    trigonRGBA (renderer, x1, y1, x2, y2, x+round(body * vec[0]), y+round(body*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    // x1 = x + round(-body*vec[0] - bodyWidth*out[0]);
    // y1 = y + round(-body*vec[1] - bodyWidth*out[1]);
    // x2 = x + round(body*vec[0] - bodyWidth*out[0]);
    // y2 = y + round(body*vec[1] - bodyWidth*out[1]);

    // lineRGBA(renderer,x,y,x2,y2,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

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

    trigonRGBA(renderer, x1, y1, x2, y2, x+round(body*wingThick*vec[0]), y+round(body*wingThick*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //tail
    x1 = x + round(-body*.75*vec[0] - tail*out[0]);
    y1 = y + round(-body*.75*vec[1] - tail*out[1]);
    x2 = x + round(-body*.75*vec[0] + tail*out[0]);
    y2 = y + round(-body*.75*vec[1] + tail*out[1]);

    trigonRGBA (renderer, x1, y1, x2, y2, x+round(-body*tailThick*vec[0]), y+round(-body*tailThick*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
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

                SDL_Color color = lerpColor(style.trailColor, style.planeGoneColor, float(elapsed_s(p->msSeen)) / (float) DISPLAY_ACTIVE);
                
                if(p == selectedAircraft) {
                    color = style.selectedColor;
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

                    uint8_t colorVal = (uint8_t)floor(127.0 * (age / (float)p->lonHistory.size()));
                               
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
            snprintf(scaleLabel,13,"%dkm", (int)pow(10,scalePower));
        } else {
            snprintf(scaleLabel,13,"%dmi", (int)pow(10,scalePower));
        }

        drawString(scaleLabel, 10+scaleBarDist, 15*screen_uiscale, mapFont, style.scaleBarColor);

        scalePower++;
        scaleBarDist = screenDist((float)pow(10,scalePower));
    }

    scalePower--;
    scaleBarDist = screenDist((float)pow(10,scalePower));

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

    for(std::vector<Label*>::iterator label = map.mapnames.begin(); label != map.mapnames.end(); ++label) {
        float dx, dy;
        int x,y;

        pxFromLonLat(&dx, &dy, (*label)->location.lon, (*label)->location.lat);
        screenCoords(&x, &y, dx, dy);

        if(outOfBounds(x,y)) {
            continue;
        }

        drawString((*label)->text, x, y, mapFont, style.geoColor);
    }

    for(std::vector<Label*>::iterator label = map.airportnames.begin(); label != map.airportnames.end(); ++label) {
        float dx, dy;
        int x,y;

        pxFromLonLat(&dx, &dy, (*label)->location.lon, (*label)->location.lat);
        screenCoords(&x, &y, dx, dy);

        if(outOfBounds(x,y)) {
            continue;
        }

        drawString((*label)->text, x, y, listFont, style.airportColor);
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

void View::drawSignalMarks(Aircraft *p, int x, int y) {
    unsigned char * pSig       = p->signalLevel;
    unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
                                              pSig[4] + pSig[5] + pSig[6] + pSig[7] + 3) >> 3; 

    SDL_Color barColor = signalToColor(signalAverage);

    Uint8 seenFade;

    if(elapsed(p->msSeen) < 1024) {
        seenFade = (Uint8) (255.0 - elapsed(p->msSeen) / 4.0);

        circleRGBA(renderer, x + mapFontWidth, y - 5, 2 * screen_uiscale, barColor.r, barColor.g, barColor.b, seenFade);
    }

    if(elapsed(p->msSeenLatLon) < 1024) {
        seenFade = (Uint8) (255.0 - elapsed(p->msSeenLatLon) / 4.0);

        hlineRGBA(renderer, x + mapFontWidth + 5 * screen_uiscale, x + mapFontWidth + 9 * screen_uiscale, y - 5, barColor.r, barColor.g, barColor.b, seenFade);
        vlineRGBA(renderer, x + mapFontWidth + 7 * screen_uiscale, y - 2 * screen_uiscale - 5, y + 2 * screen_uiscale - 5, barColor.r, barColor.g, barColor.b, seenFade);
    }
}


void View::drawPlaneText(Aircraft *p) {

    //don't draw first time
    if(p->x == 0 || p->y == 0) {
        return;
    }

    int charCount;
    int totalWidth = 0;
    int totalHeight = 0;

    int margin = 4 * screen_uiscale;

    SDL_Rect outRect;

    if(p->opacity == 0 && p->labelLevel < 2) {
        p->target_opacity = 1.0f;
    }

    if(p->opacity > 0 && p->labelLevel >= 2) {
        p->target_opacity = 0.0f;
    }

    p->opacity += 0.25f * (p->target_opacity - p->opacity);

    if(p->opacity < 0.05f) {
        p->opacity = 0;
    }

    if(p->w != 0) {

        SDL_Color drawColor = style.labelLineColor;

        drawColor.a = (int) (255.0f * p->opacity);

        if(p == selectedAircraft) {
            drawColor = style.selectedColor;
        }

        int tick = 4;

        int anchor_x, anchor_y, exit_x, exit_y;

        if(p->x + p->w / 2 > p->cx) {
            anchor_x = p->x;
        } else {
            anchor_x = p->x + p->w;
        }

        if(p->y + p->h / 2 > p->cy) {
            anchor_y = p->y - margin;
        } else {
            anchor_y = p->y + p->h + margin;
        }

        if(abs(anchor_x - p->cx) > abs(anchor_y - p->cy)) {
            exit_x = (anchor_x + p->cx) / 2;
            exit_y = anchor_y;
        } else {
            exit_x = anchor_x;
            exit_y = (anchor_y + p->cy) / 2;            
        }

        Sint16 vx[3] = {
            static_cast<Sint16>(p->cx), 
            static_cast<Sint16>(exit_x), 
            static_cast<Sint16>(anchor_x)};

        Sint16 vy[3] = {
            static_cast<Sint16>(p->cy), 
            static_cast<Sint16>(exit_y), 
            static_cast<Sint16>(anchor_y)};        

        boxRGBA(renderer, p->x, p->y, p->x + p->w, p->y + p->h, 0, 0, 0, 255);

        bezierRGBA(renderer, vx, vy, 3, 2, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

        //lineRGBA(renderer, p->x,p->y - margin, p->x + tick, p->y - margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, p->x,p->y - margin, p->x + p->w, p->y - margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, p->x,p->y - margin, p->x, p->y - margin + tick, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

        // lineRGBA(renderer, p->x + p->w, p->y - margin, p->x + p->w - tick, p->y - margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, p->x + p->w, p->y - margin, p->x + p->w, p->y - margin + tick, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

        //lineRGBA(renderer, p->x, p->y + p->h + margin, p->x + tick, p->y + p->h + margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, p->x, p->y + p->h + margin, p->x + p->w, p->y + p->h + margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, p->x, p->y + p->h + margin, p->x, p->y + p->h + margin - tick, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

        // lineRGBA(renderer, p->x + p->w, p->y + p->h + margin,p->x + p->w - tick, p->y + p->h + margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, p->x + p->w, p->y + p->h + margin,p->x + p->w, p->y + p->h + margin - tick, drawColor.r, drawColor.g, drawColor.b, drawColor.a); 
    }
    
    if(p->labelLevel < 2 || p == selectedAircraft) {
        // drawSignalMarks(p, p->x, p->y);

        SDL_Color drawColor = style.labelColor;
        drawColor.a = (int) (255.0f * p->opacity);

        char flight[10] = "";
        charCount = snprintf(flight,10," %s", p->flight);

        if(charCount > 1) {
            outRect = drawString(flight, p->x, p->y, mapBoldFont, drawColor); 

            totalWidth = std::max(totalWidth,outRect.w);  
            totalHeight += outRect.h;            
        }        
    }

    if(p->labelLevel < 1 || p == selectedAircraft) {
        SDL_Color drawColor = style.subLabelColor;
        drawColor.a = (int) (255.0f * p->opacity);

        char alt[10] = "";
        if (metric) {
            charCount = snprintf(alt,10," %dm", (int) (p->altitude / 3.2828)); 
        } else {
            charCount = snprintf(alt,10," %d'", p->altitude); 
        }

        if(charCount > 1) {
            // drawStringBG(alt, p->x, p->y + currentLine * mapFontHeight, mapFont, style.subLabelColor, style.labelBackground);   
            outRect = drawString(alt, p->x, p->y + totalHeight, mapFont, drawColor);   

            totalWidth = std::max(totalWidth,outRect.w);  
            totalHeight += outRect.h;                              
        }

        char speed[10] = "";
        if (metric) {
            charCount = snprintf(speed,10," %dkm/h", (int) (p->speed * 1.852));
        } else {
            charCount = snprintf(speed,10," %dmph", p->speed);
        }

        if(charCount > 1) {
            // drawStringBG(speed, p->x, p->y + currentLine * mapFontHeight, mapFont, style.subLabelColor, style.labelBackground);  
            outRect = drawString(speed, p->x, p->y + totalHeight, mapFont, drawColor);  

            totalWidth = std::max(totalWidth,outRect.w);  
            totalHeight += outRect.h;      
        }
    }


    //label debug
    // char debug[25] = "";
    // snprintf(debug,25,"%1.2f", p->labelLevel); 
    // drawString(debug, p->x, p->y + totalHeight, mapFont, style.red);   

    // if(maxCharCount > 1) {

    //     Sint16 vx[4] = {
    //         static_cast<Sint16>(p->cx), 
    //         static_cast<Sint16>(p->cx + (p->x - p->cx) / 2), 
    //         static_cast<Sint16>(p->x), 
    //         static_cast<Sint16>(p->x)};

    //     Sint16 vy[4] = {
    //         static_cast<Sint16>(p->cy), 
    //         static_cast<Sint16>(p->cy + (p->y - p->cy) / 2), 
    //         static_cast<Sint16>(p->y - mapFontHeight), 
    //         static_cast<Sint16>(p->y)};
        
    //     if(p->cy > p->y + currentLine * mapFontHeight) {
    //         vy[2] = p->y + currentLine * mapFontHeight + mapFontHeight;
    //         vy[3] = p->y + currentLine * mapFontHeight;
    //     } 

    //     bezierRGBA(renderer,vx,vy,4,2,style.labelLineColor.r,style.labelLineColor.g,style.labelLineColor.b,SDL_ALPHA_OPAQUE);


        // lineRGBA(renderer,p->x,p->y,p->x,p->y+currentLine*mapFontHeight,style.labelLineColor.r,style.labelLineColor.g,style.labelLineColor.b,SDL_ALPHA_OPAQUE);
    // }

    p->target_w = totalWidth;
    p->target_h = totalHeight;

    p->w += 0.25f * (p->target_w - p->w);
    p->h += 0.25f * (p->target_h - p->h);

    if(p->w < 0.05f) {
        p->w = 0;
    }

    if(p->h < 0.05f) {
        p->h = 0;
    }
}

float View::resolveLabelConflicts() {
    float label_force = 0.001f;
    float label_dist = 2.0f;
    float density_force = 0.05f;
    float attachment_force = 0.0015f;
    float attachment_dist = 10.0f;
    float icon_force = 0.001f;
    float icon_dist = 15.0f;
    float boundary_force = 0.01f;
    float damping_force = 0.85f;
    float velocity_limit = 2.0f;
    float edge_margin = 15.0f;

    float maxV = 0.0f;    

    Aircraft *p = appData->aircraftList.head;

    while(p) {
        p->ddox = 0;
        p->ddoy = 0;
        p = p->next;
    }

    p = appData->aircraftList.head;

    while(p) {
        //don't update on first run
        if(p->x == 0) {
            p = p->next;
            continue;
        }

        Aircraft *check_p = appData->aircraftList.head;

        int p_left = p->x;
        int p_right = p->x + p->w;
        int p_top = p->y;
        int p_bottom = p->y + p->h;

           
        float boxmid_x = (float)(p_left + p_right) / 2.0f;
        float boxmid_y = (float)(p_top + p_bottom) / 2.0f;
        
        float offset_x = boxmid_x - p->cx;
        float offset_y = boxmid_y - p->cy;

        float target_length_x = attachment_dist + p->w / 2.0f;
        float target_length_y = attachment_dist + p->h / 2.0f;

        // stay icon_dist away from own icon
    
        p->ddox -= sign(offset_x) * attachment_force * (fabs(offset_x) - target_length_x);
        p->ddoy -= sign(offset_y) * attachment_force * (fabs(offset_y) - target_length_y);
        

        // // //screen edge 

        if(p_left < edge_margin) {
            p->ddox += boundary_force * (float)(edge_margin - p_left);
        }

        if(p_right > screen_width - edge_margin) {
            p->ddox += boundary_force * (float)(screen_width - edge_margin - p_right);
        }

        if(p_top < edge_margin) {
            p->ddoy += boundary_force * (float)(edge_margin - p_top);
        }

        if(p_bottom > screen_height - edge_margin) {
            p->ddoy += boundary_force * (float)(screen_height - edge_margin - p_bottom);
        }


        float all_x = 0;
        float all_y = 0;
        int count = 0;
        //check against other labels

        float density_max = 0;

        while(check_p) {
            if(check_p->addr == p->addr) {
                check_p = check_p -> next;
                continue;
            }


            //calculate density for label display level (inversely proportional to area of smallest box connecting this to neighbor)
            float density = 1.0 / (0.001f + fabs(p->x - check_p->x) * fabs (p->x - check_p->y));

            if(density > density_max) {
                density_max = density;
            }

            density = 1.0 / (0.001f + fabs(p->x - check_p->cx) * fabs(p->x - check_p->cy));
            
            if(density > density_max) {
                density_max = density;
            }

            int check_left = check_p->x;
            int check_right = check_p->x + check_p->w;
            int check_top = check_p->y;
            int check_bottom = check_p->y + check_p->h;

            float icon_x = (float)check_p->cx;
            float icon_y = (float)check_p->cy;
 
            float checkboxmid_x = (float)(check_left + check_right) / 2.0f;
            float checkboxmid_y = (float)(check_top + check_bottom) / 2.0f;

            float offset_x = boxmid_x - checkboxmid_x;
            float offset_y = boxmid_y - checkboxmid_y;

            float target_length_x = label_dist + (float)(check_p->w + p->w) / 2.0f;
            float target_length_y = label_dist + (float)(check_p->h + p->h) / 2.0f;
        
            float x_mag = std::max(0.0f,(target_length_x - fabs(offset_x)));
            float y_mag = std::max(0.0f,(target_length_y - fabs(offset_y)));

            // stay at least label_dist away from other icons

            if(x_mag > 0 && y_mag > 0) {
                p->ddox += sign(offset_x) * label_force * x_mag;                        
                p->ddoy += sign(offset_y) * label_force * y_mag;    
            }            
       
            // stay at least icon_dist away from other icons

            offset_x = boxmid_x - check_p->cx;
            offset_y = boxmid_y - check_p->cy;

            target_length_x = icon_dist + (float)check_p->w / 2.0f;
            target_length_y = icon_dist + (float)check_p->h / 2.0f;
        
            x_mag = std::max(0.0f,(target_length_x - fabs(offset_x)));
            y_mag = std::max(0.0f,(target_length_y - fabs(offset_y)));

            if(x_mag > 0 && y_mag > 0) {
                p->ddox += sign(offset_x) * icon_force * x_mag;    
                p->ddoy += sign(offset_y) * icon_force * y_mag;
            }

            all_x += sign(boxmid_x - checkboxmid_x);
            all_y += sign(boxmid_y - checkboxmid_y);

            count++;
        
            check_p = check_p -> next;
        }


        // move away from others
        p->ddox += density_force * all_x / count;
        p->ddoy += density_force * all_y / count;

        // label drawlevel hysteresis
        
        float density_mult = 100.0f;
        float level_rate = 0.0005f;

        if(p->labelLevel < -1.25f + density_mult * density_max) {
            p->labelLevel += level_rate;
        } else if (p->labelLevel > 0.5f + density_mult * density_max) {
            p->labelLevel -= level_rate;                         
        }

        p = p->next;
    }

    //update 

    p = appData->aircraftList.head;

    while(p) {

        // if this is the first update don't update based on physics
        if(p->x == 0) {
            p->x = p->cx;
            p->y = p->cy + 20*screen_uiscale;      

            p = p->next;
            continue;
        }

        //add noise to acceleration to help with resonance and stuck labels
        // float noise_x = ((float) rand() / (float) RAND_MAX) - 0.5f;
        // float noise_y =  ((float) rand() / (float) RAND_MAX) - 0.5f;


        p->dox += p->ddox;// + 0.001f;// * noise_x;
        p->doy += p->ddoy;// + 0.001f;//. * noise_y;

        p->dox *= damping_force;
        p->doy *= damping_force;
  
        if(fabs(p->dox) > velocity_limit) {
            p->dox = sign(p->dox) * velocity_limit;
        }

        if(fabs(p->doy) > velocity_limit) {
            p->doy = sign(p->doy) * velocity_limit;
        }

        if(fabs(p->dox) < 0.01f) {
            p->dox = 0;
        }

        if(fabs(p->doy) < 0.01f) {
            p->doy = 0;
        }

        p->ox += p->dox;
        p->oy += p->doy;

        p->x = p->cx + (int)round(p->ox);
        p->y = p->cy + (int)round(p->oy);
    

        if(fabs(p->dox) > maxV) {
            maxV = fabs(p->dox);
        }

        if(fabs(p->doy) > maxV) {
            maxV = fabs(p->doy);
        }

        //debug box 
        // rectangleRGBA(renderer, p->x, p->y, p->x + p->w, p->y + p->h, 255,0,0, SDL_ALPHA_OPAQUE);
        // lineRGBA(renderer, p->cx, p->cy, p->x, p->y, 0,255,0, SDL_ALPHA_OPAQUE);    
        // lineRGBA(renderer,p->x, p->y,  p->x + p->ddox, p->y+p->ddoy, 255,0,255,255);
        // lineRGBA(renderer,p->x, p->y,  p->x + p->dox, p->y+p->doy, 0,255,255,255);

        p = p->next;
    }

    return maxV;
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

                    //draw predicted position
                    // if(p->timestampHistory.size() > 2) {

                    //     int x1, y1, x2, y2;

                    //     pxFromLonLat(&dx, &dy, p->lonHistory.end()[-1], p->latHistory.end()[-1]);
                    //     screenCoords(&x1, &y1, dx, dy);
                        
                    //     pxFromLonLat(&dx, &dy, p->lonHistory.end()[-2], p->latHistory.end()[-2]);
                    //     screenCoords(&x2, &y2, dx, dy);

                    //     //printf("latlon: [%f %f] -> [%f %f], px: [%d %d] -> [%d  %d]\n",p->lonHistory.end()[-1], p->latHistory.end()[-1],p->lonHistory.end()[-2], p->latHistory.end()[-2], x1,y1,x2,y2);


                    //     float velx = float(x1 - x2) / (fmilliseconds{p->timestampHistory.end()[-1] - p->timestampHistory.end()[-2]}).count();
                    //     float vely = float(y1 - y2) / (fmilliseconds{p->timestampHistory.end()[-1] - p->timestampHistory.end()[-2]}).count();

                    //     //printf("diff: %f\n",(fmilliseconds{p->timestampHistory.end()[-1] - p->timestampHistory.end()[-2]}).count());

                    //     //printf("%f %f, %d - %d \n", velx,vely,p->timestampHistory.end()[-1], p->timestampHistory.end()[-2]);

                    //     float predx = x + float(elapsed(p->msSeenLatLon)) * velx;
                    //     float predy = y + float(elapsed(p->msSeenLatLon)) * vely;
                    //     circleRGBA(renderer, predx, predy, 4 * screen_uiscale, 127,127, 127, 255);
                    //     lineRGBA(renderer, p->cx, p->cy, predx, predy, 127,127, 127, 255);
                    // }

                    planeColor = lerpColor(style.planeColor, style.planeGoneColor, float(elapsed_s(p->msSeen)) / (float) DISPLAY_ACTIVE);
                    
                    if(p == selectedAircraft) {
                        planeColor = style.selectedColor;
                    }

                    if(outOfBounds(x,y)) {
                        drawPlaneOffMap(x, y, &(p->cx), &(p->cy), planeColor);
                    } else {
                        drawPlaneIcon(usex, usey, p->track, planeColor);

                        p->cx = usex;
                        p->cy = usey;
                    }
                      
                
                    //show latlon ping
                    if(elapsed(p->msSeenLatLon) < 500) {
                        circleRGBA(renderer, p->cx, p->cy, elapsed(p->msSeenLatLon) * screen_width / (8192), 127,127, 127, 255 - (uint8_t)(255.0 * elapsed(p->msSeenLatLon) / 500.0));   
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

    float outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

    float outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((centerLat)/2.0f) * M_PI / 180.0f);

    //double outLon, outLat;
    //latLonFromScreenCoords(&outLat, &outLon, event.tfinger.dx, event.tfinger.dy);

    mapTargetLon = centerLon - outLon;
    mapTargetLat = centerLat - outLat;

    mapTargetMaxDist = 0.25 * maxDist;

    mapMoved = 1;
}


void View::moveCenterAbsolute(float x, float y) {
    float scale_factor = (screen_width > screen_height) ? screen_width : screen_height;

    float dx = -1.0 * (0.75*(double)screen_width / (double)screen_height) * (x - screen_width/2) * maxDist / (0.95 * scale_factor * 0.5);
    float dy = 1.0 * (y - screen_height/2) * maxDist / (0.95 * scale_factor * 0.5);

    float outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

    float outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((centerLat)/2.0f) * M_PI / 180.0f);

    //double outLon, outLat;
    //latLonFromScreenCoords(&outLat, &outLon, event.tfinger.dx, event.tfinger.dy);

    centerLon += outLon;
    centerLat += outLat;

    mapTargetLon = 0;
    mapTargetLat = 0;

    mapMoved = 1;
}

void View::moveCenterRelative(float dx, float dy) {
    //
    // need to make lonlat to screen conversion class - this is just the inverse of the stuff in draw.c, without offsets
    //
        
    float scale_factor = (screen_width > screen_height) ? screen_width : screen_height;

    dx = -1.0 * dx * maxDist / (0.95 * scale_factor * 0.5);
    dy =  1.0 * dy * maxDist / (0.95 * scale_factor * 0.5);

    float outLat = dy * (1.0/6371.0) * (180.0f / M_PI);

    float outLon = dx * (1.0/6371.0) * (180.0f / M_PI) / cos(((centerLat)/2.0f) * M_PI / 180.0f);

    //double outLon, outLat;
    //latLonFromScreenCoords(&outLat, &outLon, event.tfinger.dx, event.tfinger.dy);

    centerLon += outLon;
    centerLat += outLat;

    mapTargetLon = 0;
    mapTargetLat = 0;

    mapMoved = 1;
}

void View::zoomMapToTarget() {
    if(mapTargetMaxDist) {
        if(fabs(mapTargetMaxDist - maxDist) > 0.0001) {
            maxDist += 0.1 * (mapTargetMaxDist - maxDist);
            mapAnimating = 1;
            mapMoved = 1;
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

        int radius = .25 * elapsed(clickTime);
        int alpha = 128 - (int)(0.5 * elapsed(clickTime));
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
            boxSize = (int)(20.0 * (1.0 - (1.0 - float(elapsed(clickTime)) / 300.0) * cos(sqrt(float(elapsed(clickTime)))))); 
        } else {
            boxSize = 20;
        }
        //rectangleRGBA(renderer, selectedAircraft->cx - boxSize, selectedAircraft->cy - boxSize, selectedAircraft->cx + boxSize, selectedAircraft->cy + boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);                            
        lineRGBA(renderer, selectedAircraft->cx - boxSize, selectedAircraft->cy - boxSize, selectedAircraft->cx - boxSize/2, selectedAircraft->cy - boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
        lineRGBA(renderer, selectedAircraft->cx - boxSize, selectedAircraft->cy - boxSize, selectedAircraft->cx - boxSize, selectedAircraft->cy - boxSize/2, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);

        lineRGBA(renderer, selectedAircraft->cx + boxSize, selectedAircraft->cy - boxSize, selectedAircraft->cx + boxSize/2, selectedAircraft->cy - boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
        lineRGBA(renderer, selectedAircraft->cx + boxSize, selectedAircraft->cy - boxSize, selectedAircraft->cx + boxSize, selectedAircraft->cy - boxSize/2, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);

        lineRGBA(renderer, selectedAircraft->cx + boxSize, selectedAircraft->cy + boxSize, selectedAircraft->cx + boxSize/2, selectedAircraft->cy + boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
        lineRGBA(renderer, selectedAircraft->cx + boxSize, selectedAircraft->cy + boxSize, selectedAircraft->cx + boxSize, selectedAircraft->cy + boxSize/2, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);

        lineRGBA(renderer, selectedAircraft->cx - boxSize, selectedAircraft->cy + boxSize, selectedAircraft->cx - boxSize/2, selectedAircraft->cy + boxSize, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
        lineRGBA(renderer, selectedAircraft->cx - boxSize, selectedAircraft->cy + boxSize, selectedAircraft->cx - boxSize, selectedAircraft->cy + boxSize/2, style.selectedColor.r, style.selectedColor.g, style.selectedColor.b, 255);
    } 
}

void View::registerClick(int tapcount, int x, int y) {
    if(tapcount == 1) {
        Aircraft *p = appData->aircraftList.head;
        Aircraft *selection = NULL;

        while(p) {
            if(x && y) {
                if((p->cx - x) * (p->cx - x) + (p->cy - y) * (p->cy - y) < 900) {
                    if(selection) {
                        if((p->cx - x) * (p->cx - x) + (p->cy - y) * (p->cy - y) < 
                            (selection->cx - x) * (selection->cx - x) + (selection->cy - y) * (selection->cy - y)) {
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
}

//
// 
//

void View::draw() {
    drawStartTime = now();

    moveMapToTarget();
    zoomMapToTarget();
    drawGeography();

    for(int i = 0; i < 8; i++) {
        if(resolveLabelConflicts() <  0.001f) {            
            break;
        }
    }

    lineCount = 0;

    drawScaleBars();
    drawPlanes();  
    drawStatus();
    //drawMouse();
    drawClick();

    if(fps) {
        char fps[60] = " ";
        snprintf(fps,40," %d lines @ %.1ffps", lineCount, 1000.0 / elapsed(lastFrameTime));
        drawStringBG(fps, 0,0, mapFont, style.subLabelColor, style.backgroundColor);      
    }
    
    SDL_RenderPresent(renderer);  

   if (elapsed(drawStartTime) < FRAMETIME) {
        std::this_thread::sleep_for(fmilliseconds{FRAMETIME} - (now() - drawStartTime));
    } 

    lastFrameTime = now(); 
}

View::View(AppData *appData){
    this->appData = appData;

    // Display options
    screen_uiscale          = 1;
    screen_width            = 0;
    screen_height           = 0;    
    screen_depth            = 32;
    fps                     = 0;
    fullscreen              = 0;
    screen_index              = 0;

    centerLon   = 0;
    centerLat   = 0;

    maxDist                 = 25.0;

    mapMoved         = 1;
    mapRedraw        = 1;

    selectedAircraft =  NULL;
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
