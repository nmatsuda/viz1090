#include "dump1090.h"
#include "mapdata.h"
#include "structs.h"
#include <stdbool.h>

int mapPoints_count;
float *mapPoints;

void initQuadTree(QuadTree *tree) {
  if(tree == NULL) {
    return;
  }

  tree->polygons = NULL;
  tree->nw = NULL;
  tree->ne = NULL;
  tree->sw = NULL;
  tree->se = NULL;
}

void initPolygon(Polygon *currentPolygon) {
  if(currentPolygon == NULL) {
    return;
  }

  currentPolygon->lat_min = 180.0;
  currentPolygon->lon_min = 180.0;
  currentPolygon->lat_max = -180.0;
  currentPolygon->lon_max = -180.0;
  currentPolygon->numPoints = 0;
  currentPolygon->points = NULL;
  currentPolygon->next = NULL;
}

bool QTInsert(QuadTree *tree, Polygon* polygon) {
    // printf("Inserting %d point poly\n", polygon->numPoints);

  if (!(polygon->lat_min >= tree->lat_min &&
  	polygon->lat_max <= tree->lat_max &&
  	polygon->lon_min >= tree->lon_min &&
  	polygon->lon_max <=	 tree->lon_max)) {
  	// printf("doesnt fit: %f > %f, %f < %f, %f < %f,%f > %f \n",polygon->lat_min, tree->lat_min, polygon->lat_max, tree->lat_max, polygon->lon_min, tree->lon_min, polygon->lon_max,tree->lon_max);

  	return false;	
  }
        
  if (tree->nw == NULL) {
  	tree->nw = (QuadTree*)malloc(sizeof(QuadTree));
    initQuadTree(tree->nw);

  	tree->nw->lat_min = tree->lat_min;	
  	tree->nw->lat_max = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
  	tree->nw->lon_min = tree->lon_min;
  	tree->nw->lon_max = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  }

  if (QTInsert(tree->nw,polygon)){
  	return true;
  }

  if (tree->sw == NULL) {
  	tree->sw = (QuadTree*)malloc(sizeof(QuadTree));
    initQuadTree(tree->sw);

  	tree->sw->lat_min = tree->lat_min;	
  	tree->sw->lat_max = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
  	tree->sw->lon_min = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  	tree->sw->lon_max = tree->lon_max;
  }

	if (QTInsert(tree->sw,polygon)){
	 return true;
  }

  if (tree->ne == NULL) {
  	tree->ne = (QuadTree*)malloc(sizeof(QuadTree));
    initQuadTree(tree->ne);

  	tree->ne->lat_min = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);	
  	tree->ne->lat_max = tree->lat_max;
  	tree->ne->lon_min = tree->lon_min;
  	tree->ne->lon_max = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  } 

  if (QTInsert(tree->ne,polygon)){
  	return true;	
  } 	

  if (tree->se == NULL) {  
  	tree->se = (QuadTree*)malloc(sizeof(QuadTree));
    initQuadTree(tree->se);

  	tree->se->lat_min = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
  	tree->se->lat_max = tree->lat_max;
  	tree->se->lon_min = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  	tree->se->lon_max = tree->lon_max;
	}  

  if (QTInsert(tree->se,polygon)){
  	return true;	
	} 
	
  polygon->next = tree->polygons;
  tree->polygons = polygon;

  return true;
}

void initMaps() { 
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

	appData.root.lat_min = 180;
	appData.root.lon_min = 180;
	appData.root.lat_max = -180;
	appData.root.lon_max = -180;

	appData.root.nw = NULL;
	appData.root.ne = NULL;
	appData.root.sw = NULL;
	appData.root.se = NULL;

	for(int i = 0; i < mapPoints_count; i+=2) {
		if(mapPoints[i] == 0)
			continue;

		if(mapPoints[i] < appData.root.lon_min) {
			appData.root.lon_min = mapPoints[i];
		} else if(mapPoints[i] > appData.root.lon_max) {
			appData.root.lon_max = mapPoints[i];
		} 

		if(mapPoints[i+1] < appData.root.lat_min) {
			appData.root.lat_min = mapPoints[i+1];
		} else if(mapPoints[i+1] > appData.root.lat_max) {
			appData.root.lat_max = mapPoints[i+1];
		} 
	}

  Polygon *currentPolygon = (Polygon*)malloc(sizeof(Polygon));
  initPolygon(currentPolygon);

  for(int i = 0; i < mapPoints_count; i+=2) {

    if(mapPoints[i] == 0) {
        QTInsert(&appData.root, currentPolygon);
        currentPolygon = (Polygon*)malloc(sizeof(Polygon));
        initPolygon(currentPolygon);
        continue;
    }

    currentPolygon->numPoints++;

		Point *currentPoint = (Point*)malloc(sizeof(Point));

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

		currentPoint->next  = currentPolygon->points;
		currentPolygon->points = currentPoint;
	}
}
