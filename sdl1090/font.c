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
	SDL_Rect dest;

	surface = TTF_RenderUTF8_Solid(font, text, color);

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

	SDL_BlitSurface(surface, NULL, game.screen, &dest);
	
	/* Free the generated string image */

	SDL_FreeSurface(surface);

}


