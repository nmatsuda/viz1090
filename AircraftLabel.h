
#include <string>
#include "SDL2/SDL_ttf.h" 

#include "Label.h"
#include "Style.h" 

class Aircraft;	


class AircraftLabel {
	public:
		void update();
		void clearAcceleration();
		void calculateForces(Aircraft *check_p);
		void applyForces();

		void draw(SDL_Renderer *renderer);

		AircraftLabel(Aircraft *p, bool metric, int screen_width, int screen_height, TTF_Font *font);

	private:

		Aircraft *p;
		
		Label flightLabel;
		Label altitudeLabel;
		Label speedLabel;

		int labelLevel;

		bool metric;

		float x;
		float y;
		float w;
		float h;

		float target_w;
		float target_h;

		float dx;
		float dy;
		float ddx;
		float ddy;

		float opacity;
		float target_opacity;

		float pressure;

		int screen_width;
		int screen_height;

		///////////

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

		Style style;
};		