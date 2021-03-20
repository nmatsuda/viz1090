#include "Label.h"
#include <string>

void Label::draw(SDL_Renderer *renderer) {
    SDL_Rect rect = getRect();
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
}


void Label::makeSurface() {
    surface = TTF_RenderUTF8_Solid(font, text.c_str(), color);   
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

void Label::setColor(SDL_Color color) {
    this->color = color;
}
// 
Label::Label() {
    this->color = {255,255,255,255};
    surface = NULL;
}

Label::~Label() {
    SDL_FreeSurface(surface);
}


