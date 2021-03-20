
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h" 
#include <string>

class Label {
	public:
		void draw(SDL_Renderer *renderer);

		void setText(std::string text);
		void setPosition(int x, int y);
		void setFont(TTF_Font *font);
		void setFGColor(SDL_Color color);
		void setBGColor(SDL_Color color);

		SDL_Rect getRect();

		Label();
		~Label();

	private:
		void makeSurface();


		std::string text;
		int x;
		int y;
		TTF_Font *font;
		SDL_Color FGColor;
		SDL_Color BGColor;
		SDL_Surface *surface;
};