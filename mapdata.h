#ifndef MAPPOINTS_H
#define MAPPOINTS_H

float *mapPoints_relative;
int mapPoints_count;

float *mapPoints;

typedef struct Point{
	float lat;
	float lon;
	struct Point *next;
} Point;

typedef struct Polygon{
	float lat_min;
  	float lat_max;
  	float lon_min;
  	float lon_max;

	Point *points;
	int numPoints;

	struct Polygon *next;
} Polygon;

typedef struct QuadTree{
  	float lat_min;
  	float lat_max;
  	float lon_min;
  	float lon_max;

	Polygon *polygons;

	struct QuadTree *nw;
	struct QuadTree *sw;
	struct QuadTree *ne;
	struct QuadTree *se;
} QuadTree;

QuadTree root;

#endif