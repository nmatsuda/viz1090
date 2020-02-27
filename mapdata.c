#include "dump1090.h"
#include "mapdata.h"

#include <stdbool.h>

//sourced from http://www.mccurley.org/svg/
// 
//extern float mapPoints[3878131];

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
    	tree->nw->polygons = NULL;
    	tree->nw->nw = NULL;
		tree->nw->ne = NULL;
    	tree->nw->sw = NULL;
    	tree->nw->se = NULL;

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
    	tree->sw->polygons = NULL;
    	tree->sw->nw = NULL;
    	tree->sw->ne = NULL;
    	tree->sw->sw = NULL;
    	tree->sw->se = NULL;

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
      	tree->ne->polygons = NULL;
      	tree->ne->nw = NULL;
      	tree->ne->ne = NULL;
      	tree->ne->sw = NULL;
      	tree->ne->se = NULL;

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
      	tree->se->polygons = NULL;
      	tree->se->nw = NULL;
      	tree->se->ne = NULL;
		tree->se->sw = NULL;
		tree->se->se = NULL;

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
	// printf("insert done\n");
    return true;
}

void initMaps() { 
  mapPoints_count = sizeof(mapPoints) / sizeof(float);


  FILE *fileptr;

fileptr = fopen("mapdata.bin", "rb");  // Open the file in binary mode
fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
mapPoints_count = ftell(fileptr) / sizeof(float);             // Get the current byte offset in the file
rewind(fileptr);                      // Jump back to the beginning of the file

mapPoints = (float *)malloc(mapPoints_count * sizeof(float)); // Enough memory for the file
fread(mapPoints, sizeof(float), mapPoints_count, fileptr); // Read in the entire file
fclose(fileptr); // Close the fileptr

printf("%d points read\n",mapPoints_count);

  //mapPoints_relative = (float *) malloc(sizeof(mapPoints));

	// mapPoints_count = sizeof(mapPoints) / (2 * sizeof(double));
	// mapPoints_x = (double *) malloc(sizeof(mapPoints) / 2);
	// mapPoints_y = (double *) malloc(sizeof(mapPoints) / 2);

	// int current = 0;
	// for(int i = 0; i < 2 * mapPoints_count; i++) {
	// 	if(mapPoints[i] != 0) {
	// 		if(i%2 == 0) { //longitude points      
	// 			double dLon = mapPoints[i] - Modes.fUserLon;
	// 			mapPoints_x[current] = 6371.0 * dLon * M_PI / 180.0 * cos(((mapPoints[i+1] + Modes.fUserLat)/2.0) * M_PI / 180.0);      
	// 		} else { //latitude points
	// 			double dLat = mapPoints[i] - Modes.fUserLat;
	// 			mapPoints_y[current] = 6371.0 * dLat * M_PI / 180.0f;
	// 			current++;
	// 		}
	// 	}	
	// }

  	root.lat_min = 180;
  	root.lon_min = 180;
  	root.lat_max = -180;
  	root.lon_max = -180;

  	root.nw = NULL;
  	root.ne = NULL;
  	root.sw = NULL;
  	root.se = NULL;

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

	Polygon *currentPolygon = (Polygon*)malloc(sizeof(Polygon));

  	currentPolygon->lat_min = 180.0;
  	currentPolygon->lon_min = 180.0;
  	currentPolygon->lat_max = -180.0;
  	currentPolygon->lon_max = -180.0;

  	currentPolygon->numPoints = 0;

  	currentPolygon->points = NULL;
  	currentPolygon->next = NULL;

	for(int i = 0; i < mapPoints_count; i+=2) {

		if(mapPoints[i] == 0) { //end of polygon
			if(currentPolygon->numPoints != 7) 
				QTInsert(&root, currentPolygon);

			currentPolygon = (Polygon*)malloc(sizeof(Polygon));

		  	currentPolygon->lat_min = 180.0;
		  	currentPolygon->lon_min = 180.0;
		  	currentPolygon->lat_max = -180.0;
		  	currentPolygon->lon_max = -180.0;

		  	currentPolygon->numPoints = 0;
  			currentPolygon->points = NULL;
  			currentPolygon->next = NULL;
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


// void recenter() {
//   for(int i = 0; i < mapPoints_count; i++) {

//     if(mapPoints[i] == 0) {
//       mapPoints_relative[i] = 0;
//     } else {
//       if(i%2 == 0) { //longitude points      
//         double dLon = mapPoints[i] - Modes.fUserLon;
//         mapPoints_relative[i] = 6371.0 * dLon * M_PI / 180.0 * cos(((mapPoints[i+1] + Modes.fUserLat)/2.0) * M_PI / 180.0);      
//       } else { //latitude points
//         double dLat = mapPoints[i] - Modes.fUserLat;
//         mapPoints_relative[i] = 6371.0 * dLat * M_PI / 180.0f;
//       }
//     }
//   }
// }