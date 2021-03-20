#include "Label.h"
#include <string>


void Label::draw(SDL_Renderer *renderer) {
    SDL_Rect rect = getRect();
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
}


void Label::makeSurface() {
//    if(BGColor.a = 0) {
        surface = TTF_RenderUTF8_Solid(font, text.c_str(), FGColor);   
    // } else {
    //     surface = TTF_RenderUTF8_Shaded(font, text.c_str(), FGColor, BGColor);
    // }

    
    if (surface == NULL)
    {
        printf("Couldn't create surface for String %s: %s\n", text.c_str(), SDL_GetError());
    }
}


SDL_Rect Label::getRect() {
    SDL_Rect rect = {0,0,0,0};

    if(!text.length()) { 
        return rect;
    }

    if(surface == NULL) {
        return rect;
    }

    rect.x = x;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;

    return rect;
}

void Label::setText(std::string text) {
    this->text = text;
    makeSurface();
}

void Label::setPosition(int x, int y) {
    this->x = x;
    this->y = y;
}

void Label::setFont(TTF_Font *font) {
    this->font = font;
}

void Label::setFGColor(SDL_Color color) {
    this->FGColor = color;
}

void Label::setBGColor(SDL_Color color) {
    this->BGColor = color;  
}

Label::Label() {
    BGColor = {0, 0, 0, 0};
    surface = NULL;
}

Label::~Label() {
    SDL_FreeSurface(surface);
}


