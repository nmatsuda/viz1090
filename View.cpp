#include "SDL2/SDL2_rotozoom.h"
#include "SDL2/SDL2_gfxPrimitives.h"
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
    float scale_factor = (screen_width > screen_height) ? screen_width : screen_height;
    return round(0.95 * scale_factor * 0.5 * fabs(d) / maxDist);        
}

void View::pxFromLonLat(float *dx, float *dy, float lon, float lat) {
    if(!lon || !lat) {
        *dx = 0;
        *dy = 0;
        return;
    }

    *dx = 6371.0 * (lon - centerLon) * M_PI / 180.0f * cos(((lat + centerLat)/2.0f) * M_PI / 180.0f);
    *dy = 6371.0 * (lat - centerLat) * M_PI / 180.0f;
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
    if(x < 0 || x >= screen_width || y < 0 || y >= screen_height ) {
        return 1;
    } else {
        return 0;
    }
}

//
// Font stuff
//

TTF_Font* View::loadFont(char *name, int size)
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

    window =  SDL_CreateWindow("map1090",  SDL_WINDOWPOS_CENTERED_DISPLAY(screen_index),  SDL_WINDOWPOS_CENTERED_DISPLAY(screen_index), screen_width, screen_height, flags);        
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

    //
    // todo separate style stuff
    //

    SDL_Color bgcolor = {10,20,30,255};
    SDL_Color greenblue = {236,192,68,255};
    SDL_Color lightblue = {211,208,203,255};
    SDL_Color mediumblue ={110,136,152,255};
    SDL_Color darkblue =  {46,82,102,255};

    style.backgroundColor = bgcolor;
    style.selectedColor = pink;
    style.planeColor = greenblue;
    style.planeGoneColor = grey;
    style.mapInnerColor = mediumblue;
    style.mapOuterColor = darkblue;
    style.scaleBarColor = lightGrey;
    style.buttonColor =  lightblue;
}

void View::drawString(char * text, int x, int y, TTF_Font *font, SDL_Color color)
{
    if(!strlen(text)) { 
        return;
    }

    SDL_Surface *surface;
    SDL_Rect dest;

    surface = TTF_RenderUTF8_Solid(font, text, color);

    if (surface == NULL)
    {
        printf("Couldn't create String %s: %s\n", text, SDL_GetError());

        return;
    }
    
    dest.x = x;
    dest.y = y;
    dest.w = surface->w;
    dest.h = surface->h;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void View::drawStringBG(char * text, int x, int y, TTF_Font *font, SDL_Color color, SDL_Color bgColor) {
    if(!strlen(text)) { 
        return;
    }
        
    SDL_Surface *surface;
    SDL_Rect dest;

    surface = TTF_RenderUTF8_Shaded(font, text, color, bgColor);

    if (surface == NULL)
    {
        printf("Couldn't create String %s: %s\n", text, SDL_GetError());

        return;
    }
    
    /* Blit the entire surface to the screen */

    dest.x = x;
    dest.y = y;
    dest.w = surface->w;
    dest.h = surface->h;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

//
// Status boxes
//

void View::drawStatusBox(int *left, int *top, char *label, char *message, SDL_Color color) {
    int labelWidth = (strlen(label) + ((strlen(label) > 0 ) ? 1 : 0)) * labelFontWidth;
    int messageWidth = (strlen(message) + ((strlen(message) > 0 ) ? 1 : 0)) * messageFontWidth;

    if(*left + labelWidth + messageWidth + PAD > screen_width) {
        *left = PAD;
        *top = *top - messageFontHeight - PAD;
    }

    // filled black background
    if(messageWidth) {
        roundedBoxRGBA(renderer, *left, *top, *left + labelWidth + messageWidth, *top + messageFontHeight, ROUND_RADIUS, black.r, black.g, black.b, SDL_ALPHA_OPAQUE);
    }

    // filled label box
    if(labelWidth) {
        roundedBoxRGBA(renderer, *left, *top, *left + labelWidth, *top + messageFontHeight, ROUND_RADIUS,color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
    }

    // outline message box
    if(messageWidth) {
        roundedRectangleRGBA(renderer, *left, *top, *left + labelWidth + messageWidth, *top + messageFontHeight, ROUND_RADIUS,color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
    }

    drawString(label, *left + labelFontWidth/2, *top, labelFont, black);

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
    filledTrigonRGBA(renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    // arrow 2
    x1 = (screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(-arrowWidth*out[0]);
    y1 = (screen_height * CENTEROFFSET) + outy - 3.0 * arrowWidth * vec[1] + round(-arrowWidth*out[1]);
    x2 = (screen_width>>1) + outx - 3.0 * arrowWidth * vec[0] + round(arrowWidth*out[0]);
    y2 = (screen_height * CENTEROFFSET) + outy - 3.0 * arrowWidth * vec[1] + round(arrowWidth*out[1]);
    x3 = (screen_width>>1) +  outx - 2.0 * arrowWidth * vec[0];
    y3 = (screen_height * CENTEROFFSET) + outy - 2.0 * arrowWidth * vec[1];
    filledTrigonRGBA(renderer, x1, y1, x2, y2, x3, y3, planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    *returnx = x3;
    *returny = y3;
}

void View::drawPlaneIcon(int x, int y, float heading, SDL_Color planeColor)
{
    float body = 8.0 * screen_uiscale;
    float wing = 6.0 * screen_uiscale;
    float tail = 3.0 * screen_uiscale;
    float bodyWidth = 2.0 * screen_uiscale;

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

    thickLineRGBA(renderer,x,y,x2,y2,bodyWidth,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
    filledTrigonRGBA(renderer, x + round(-wing*.35*out[0]), y + round(-wing*.35*out[1]), x + round(wing*.35*out[0]), y + round(wing*.35*out[1]), x1, y1,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);        
    filledCircleRGBA(renderer, x2,y2,screen_uiscale,planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //wing
    x1 = x + round(-wing*out[0]);
    y1 = y + round(-wing*out[1]);
    x2 = x + round(wing*out[0]);
    y2 = y + round(wing*out[1]);

    filledTrigonRGBA(renderer, x1, y1, x2, y2, x+round(body*.35*vec[0]), y+round(body*.35*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);

    //tail
    x1 = x + round(-body*.75*vec[0]) + round(-tail*out[0]);
    y1 = y + round(-body*.75*vec[1]) + round(-tail*out[1]);
    x2 = x + round(-body*.75*vec[0]) + round(tail*out[0]);
    y2 = y + round(-body*.75*vec[1]) + round(tail*out[1]);

    filledTrigonRGBA (renderer, x1, y1, x2, y2, x+round(-body*.5*vec[0]), y+round(-body*.5*vec[1]),planeColor.r,planeColor.g,planeColor.b,SDL_ALPHA_OPAQUE);
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
                   
        thickLineRGBA(renderer, prevX, prevY, currentX, currentY, 2 * screen_uiscale, 255, 255, 255, colorVal); 

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

        // int cross_size = 5 * screen_uiscale * age;

        // // //forward cross
        // x1 = currentX + round(-cross_size*vec[0]);
        // y1 = currentY + round(-cross_size*vec[1]);
        // x2 = currentX + round(cross_size*vec[0]);
        // y2 = currentY + round(cross_size*vec[1]);

        // thickLineRGBA(renderer,x1,y1,x2,y2,screen_uiscale,255,255,255,64);
   
        // //side cross
        // x1 = currentX + round(-cross_size*out[0]);
        // y1 = currentY + round(-cross_size*out[1]);
        // x2 = currentX + round(cross_size*out[0]);
        // y2 = currentY + round(cross_size*out[1]);
        
        // thickLineRGBA(renderer,x1,y1,x2,y2,screen_uiscale,255,255,255,64);

        idx--;
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

void View::drawPolys(float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max) {
    std::list<Polygon> polyList = map.getPolys(screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);

    std::list<Polygon>::iterator currentPolygon;

    for (currentPolygon = polyList.begin(); currentPolygon != polyList.end(); ++currentPolygon) {
        int x1,y1,x2,y2;
        float dx,dy;

        std::list<Point>::iterator currentPoint;
        std::list<Point>::iterator prevPoint;

        for (currentPoint = std::next(currentPolygon->points.begin()); currentPoint != currentPolygon->points.end(); ++currentPoint) {
            prevPoint = std::prev(currentPoint);

            pxFromLonLat(&dx, &dy, prevPoint->lon, prevPoint->lat); 
            screenCoords(&x1, &y1, dx, dy);

            if(outOfBounds(x1,y1)) {
                continue;
            }

            float d1 = dx* dx + dy * dy;

            pxFromLonLat(&dx, &dy, currentPoint->lon, currentPoint->lat); 
            screenCoords(&x2, &y2, dx, dy);

            if(outOfBounds(x2,y2)) {
                continue;
            }

            //this needs a differnt approach for the std list setup

            // if((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1) < MIN_MAP_FEATURE){
            //     currentPoint = currentPoint->next;
            //     continue;
            // }

            float d2 = dx* dx + dy * dy;

            float factor = 1.0 - (d1+d2) / (3* maxDist * maxDist);

            SDL_Color lineColor = lerpColor(style.mapOuterColor, style.mapInnerColor, factor); 

            lineRGBA(renderer, x1, y1, x2, y2, lineColor.r, lineColor.g, lineColor.b, 255);
        }

        ////bounding boxes

        // int x, y;

        // pxFromLonLat(&dx, &dy, currentPolygon->lon_min, currentPolygon->lat_min); 
        // screenCoords(&x, &y, dx, dy);

        // int top = y;
        // int left = x;

        // pxFromLonLat(&dx, &dy, currentPolygon->lon_max, currentPolygon->lat_max); 
        // screenCoords(&x, &y, dx, dy);

        // int bottom = y;
        // int right = x;
          
        // rectangleRGBA(renderer, left, top, right, bottom,  purple.r, purple.g, purple.b, 255);      
    }

}

void View::drawGeography() {
    float screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max;

    latLonFromScreenCoords(&screen_lat_min, &screen_lon_min, 0,  screen_height * -0.2);
    latLonFromScreenCoords(&screen_lat_max, &screen_lon_max, screen_width, screen_height * 1.2);

    drawPolys(screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);    
}

void View::drawSignalMarks(Aircraft *p, int x, int y) {
    unsigned char * pSig       = p->signalLevel;
    unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
                                              pSig[4] + pSig[5] + pSig[6] + pSig[7] + 3) >> 3; 

    SDL_Color barColor = signalToColor(signalAverage);

    Uint8 seenFade = (Uint8) (255.0 - (mstime() - p->msSeen) / 4.0);

    circleRGBA(renderer, x + mapFontWidth, y - 5, 2 * screen_uiscale, barColor.r, barColor.g, barColor.b, seenFade);

    seenFade = (Uint8) (255.0 - (mstime() - p->msSeenLatLon) / 4.0);

    hlineRGBA(renderer, x + mapFontWidth + 5 * screen_uiscale, x + mapFontWidth + 9 * screen_uiscale, y - 5, barColor.r, barColor.g, barColor.b, seenFade);
    vlineRGBA(renderer, x + mapFontWidth + 7 * screen_uiscale, y - 2 * screen_uiscale - 5, y + 2 * screen_uiscale - 5, barColor.r, barColor.g, barColor.b, seenFade);
}


void View::drawPlaneText(Aircraft *p) {
    int maxCharCount = 0;
    int currentCharCount;

    int currentLine = 0;

    if(p->pressure * screen_width< 0.4f) {
        drawSignalMarks(p, p->x, p->y);

        char flight[10] = " ";
        maxCharCount = snprintf(flight,10," %s", p->flight);

        if(maxCharCount > 1) {
            drawStringBG(flight, p->x, p->y, mapBoldFont, white, black); 
            //roundedRectangleRGBA(renderer, p->x, p->y, p->x + maxCharCount * mapFontWidth, p->y + mapFontHeight, ROUND_RADIUS, white.r, white.g, white.b, SDL_ALPHA_OPAQUE);
            //drawString(flight, p->x, p->y, mapBoldFont, white); 
            currentLine++;             
        }
    }

  if(p->pressure * screen_width < 0.2f) {
        char alt[10] = " ";
        if (metric) {
            currentCharCount = snprintf(alt,10," %dm", (int) (p->altitude / 3.2828)); 
        } else {
            currentCharCount = snprintf(alt,10," %d'", p->altitude); 
        }

        if(currentCharCount > 1) {
            drawStringBG(alt, p->x, p->y + currentLine * mapFontHeight, mapFont, grey, black);   
            currentLine++;                              
        }

        if(currentCharCount > maxCharCount) {
            maxCharCount = currentCharCount;
        }   

        char speed[10] = " ";
        if (metric) {
            currentCharCount = snprintf(speed,10," %dkm/h", (int) (p->speed * 1.852));
        } else {
            currentCharCount = snprintf(speed,10," %dmph", p->speed);
        }

        if(currentCharCount > 1) {
            drawStringBG(speed, p->x, p->y + currentLine * mapFontHeight, mapFont, grey, black);  
            currentLine++;               
        }

        if(currentCharCount > maxCharCount) {
            maxCharCount = currentCharCount;
        }
    }

    if(maxCharCount > 1) {

        Sint16 vx[4] = {p->cx, p->cx + (p->x - p->cx) / 2, p->x, p->x};
        Sint16 vy[4] = {p->cy, p->cy + (p->y - p->cy) / 2, p->y - mapFontHeight, p->y};
        
        if(p->cy > p->y + currentLine * mapFontHeight) {
            vy[2] = p->y + currentLine * mapFontHeight + mapFontHeight;
            vy[3] = p->y + currentLine * mapFontHeight;
        } 

        bezierRGBA(renderer,vx,vy,4,2,200,200,200,SDL_ALPHA_OPAQUE);


        thickLineRGBA(renderer,p->x,p->y,p->x,p->y+currentLine*mapFontHeight,screen_uiscale,200,200,200,SDL_ALPHA_OPAQUE);
    }
    
    p->w = maxCharCount * mapFontWidth;
    p->h = currentLine * mapFontHeight;         
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
        drawStringBG(flight, x, y, mapBoldFont, white, black); 
        //roundedRectangleRGBA(renderer, p->x, p->y, p->x + maxCharCount * mapFontWidth, p->y + mapFontHeight, ROUND_RADIUS, white.r, white.g, white.b, SDL_ALPHA_OPAQUE);
        //drawString(flight, p->x, p->y, mapBoldFont, white); 
        currentLine++;             
    }

    char alt[10] = " ";
    if (metric) {
        currentCharCount = snprintf(alt,10," %dm", (int) (p->altitude / 3.2828)); 
    } else {
        currentCharCount = snprintf(alt,10," %d'", p->altitude); 
    }

    if(currentCharCount > 1) {
        drawStringBG(alt, x, y + currentLine * mapFontHeight, mapFont, grey, black);   
        currentLine++;                              
    }

    if(currentCharCount > maxCharCount) {
        maxCharCount = currentCharCount;
    }   

    char speed[10] = " ";
    if (metric) {
        currentCharCount = snprintf(speed,10," %dkm/h", (int) (p->speed * 1.852));
    } else {
        currentCharCount = snprintf(speed,10," %dmph", p->speed);
    }

    if(currentCharCount > 1) {
        drawStringBG(speed, x, y + currentLine * mapFontHeight, mapFont, grey, black);  
        currentLine++;               
    }
}

void View::resolveLabelConflicts() {
    Aircraft *p = appData->aircraftList.head;

    float label_force = 0.01f;
    float plane_force = 0.01f;
    float damping_force = 0.95f;
    float spring_force = 0.02f;
    float spring_length = 10.0f;

    while(p) {

        Aircraft *check_p = appData->aircraftList.head;

        int p_left = p->x - 10 * screen_uiscale;
        int p_right = p->x + p->w + 10 * screen_uiscale;
        int p_top = p->y - 10 * screen_uiscale;
        int p_bottom = p->y + p->h + 10 * screen_uiscale;

        //debug box 
        //rectangleRGBA(renderer, p->x, p->y, p->x + p->w, p->y + p->h, 255,0,0, SDL_ALPHA_OPAQUE);
        //lineRGBA(renderer, p->cx, p->cy, p->x, p->y, 0,255,0, SDL_ALPHA_OPAQUE);
        
        p->ddox = 0;
        p->ddoy = 0;

        float o_mag = sqrt(p->ox*p->ox + p->oy*p->oy);

        //spring back to origin

        if(o_mag > 0) {
            p->ddox -= p->ox / o_mag * spring_force * (o_mag - spring_length);
            p->ddoy -= p->oy / o_mag * spring_force * (o_mag - spring_length);
        }
        
        // // //screen edge 

        if(p_left < 10 * screen_uiscale) {
            p->ox += (float)(10 * screen_uiscale - p_left);
        }

        if(p_right > (screen_width - 10 * screen_uiscale)) {
            p->ox -= (float)(p_right - (screen_width - 10 * screen_uiscale));
        }

        if(p_top < 10 * screen_uiscale) {
            p->oy += (float)(10 * screen_uiscale - p_top);
        }

        if(p_bottom > (screen_height - 10 * screen_uiscale)) {
            p->oy -= (float)(p_bottom - (screen_height - 10 * screen_uiscale));
        }

        p->pressure = 0;


        //check against other labels
        while(check_p) {
            if(check_p->addr != p->addr) {

                int check_left = check_p->x - 5 * screen_uiscale;
                int check_right = check_p->x + check_p->w + 5 * screen_uiscale;
                int check_top = check_p->y - 5 * screen_uiscale; 
                int check_bottom = check_p->y + check_p->h + 5 * screen_uiscale;


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
                    check_p->ddox -= label_force * (float)(check_left - p_right);   
                }

                //right collision
                if(check_right > p_left && check_right < p_right) {
                    check_p->ddox -= label_force * (float)(check_right - p_left);   
                }

                //top collision
                if(check_top > p_top && check_top < p_bottom) {
                    check_p->ddoy -= label_force * (float)(check_top - p_bottom);   
                }

                //bottom collision
                if(check_bottom > p_top && check_bottom < p_bottom) {
                    check_p->ddoy -= label_force * (float)(check_bottom - p_top);   
                }    
            }
            check_p = check_p -> next;
        }

        check_p = appData->aircraftList.head;

        //check against plane icons (include self)

        p_left = p->x - 5 * screen_uiscale;
        p_right = p->x + 5 * screen_uiscale;
        p_top = p->y - 5 * screen_uiscale;
        p_bottom = p->y + 5 * screen_uiscale;

        while(check_p) {

            int check_left = check_p->x - 5 * screen_uiscale;
            int check_right = check_p->x + check_p->w + 5 * screen_uiscale;
            int check_top = check_p->y - 5 * screen_uiscale; 
            int check_bottom = check_p->y + check_p->h + 5 * screen_uiscale;

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

    p = appData->aircraftList.head;

    while(p) {
        p->dox += p->ddox;
        p->doy += p->ddoy;

        p->dox *= damping_force;
        p->doy *= damping_force;
  
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

        p->x = p->cx + (int)round(p->ox);
        p->y = p->cy + (int)round(p->oy);

        p = p->next;
    }
}


void View::drawPlanes() {
    Aircraft *p = appData->aircraftList.head;
    time_t now = time(NULL);
    SDL_Color planeColor;

    // draw all trails first so they don't cover up planes and text
    // also find closest plane to selection point
    while(p) {
        drawTrail(p);
        p = p->next;
    }

    if(selectedAircraft) {
        mapTargetLon = selectedAircraft->lon;
        mapTargetLat = selectedAircraft->lat;             
    }

    p = appData->aircraftList.head;

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
                circleRGBA(renderer, x, y, 500 - age_ms, 255,255, 255, (uint8_t)(255.0 * age_ms / 500.0));   
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
                        
                    planeColor = lerpColor(style.planeColor, style.planeGoneColor, (now - p->seen) / (float) DISPLAY_ACTIVE);
                    
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
                      
                    if(p != selectedAircraft) {
                        drawPlaneText(p);
                    }

                }
            }
        }
        p = p->next;
    }

    drawSelectedAircraftText(selectedAircraft);    
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

    dx = -1.0 * (0.75*(double)screen_width / (double)screen_height) * dx * maxDist / (0.95 * scale_factor * 0.5);
    dy = 1.0 * dy * maxDist / (0.95 * scale_factor * 0.5);

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

            mapMoved = 1;    
        } else {
            mapTargetLon = 0;
            mapTargetLat = 0;
        }     
    }
}

void View::drawMouse() {
    if(mouseMovedTime == 0  || (mstime() - mouseMovedTime) > 1000) {
        mouseMovedTime = 0;
        return;
    }

    int alpha =  (int)(255.0f - 255.0f * (float)(mstime() - mouseMovedTime) / 1000.0f);

    lineRGBA(renderer, mousex - 10 * screen_uiscale, mousey,  mousex + 10 * screen_uiscale, mousey, white.r, white.g, white.b, alpha);
    lineRGBA(renderer, mousex,  mousey - 10 * screen_uiscale,  mousex,  mousey + 10 * screen_uiscale, white.r, white.g, white.b, alpha);
}

void View::drawClick() {
    if(clickx && clicky) {

        int radius = .25 * (mstime() - clickTime);
        int alpha = 128 - (int)(0.5 * (mstime() - clickTime));
        if(alpha < 0 ) {
            alpha = 0;
            clickx = 0;
            clicky = 0;
        }

        filledCircleRGBA(renderer, clickx, clicky, radius,  white.r, white.g, white.b, alpha);      
    }


    if(selectedAircraft) {
        // this logic should be in input, register a callback for click?
        float elapsed  = mstime() - clickTime;

        int boxSize;
        if(elapsed < 300) {
            boxSize = (int)(20.0 * (1.0 - (1.0 - elapsed / 300.0) * cos(sqrt(elapsed)))); 
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
        lineRGBA(renderer, selectedAircraft->cx - boxSize, selectedAircraft->cy + boxSize, selectedAircraft->cx - boxSize, selectedAircraft->cy + boxSize/2, style.selectedColor.r, style.selectedColor.g, pink.b, 255);
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
    clickTime = mstime();
}

void View::registerMouseMove(int x, int y) {
    mouseMovedTime = mstime();
    this->mousex = x;
    this->mousey = y;
}

//
// 
//

void View::draw() {
    uint64_t drawStartTime = mstime();
    
    moveMapToTarget();
    zoomMapToTarget();

    //updatePlanes();

    if(mapMoved) {
        SDL_SetRenderTarget(renderer, mapTexture);
        
        SDL_SetRenderDrawColor(renderer, style.backgroundColor.r, style.backgroundColor.g, style.backgroundColor.b, 255);

        SDL_RenderClear(renderer);
        
        drawGeography();

        drawScaleBars();

        SDL_SetRenderTarget(renderer, NULL );   

        mapMoved = 0; 
    }
    
    for(int i = 0; i < 4; i++) {
        resolveLabelConflicts();
    }

    //SDL_SetRenderDrawColor( renderer, 0, 15, 30, 0);
    SDL_SetRenderDrawColor(renderer, style.backgroundColor.r, style.backgroundColor.g, style.backgroundColor.b, 255);

    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, mapTexture, NULL, NULL);

    drawPlanes();  
    drawStatus();
    drawMouse();
    drawClick();

    char fps[13] = " ";
    snprintf(fps,13," %.1ffps", 1000.0 / (mstime() - lastFrameTime));
    drawStringBG(fps, 0,0, mapFont, grey, black);  

    SDL_RenderPresent(renderer);  

    lastFrameTime = mstime(); 

    if ((mstime() - drawStartTime) < FRAMETIME) {
        usleep(1000 * (FRAMETIME - (mstime() - drawStartTime)));
    } 
}

View::View(AppData *appData){
    this->appData = appData;

    // Display options
    screen_uiscale          = 1;
    screen_width            = 0;
    screen_height           = 0;    
    screen_depth            = 32;
    fullscreen              = 0;
    screen_index              = 0;

    maxDist                 = 25.0;

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