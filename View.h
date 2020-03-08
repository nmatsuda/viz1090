#ifndef VIEW_H
#define VIEW_H

#include  "AircraftData.h"

class View {

AircraftData *aircraftData;

public:
	int screenDist(float d);
	void pxFromLonLat(float *dx, float *dy, float lon, float lat);
	void latLonFromScreenCoords(float *lat, float *lon, int x, int y);
	void screenCoords(int *outX, int *outY, float dx, float dy);
	int outOfBounds(int x, int y);
	void drawPlaneOffMap(int x, int y, int *returnx, int *returny, SDL_Color planeColor);
	void drawPlaneIcon(int x, int y, float heading, SDL_Color planeColor);
	void drawTrail(float *oldDx, float *oldDy, float *oldHeading, time_t * oldSeen, int idx);
	void drawScaleBars();
	void drawPolys(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max);
	void drawGeography();
	void drawSignalMarks(Aircraft *p, int x, int y);
	void drawPlaneText(Aircraft *p);
	void drawSelectedAircraftText(Aircraft *p);
	void resolveLabelConflicts();
	void drawPlanes();
	void animateCenterAbsolute(float x, float y);
	void moveCenterAbsolute(float x, float y);
	void moveCenterRelative(float dx, float dy);
	void zoomMapToTarget();
	void moveMapToTarget();
	void drawMouse();
	void registerClick();
	void draw();
	
	View(AircraftData *aircraftData);

	bool metric;
};

#endif