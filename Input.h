#ifndef INPUT_H
#define INPUT_H

#include "AircraftData.h"
#include "View.h"

class Input {
public:
	void getInput();

	//should input know about view?
	Input(View *view);

	View *view;
};

#endif