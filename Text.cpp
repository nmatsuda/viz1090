#include "SDL2/SDL2_gfxPrimitives.h"

#include "Text.h"

SDL_Rect Text::drawString(std::string text, int x, int y, TTF_Font *font, SDL_Color color)
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

SDL_Rect Text::drawStringBG(std::string text, int x, int y, TTF_Font *font, SDL_Color color, SDL_Color bgColor) {
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


// check if text has changed and surface exists
// redraw existing surface
// or make new surface
// for BG, draw bg size of surface
// draw surface