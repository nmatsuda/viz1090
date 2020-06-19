// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// Copyright (C) 2014, Malcolm Robb <Support@ATTAvionics.com>
// Copyright (C) 2012, Salvatore Sanfilippo <antirez at gmail dot com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

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

	std::vector<Line*> lines;

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
	std::vector<Line*> getLinesRecursive(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max);
	std::vector<Line*> getLines(float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max);

	Map(); 

	int mapPoints_count;
	float *mapPoints;
};
#endif