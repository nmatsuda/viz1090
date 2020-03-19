#ifndef INPUT_H
#define INPUT_H

#include "AppData.h"
#include "View.h"

class Input {
public:
	void getInput();

	//should input know about view?
	Input(AppData *appData, View *view);

	View *view;
	AppData *appData;

	uint64_t touchDownTime;
    int touchx;
    int touchy;
    int tapCount;
};

#endif