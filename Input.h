#ifndef INPUT_H
#define INPUT_H

#include "AppData.h"
#include "View.h" 

#include <chrono>

class Input {
public:
	void getInput();

	//should input know about view?
	Input(AppData *appData, View *view);

	View *view;
	AppData *appData;

	std::chrono::high_resolution_clock::time_point touchDownTime;
    int touchx;
    int touchy;
    int tapCount;
};

#endif