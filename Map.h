#ifndef MAP_H
#define MAP_H

#include <vector>

typedef struct Point{
	float lat;
	float lon;
} Point;

typedef struct Line{
	float lat_min;
  	float lat_max;
  	float lon_min;
  	float lon_max;

	Point start;
	Point end;

	Line(Point start, Point end) {
		this->start = start;
		this->end = end;

		lat_min = std::min(start.lat,end.lat);
		lat_max = std::max(start.lat,end.lat);
		lon_min = std::min(start.lon,end.lon);
		lon_max = std::max(start.lon,end.lon);
	}
} Line;

typedef struct QuadTree{
  	float lat_min;
  	float lat_max;
  	float lon_min;
  	float lon_max;

	std::vector<Line> lines;

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

	bool QTInsert(QuadTree *tree, Line *line, int depth);
	std::vector<Line> getLinesRecursive(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max);
	std::vector<Line> getLines(float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max);

	Map(); 

	int mapPoints_count;
	float *mapPoints;
};
#endif