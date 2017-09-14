#include "font.h"

TTF_Font *loadFont(char *name, int size)
{
	/* Use SDL_TTF to load the font at the specified size */
	
	TTF_Font *font = TTF_OpenFont(name, size);

	if (font == NULL)
	{
		printf("Failed to open Font %s: %s\n", name, TTF_GetError());

		exit(1);
	}
	
	return font;
}

void closeFont(TTF_Font *font)
{
	/* Close the font once we're done with it */
	
	if (font != NULL)
	{
		TTF_CloseFont(font);
	}
}

void drawString(char * text, int x, int y, TTF_Font *font, SDL_Color color)
{
	SDL_Surface *surface;
	
	// surface = TTF_RenderUTF8_Shaded(font, text, foregroundColor, backgroundColor);
	surface = TTF_RenderText_Solid(font, text, color);

	if (surface == NULL)
	{
		printf("Couldn't create String: %s\n", SDL_GetError());

		return;
	}

	SDL_Texture* Message = SDL_CreateTextureFromSurface(game.renderer, surface); //now you can convert it into a texture

	SDL_Rect Message_rect; //create a rect
	Message_rect.x = x;  //controls the rect's x coordinate 
	Message_rect.y = y; // controls the rect's y coordinte
	Message_rect.w = surface->w; // controls the width of the rect
	Message_rect.h = surface->h; // controls the height of the rect

	SDL_RenderCopy(game.renderer, Message, NULL, &Message_rect); //you put the renderer's name first, the Message, the crop size(you can ignore this if you don't want to dabble with cropping), and the rect which is the size and coordinate of your texture


	SDL_FreeSurface(surface);

}


