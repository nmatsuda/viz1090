#ifndef MAPPOINTS_H
#define MAPPOINTS_H

double *mapPoints_relative;
int mapPoints_count;

extern double mapPoints[];

typedef struct Point{
	double lat;
	double lon;
	struct Point *next;
} Point;

typedef struct Polygon{
	double lat_min;
  	double lat_max;
  	double lon_min;
  	double lon_max;

	Point *points;
	int numPoints;

	struct Polygon *next;
} Polygon;

typedef struct QuadTree{
  	double lat_min;
  	double lat_max;
  	double lon_min;
  	double lon_max;

	Polygon *polygons;

	struct QuadTree *nw;
	struct QuadTree *sw;
	struct QuadTree *ne;
	struct QuadTree *se;
} QuadTree;

QuadTree root;

#endif