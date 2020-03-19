#include "Map.h"
#include <stdio.h>
#include <cstdlib>

bool Map::QTInsert(QuadTree *tree, Polygon *polygon) {
    // printf("Inserting %d point poly\n", polygon->numPoints);

  if (!(polygon->lat_min >= tree->lat_min &&
  	polygon->lat_max <= tree->lat_max &&
  	polygon->lon_min >= tree->lon_min &&
  	polygon->lon_max <=	 tree->lon_max)) {
  	// printf("doesnt fit: %f > %f, %f < %f, %f < %f,%f > %f \n",polygon->lat_min, tree->lat_min, polygon->lat_max, tree->lat_max, polygon->lon_min, tree->lon_min, polygon->lon_max,tree->lon_max);

  	return false;	
  }
        
  if (tree->nw == NULL) {
  	tree->nw = new QuadTree;

  	tree->nw->lat_min = tree->lat_min;	
  	tree->nw->lat_max = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
  	tree->nw->lon_min = tree->lon_min;
  	tree->nw->lon_max = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  }

  if (QTInsert(tree->nw,polygon)){
  	return true;
  }

  if (tree->sw == NULL) {
  	tree->sw = new QuadTree;

  	tree->sw->lat_min = tree->lat_min;	
  	tree->sw->lat_max = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
  	tree->sw->lon_min = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  	tree->sw->lon_max = tree->lon_max;
  }

	if (QTInsert(tree->sw,polygon)){
	 return true;
  }

  if (tree->ne == NULL) {
  	tree->ne = new QuadTree;

  	tree->ne->lat_min = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);	
  	tree->ne->lat_max = tree->lat_max;
  	tree->ne->lon_min = tree->lon_min;
  	tree->ne->lon_max = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  } 

  if (QTInsert(tree->ne,polygon)){
  	return true;	
  } 	

  if (tree->se == NULL) {  
  	tree->se = new QuadTree;

  	tree->se->lat_min = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
  	tree->se->lat_max = tree->lat_max;
  	tree->se->lon_min = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  	tree->se->lon_max = tree->lon_max;
	}  

  if (QTInsert(tree->se,polygon)){
  	return true;	
	} 
	
  tree->polygons.push_back(*polygon);

  return true;
}


std::list<Polygon> Map::getPolysRecursive(QuadTree *tree, float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max) {
    std::list<Polygon> retPolys;

    if(tree == NULL) {
        return retPolys;
    }

    if (tree->lat_min > screen_lat_max || screen_lat_min > tree->lat_max) {
        return retPolys; 
    }

    if (tree->lon_min > screen_lon_max || screen_lon_min > tree->lon_max) {
        return retPolys; 
    }

    retPolys.splice(retPolys.end(),getPolysRecursive(tree->nw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max));
    retPolys.splice(retPolys.end(),getPolysRecursive(tree->sw, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max));
    retPolys.splice(retPolys.end(),getPolysRecursive(tree->ne, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max));
    retPolys.splice(retPolys.end(),getPolysRecursive(tree->se, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max));

    float dx, dy;

    std::list<Polygon>::iterator currentPolygon;

    for (currentPolygon = tree->polygons.begin(); currentPolygon != tree->polygons.end(); ++currentPolygon) {
        if(currentPolygon->points.empty()) {
          continue;
        }

        retPolys.push_back(*currentPolygon);
    }
}

std::list<Polygon> Map::getPolys(float screen_lat_min, float screen_lat_max, float screen_lon_min, float screen_lon_max) {
  return getPolysRecursive(&root, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
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

  printf("Read %d map points.\n",mapPoints_count);

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

  Polygon *currentPolygon = new Polygon;

  for(int i = 0; i < mapPoints_count; i+=2) {

    if(mapPoints[i] == 0) {
        QTInsert(&root, currentPolygon);
        currentPolygon = new Polygon;
        continue;
    }

    currentPolygon->numPoints++;

		Point *currentPoint = new Point;

		if(mapPoints[i] < currentPolygon->lon_min) {
			currentPolygon->lon_min = mapPoints[i];
		} else if(mapPoints[i] > currentPolygon->lon_max) {
			currentPolygon->lon_max = mapPoints[i];
		} 

		if(mapPoints[i+1] < currentPolygon->lat_min) {
			currentPolygon->lat_min = mapPoints[i+1];
		} else if(mapPoints[i+1] > currentPolygon->lat_max) {
			currentPolygon->lat_max = mapPoints[i+1];
		} 
		
		currentPoint->lon = mapPoints[i];
		currentPoint->lat = mapPoints[i+1]; 

    currentPolygon->points.push_back(*currentPoint);
	}
}
