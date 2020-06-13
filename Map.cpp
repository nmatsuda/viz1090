#include "Map.h"
#include <stdio.h>
#include <cstdlib>

bool Map::QTInsert(QuadTree *tree, Line *line, int depth) {
  // printf("Inserting %d point poly\n", line->numPoints);

  
  if (!(line->lat_min >= tree->lat_min &&
  	line->lat_max <= tree->lat_max &&
  	line->lon_min >= tree->lon_min &&
  	line->lon_max <=	 tree->lon_max)) {
  	// printf("doesnt fit: %f > %f, %f < %f, %f < %f,%f > %f \n",line->lat_min, tree->lat_min, line->lat_max, tree->lat_max, line->lon_min, tree->lon_min, line->lon_max,tree->lon_max);

  	return false;	
  }
        

  // //temp maxdepth for debugging
  // if(depth > 20) {
  //   tree->lines.push_back(*line);
  //   return true;
  // }

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
	
  tree->lines.push_back(*line);
  return true;
}


std::vector<Line> Map::getLinesRecursive(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max) {
    std::vector<Line> retLines;

    if(tree == NULL) {
        return retLines;
    }

    if (tree->lat_min > screen_lat_max || screen_lat_min > tree->lat_max) {
        return retLines; 
    }

    if (tree->lon_min > screen_lon_max || screen_lon_min > tree->lon_max) {
        return retLines; 
    }

    std::vector<Line> ret;
    ret = getLinesRecursive(tree->nw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    retLines.insert(retLines.end(), ret.begin(), ret.end());

    ret = getLinesRecursive(tree->sw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    retLines.insert(retLines.end(), ret.begin(), ret.end());

    ret = getLinesRecursive(tree->ne, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    retLines.insert(retLines.end(), ret.begin(), ret.end());

    ret = getLinesRecursive(tree->se, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
    retLines.insert(retLines.end(), ret.begin(), ret.end());

    retLines.insert(retLines.end(), tree->lines.begin(), tree->lines.end());   

    return retLines;
}

std::vector<Line> Map::getLines(float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max) {
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

  Point currentPoint;
  Point nextPoint;

  for(int i = 0; i < mapPoints_count - 2; i+=2) {
    if(mapPoints[i] == 0)
      continue;
    currentPoint.lon = mapPoints[i];
    currentPoint.lat = mapPoints[i + 1];

    nextPoint.lon = mapPoints[i + 2];
    nextPoint.lat = mapPoints[i + 3];

    QTInsert(&root, new Line(currentPoint, nextPoint), 0);
	}

}
