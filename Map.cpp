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

#include "Map.h"
#include <stdio.h>
#include <cstdlib>

bool Map::QTInsert(QuadTree *tree, Line *line, int depth) {

  // if(depth > 25) {
  //  // printf("fail [%f %f] -> [%f %f]\n",line->start.lon,line->start.lat,line->end.lon,line->end.lat);

  //  // printf("bounds %f %f %f %f\n",tree->lon_min, tree->lon_max, tree->lat_min, tree->lat_max);
  //   // fflush(stdout); 
  //    tree->lines.push_back(&(*line));
  //    return true;
  // }
  

  bool startInside = line->start.lat >= tree->lat_min &&
   line->start.lat <= tree->lat_max &&
   line->start.lon >= tree->lon_min &&
   line->start.lon <= tree->lon_max;

  bool endInside = line->end.lat >= tree->lat_min &&
   line->end.lat <= tree->lat_max &&
   line->end.lon >= tree->lon_min &&
   line->end.lon <= tree->lon_max;

  // if (!startInside || !endInside) {
  //   return false; 
  // }
  if (!startInside && !endInside) {
    return false; 
  }
  
  if (startInside != endInside) {
    tree->lines.push_back(&(*line));
    return true; 
  }     

  if (tree->nw == NULL) {
  	tree->nw = new QuadTree;

  	tree->nw->lat_min = tree->lat_min;	
  	tree->nw->lat_max = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
  	tree->nw->lon_min = tree->lon_min;
  	tree->nw->lon_max = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  }

  if (QTInsert(tree->nw, line, depth++)){
  	return true;
  }

  if (tree->sw == NULL) {
  	tree->sw = new QuadTree;

  	tree->sw->lat_min = tree->lat_min;	
  	tree->sw->lat_max = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
  	tree->sw->lon_min = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  	tree->sw->lon_max = tree->lon_max;
  }

	if (QTInsert(tree->sw, line, depth++)){
	 return true;
  }

  if (tree->ne == NULL) {
  	tree->ne = new QuadTree;

  	tree->ne->lat_min = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);	
  	tree->ne->lat_max = tree->lat_max;
  	tree->ne->lon_min = tree->lon_min;
  	tree->ne->lon_max = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  } 

  if (QTInsert(tree->ne, line, depth++)){
  	return true;	
  } 	

  if (tree->se == NULL) {  
  	tree->se = new QuadTree;

  	tree->se->lat_min = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
  	tree->se->lat_max = tree->lat_max;
  	tree->se->lon_min = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  	tree->se->lon_max = tree->lon_max;
	}  

  if (QTInsert(tree->se, line, depth++)){
  	return true;	
	} 
	
  tree->lines.push_back(&(*line));

  return true;
}


std::vector<Line*> Map::getLinesRecursive(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max) {
    std::vector<Line*> retLines;

    if(tree == NULL) {
        return retLines;
    }

    if (tree->lat_min > screen_lat_max || screen_lat_min > tree->lat_max) {
        return retLines; 
    }

    if (tree->lon_min > screen_lon_max || screen_lon_min > tree->lon_max) {
        return retLines; 
    }

    std::vector<Line*> ret;
    ret = getLinesRecursive(tree->nw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    retLines.insert(retLines.end(), ret.begin(), ret.end());

    ret = getLinesRecursive(tree->sw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    retLines.insert(retLines.end(), ret.begin(), ret.end());

    ret = getLinesRecursive(tree->ne, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    retLines.insert(retLines.end(), ret.begin(), ret.end());

    ret = getLinesRecursive(tree->se, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    retLines.insert(retLines.end(), ret.begin(), ret.end());

    retLines.insert(retLines.end(), tree->lines.begin(), tree->lines.end());   

    // Debug quadtree
    // Point TL, TR, BL, BR;

    // TL.lat = tree->lat_min;
    // TL.lon = tree->lon_min;

    // TR.lat = tree->lat_max;
    // TR.lon = tree->lon_min;

    // BL.lat = tree->lat_min;
    // BL.lon = tree->lon_max;

    // BR.lat = tree->lat_max;
    // BR.lon = tree->lon_max;

    // retLines.push_back(new Line(TL,TR));
    // retLines.push_back(new Line(TR,BR));
    // retLines.push_back(new Line(BL,BR));
    // retLines.push_back(new Line(TL,BL));

    return retLines;
}

std::vector<Line*> Map::getLines(float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max) {
  return getLinesRecursive(&root, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
};

Map::Map() { 
  FILE *fileptr;

  if(!(fileptr = fopen("mapdata.bin", "rb"))) {
    printf("Couldn't read mapdata.bin\nDid you run getmap.sh?\n");
    exit(0);
  }  

  fseek(fileptr, 0, SEEK_END);
  mapPoints_count = ftell(fileptr) / sizeof(float);
  rewind(fileptr);                   

  mapPoints = (float *)malloc(mapPoints_count * sizeof(float)); 
  if(!fread(mapPoints, sizeof(float), mapPoints_count, fileptr)){
    printf("Read error\n");
    exit(0);
  } 

  fclose(fileptr);

  printf("Read %d map points.\n",mapPoints_count / 2);

  // load quad tree

	for(int i = 0; i < mapPoints_count; i+=2) {
		if(mapPoints[i] == 0)
			continue;

		if(mapPoints[i] < root.lon_min) {
			root.lon_min = mapPoints[i];
		} else if(mapPoints[i] > root.lon_max) {
			root.lon_max = mapPoints[i];
		} 

		if(mapPoints[i+1] < root.lat_min) {
			root.lat_min = mapPoints[i+1];
		} else if(mapPoints[i+1] > root.lat_max) {
			root.lat_max = mapPoints[i+1];
		} 
	}

  printf("map bounds: %f %f %f %f\n",root.lon_min, root.lon_max, root.lat_min, root.lat_max);

  Point currentPoint;
  Point nextPoint;

  for(int i = 0; i < mapPoints_count - 2; i+=2) {
    if(mapPoints[i] == 0)
      continue;
    if(mapPoints[i + 1] == 0)
      continue;
    if(mapPoints[i + 2] == 0)
      continue;
    if(mapPoints[i + 3] == 0)
      continue;
    currentPoint.lon = mapPoints[i];
    currentPoint.lat = mapPoints[i + 1];

    nextPoint.lon = mapPoints[i + 2];
    nextPoint.lat = mapPoints[i + 3];

    // printf("inserting [%f %f] -> [%f %f]\n",currentPoint.lon,currentPoint.lat,nextPoint.lon,nextPoint.lat);

    QTInsert(&root, new Line(currentPoint, nextPoint), 0);
	}

  printf("done\n");
}
