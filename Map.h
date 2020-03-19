#ifndef MAP_H
#define MAP_H

#include <list>

typedef struct Point{
	float lat;
	float lon;
} Point;

typedef struct Polygon{
	float lat_min;
  	float lat_max;
  	float lon_min;
  	float lon_max;

	std::list<Point> points;
	int numPoints;

	Polygon() {
		lat_min = 180.0f;
  		lon_min = 180.0f;
  		lat_max = -180.0f;
  		lon_max = -180.0f;
  		numPoints = 0;
	}
} Polygon;

typedef struct QuadTree{
  	float lat_min;
  	float lat_max;
  	float lon_min;
  	float lon_max;

	std::list<Polygon> polygons;

	struct QuadTree *nw;
	struct QuadTree *sw;
	struct QuadTree *ne;
	struct QuadTree *se;

	QuadTree() {
		lat_min = 180.0f;
		lon_min = 180.0f;
		lat_max = -180.0f;
		lon_max = -180.0f;

		nw = NULL;
		sw = NULL;
		ne = NULL;
		se = NULL;
	}

	~QuadTree() {
		if(nw != NULL) {
			delete nw;
		}

		if(sw != NULL) {
			delete nw;
		}

		if(ne != NULL) {
			delete nw;
		}

		if(ne != NULL) {
			delete nw;
		}
	}
} QuadTree;

class Map {

public:
	QuadTree root;

	bool QTInsert(QuadTree *tree, Polygon *polygon);
	std::list<Polygon> getPolysRecursive(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max);
	std::list<Polygon> getPolys(float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max);

	Map(); 

	int mapPoints_count;
	float *mapPoints;
};
#endif