
#include <string>
#include "SDL2/SDL_ttf.h" 
#include <chrono>

#include "Label.h"
#include "Style.h" 

class Aircraft;	


class AircraftLabel {
	public:
		void update();
		void clearAcceleration();
		void calculateForces(Aircraft *check_p);
		void applyForces();
		void move(float dx, float dy);
		bool getIsChanging();

		void draw(SDL_Renderer *renderer, bool selected);

		AircraftLabel(Aircraft *p, bool metric, int screen_width, int screen_height, TTF_Font *font);

	private:
		SDL_Rect getFullRect(int labelLevel);
		float calculateDensity(Aircraft *check_p, int labelLevel);

		Aircraft *p;
		
		Label flightLabel;
		Label altitudeLabel;
		Label speedLabel;
		Label debugLabel;

		float labelLevel;

		bool metric;

		float x;
		float y;
		float w;
		float h;

		float target_w;
		float target_h;

		float dx;
		float dy;

		float x_buffer[15];
		float y_buffer[15];
		int buffer_idx;
		int buffer_length = 15;

		float ddx;
		float ddy;

		float opacity;
		float target_opacity;

		float pressure;

		int screen_width;
		int screen_height;

		bool isChanging;

		std::chrono::high_resolution_clock::time_point lastLevelChange;

		///////////

	    float label_force = 0.01f;
		float label_dist = 2.0f;
		float density_force = 0.01f;
		float attachment_force = 0.01f;
		float attachment_dist = 10.0f;
		float icon_force = 0.01f;
		float icon_dist = 15.0f;
		float boundary_force = 0.01f;
		float damping_force = 0.65f;
		float velocity_limit = 1.0f;
		float edge_margin = 15.0f;
		float drag_force = 0.00f;


		Style style;
};		
