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

#include "ui/Map.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
bool
Map::QTInsert(QuadTree* tree, Line* line, int depth) {

  // if(depth > 25) {
  //  // printf("fail [%f %f] -> [%f
  //  %f]\n",line->start.lon,line->start.lat,line->end.lon,line->end.lat);

  //  // printf("bounds %f %f %f %f\n",tree->lon_min, tree->lon_max, tree->lat_min, tree->lat_max);
  //   // fflush(stdout);
  //    tree->lines.push_back(&(*line));
  //    return true;
  // }

  bool startInside = line->start.lat >= tree->lat_min && line->start.lat <= tree->lat_max &&
                     line->start.lon >= tree->lon_min && line->start.lon <= tree->lon_max;

  bool endInside = line->end.lat >= tree->lat_min && line->end.lat <= tree->lat_max &&
                   line->end.lon >= tree->lon_min && line->end.lon <= tree->lon_max;

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

  if (QTInsert(tree->nw, line, depth++)) {
    return true;
  }

  if (tree->sw == NULL) {
    tree->sw = new QuadTree;

    tree->sw->lat_min = tree->lat_min;
    tree->sw->lat_max = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
    tree->sw->lon_min = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
    tree->sw->lon_max = tree->lon_max;
  }

  if (QTInsert(tree->sw, line, depth++)) {
    return true;
  }

  if (tree->ne == NULL) {
    tree->ne = new QuadTree;

    tree->ne->lat_min = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
    tree->ne->lat_max = tree->lat_max;
    tree->ne->lon_min = tree->lon_min;
    tree->ne->lon_max = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
  }

  if (QTInsert(tree->ne, line, depth++)) {
    return true;
  }

  if (tree->se == NULL) {
    tree->se = new QuadTree;

    tree->se->lat_min = tree->lat_min + 0.5 * (tree->lat_max - tree->lat_min);
    tree->se->lat_max = tree->lat_max;
    tree->se->lon_min = tree->lon_min + 0.5 * (tree->lon_max - tree->lon_min);
    tree->se->lon_max = tree->lon_max;
  }

  if (QTInsert(tree->se, line, depth++)) {
    return true;
  }

  tree->lines.push_back(&(*line));

  return true;
}

std::vector<Line*>
Map::getLinesRecursive(QuadTree* tree, float screen_lat_min, float screen_lat_max,
                       float screen_lon_min, float screen_lon_max) {
  std::vector<Line*> retLines;

  if (tree == NULL) {
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

std::vector<Line*>
Map::getLines(float screen_lat_min, float screen_lat_max, float screen_lon_min,
              float screen_lon_max) {
  return getLinesRecursive(&root, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
};

std::vector<Line*>
Map::getCountryLines(float screen_lat_min, float screen_lat_max, float screen_lon_min,
                     float screen_lon_max) {
  return getLinesRecursive(&country_root, screen_lat_min, screen_lat_max, screen_lon_min, screen_lon_max);
};

void
Map::load() {
  FILE* fileptr;

  // Load land polygon data
  if ((fileptr = fopen("landdata.bin", "rb"))) {
    fseek(fileptr, 0, SEEK_END);
    landPoints_count = ftell(fileptr) / sizeof(float);
    rewind(fileptr);

    landPoints = (float*)malloc(landPoints_count * sizeof(float));
    if (!fread(landPoints, sizeof(float), landPoints_count, fileptr)) {
      printf("Land data read error\n");
      exit(0);
    }

    fclose(fileptr);

    printf("Read %d land polygon points.\n", landPoints_count / 2);

    // Parse land points into polygons (0,0 is terminator between polygons)
    Polygon currentPoly;
    for (int i = 0; i < landPoints_count; i += 2) {
      if (landPoints[i] == 0 && landPoints[i + 1] == 0) {
        // End of polygon
        if (!currentPoly.vertices.empty()) {
          currentPoly.computeBounds();
          landPolygons.push_back(currentPoly);
          currentPoly.vertices.clear();
        }
      } else {
        Point p;
        p.lon = landPoints[i];
        p.lat = landPoints[i + 1];
        currentPoly.vertices.push_back(p);
      }
    }
    // Don't forget last polygon if no trailing terminator
    if (!currentPoly.vertices.empty()) {
      currentPoly.computeBounds();
      landPolygons.push_back(currentPoly);
    }

    printf("Parsed %zu land polygons.\n", landPolygons.size());
  } else {
    printf("No land data file found\n");
  }

  if ((fileptr = fopen("mapdata.bin", "rb"))) {

    fseek(fileptr, 0, SEEK_END);
    mapPoints_count = ftell(fileptr) / sizeof(float);
    rewind(fileptr);

    mapPoints = (float*)malloc(mapPoints_count * sizeof(float));
    if (!fread(mapPoints, sizeof(float), mapPoints_count, fileptr)) {
      printf("Map read error\n");
      exit(0);
    }

    fclose(fileptr);

    printf("Read %d map points.\n", mapPoints_count / 2);
  }

  if ((fileptr = fopen("countrydata.bin", "rb"))) {
    fseek(fileptr, 0, SEEK_END);
    countryPoints_count = ftell(fileptr) / sizeof(float);
    rewind(fileptr);

    countryPoints = (float*)malloc(countryPoints_count * sizeof(float));
    if (!fread(countryPoints, sizeof(float), countryPoints_count, fileptr)) {
      printf("Country data read error\n");
      exit(0);
    }

    fclose(fileptr);

    printf("Read %d country boundary points.\n", countryPoints_count / 2);
  }

  if ((fileptr = fopen("coastlinedata.bin", "rb"))) {
    fseek(fileptr, 0, SEEK_END);
    coastlinePoints_count = ftell(fileptr) / sizeof(float);
    rewind(fileptr);

    coastlinePoints = (float*)malloc(coastlinePoints_count * sizeof(float));
    if (!fread(coastlinePoints, sizeof(float), coastlinePoints_count, fileptr)) {
      printf("Coastline data read error\n");
      exit(0);
    }

    fclose(fileptr);

    printf("Read %d coastline points.\n", coastlinePoints_count / 2);
  }

  if ((fileptr = fopen("riverdata.bin", "rb"))) {
    fseek(fileptr, 0, SEEK_END);
    riverPoints_count = ftell(fileptr) / sizeof(float);
    rewind(fileptr);

    riverPoints = (float*)malloc(riverPoints_count * sizeof(float));
    if (!fread(riverPoints, sizeof(float), riverPoints_count, fileptr)) {
      printf("River data read error\n");
      exit(0);
    }

    fclose(fileptr);

    printf("Read %d river points.\n", riverPoints_count / 2);
  }

  if ((fileptr = fopen("lakedata.bin", "rb"))) {
    fseek(fileptr, 0, SEEK_END);
    lakePoints_count = ftell(fileptr) / sizeof(float);
    rewind(fileptr);

    lakePoints = (float*)malloc(lakePoints_count * sizeof(float));
    if (!fread(lakePoints, sizeof(float), lakePoints_count, fileptr)) {
      printf("Lake data read error\n");
      exit(0);
    }

    fclose(fileptr);

    printf("Read %d lake points.\n", lakePoints_count / 2);
  }

  if ((fileptr = fopen("airportdata.bin", "rb"))) {
    fseek(fileptr, 0, SEEK_END);
    airportPoints_count = ftell(fileptr) / sizeof(float);
    rewind(fileptr);

    airportPoints = (float*)malloc(airportPoints_count * sizeof(float));
    if (!fread(airportPoints, sizeof(float), airportPoints_count, fileptr)) {
      printf("Map read error\n");
      exit(0);
    }

    fclose(fileptr);

    printf("Read %d airport points.\n", airportPoints_count / 2);
  }

  int total = mapPoints_count / 2 + countryPoints_count / 2 + coastlinePoints_count / 2 +
              riverPoints_count / 2 + lakePoints_count / 2 + airportPoints_count / 2;
  int processed = 0;

  // load quad tree
  if (mapPoints_count > 0) {
    for (int i = 0; i < mapPoints_count; i += 2) {
      if (mapPoints[i] == 0)
        continue;

      if (mapPoints[i] < root.lon_min) {
        root.lon_min = mapPoints[i];
      } else if (mapPoints[i] > root.lon_max) {
        root.lon_max = mapPoints[i];
      }

      if (mapPoints[i + 1] < root.lat_min) {
        root.lat_min = mapPoints[i + 1];
      } else if (mapPoints[i + 1] > root.lat_max) {
        root.lat_max = mapPoints[i + 1];
      }
    }

    // printf("map bounds: %f %f %f %f\n",root.lon_min, root.lon_max, root.lat_min, root.lat_max);

    Point currentPoint;
    Point nextPoint;

    for (int i = 0; i < mapPoints_count - 2; i += 2) {
      if (mapPoints[i] == 0)
        continue;
      if (mapPoints[i + 1] == 0)
        continue;
      if (mapPoints[i + 2] == 0)
        continue;
      if (mapPoints[i + 3] == 0)
        continue;
      currentPoint.lon = mapPoints[i];
      currentPoint.lat = mapPoints[i + 1];

      nextPoint.lon = mapPoints[i + 2];
      nextPoint.lat = mapPoints[i + 3];

      // printf("inserting [%f %f] -> [%f
      // %f]\n",currentPoint.lon,currentPoint.lat,nextPoint.lon,nextPoint.lat);

      QTInsert(&root, new Line(currentPoint, nextPoint), 0);

      processed++;

      loaded = floor(100.0f * (float)processed / (float)total);
    }
  } else {
    printf("No map file found\n");
  }

  // load country boundaries quad tree
  if (countryPoints_count > 0) {
    for (int i = 0; i < countryPoints_count; i += 2) {
      if (countryPoints[i] == 0)
        continue;

      if (countryPoints[i] < country_root.lon_min) {
        country_root.lon_min = countryPoints[i];
      } else if (countryPoints[i] > country_root.lon_max) {
        country_root.lon_max = countryPoints[i];
      }

      if (countryPoints[i + 1] < country_root.lat_min) {
        country_root.lat_min = countryPoints[i + 1];
      } else if (countryPoints[i + 1] > country_root.lat_max) {
        country_root.lat_max = countryPoints[i + 1];
      }
    }

    Point currentPoint;
    Point nextPoint;

    for (int i = 0; i < countryPoints_count - 2; i += 2) {
      if (countryPoints[i] == 0)
        continue;
      if (countryPoints[i + 1] == 0)
        continue;
      if (countryPoints[i + 2] == 0)
        continue;
      if (countryPoints[i + 3] == 0)
        continue;
      currentPoint.lon = countryPoints[i];
      currentPoint.lat = countryPoints[i + 1];

      nextPoint.lon = countryPoints[i + 2];
      nextPoint.lat = countryPoints[i + 3];

      QTInsert(&country_root, new Line(currentPoint, nextPoint), 0);

      processed++;

      loaded = floor(100.0f * (float)processed / (float)total);
    }
  } else {
    printf("No country boundaries file found\n");
  }

  // load coastline quad tree
  if (coastlinePoints_count > 0) {
    for (int i = 0; i < coastlinePoints_count; i += 2) {
      if (coastlinePoints[i] == 0)
        continue;

      if (coastlinePoints[i] < coastline_root.lon_min) {
        coastline_root.lon_min = coastlinePoints[i];
      } else if (coastlinePoints[i] > coastline_root.lon_max) {
        coastline_root.lon_max = coastlinePoints[i];
      }

      if (coastlinePoints[i + 1] < coastline_root.lat_min) {
        coastline_root.lat_min = coastlinePoints[i + 1];
      } else if (coastlinePoints[i + 1] > coastline_root.lat_max) {
        coastline_root.lat_max = coastlinePoints[i + 1];
      }
    }

    Point currentPoint;
    Point nextPoint;

    for (int i = 0; i < coastlinePoints_count - 2; i += 2) {
      if (coastlinePoints[i] == 0)
        continue;
      if (coastlinePoints[i + 1] == 0)
        continue;
      if (coastlinePoints[i + 2] == 0)
        continue;
      if (coastlinePoints[i + 3] == 0)
        continue;
      currentPoint.lon = coastlinePoints[i];
      currentPoint.lat = coastlinePoints[i + 1];

      nextPoint.lon = coastlinePoints[i + 2];
      nextPoint.lat = coastlinePoints[i + 3];

      QTInsert(&coastline_root, new Line(currentPoint, nextPoint), 0);

      processed++;

      loaded = floor(100.0f * (float)processed / (float)total);
    }
  } else {
    printf("No coastline file found\n");
  }

  // load river quad tree
  if (riverPoints_count > 0) {
    for (int i = 0; i < riverPoints_count; i += 2) {
      if (riverPoints[i] == 0)
        continue;

      if (riverPoints[i] < river_root.lon_min) {
        river_root.lon_min = riverPoints[i];
      } else if (riverPoints[i] > river_root.lon_max) {
        river_root.lon_max = riverPoints[i];
      }

      if (riverPoints[i + 1] < river_root.lat_min) {
        river_root.lat_min = riverPoints[i + 1];
      } else if (riverPoints[i + 1] > river_root.lat_max) {
        river_root.lat_max = riverPoints[i + 1];
      }
    }

    Point currentPoint;
    Point nextPoint;

    for (int i = 0; i < riverPoints_count - 2; i += 2) {
      if (riverPoints[i] == 0)
        continue;
      if (riverPoints[i + 1] == 0)
        continue;
      if (riverPoints[i + 2] == 0)
        continue;
      if (riverPoints[i + 3] == 0)
        continue;
      currentPoint.lon = riverPoints[i];
      currentPoint.lat = riverPoints[i + 1];

      nextPoint.lon = riverPoints[i + 2];
      nextPoint.lat = riverPoints[i + 3];

      QTInsert(&river_root, new Line(currentPoint, nextPoint), 0);

      processed++;

      loaded = floor(100.0f * (float)processed / (float)total);
    }
  } else {
    printf("No river file found\n");
  }

  // load lake quad tree
  if (lakePoints_count > 0) {
    for (int i = 0; i < lakePoints_count; i += 2) {
      if (lakePoints[i] == 0)
        continue;

      if (lakePoints[i] < lake_root.lon_min) {
        lake_root.lon_min = lakePoints[i];
      } else if (lakePoints[i] > lake_root.lon_max) {
        lake_root.lon_max = lakePoints[i];
      }

      if (lakePoints[i + 1] < lake_root.lat_min) {
        lake_root.lat_min = lakePoints[i + 1];
      } else if (lakePoints[i + 1] > lake_root.lat_max) {
        lake_root.lat_max = lakePoints[i + 1];
      }
    }

    Point currentPoint;
    Point nextPoint;

    for (int i = 0; i < lakePoints_count - 2; i += 2) {
      if (lakePoints[i] == 0)
        continue;
      if (lakePoints[i + 1] == 0)
        continue;
      if (lakePoints[i + 2] == 0)
        continue;
      if (lakePoints[i + 3] == 0)
        continue;
      currentPoint.lon = lakePoints[i];
      currentPoint.lat = lakePoints[i + 1];

      nextPoint.lon = lakePoints[i + 2];
      nextPoint.lat = lakePoints[i + 3];

      QTInsert(&lake_root, new Line(currentPoint, nextPoint), 0);

      processed++;

      loaded = floor(100.0f * (float)processed / (float)total);
    }
  } else {
    printf("No lake file found\n");
  }

  // load airport quad tree
  if (airportPoints_count > 0) {
    for (int i = 0; i < airportPoints_count; i += 2) {
      if (airportPoints[i] == 0)
        continue;

      if (airportPoints[i] < airport_root.lon_min) {
        airport_root.lon_min = airportPoints[i];
      } else if (airportPoints[i] > airport_root.lon_max) {
        airport_root.lon_max = airportPoints[i];
      }

      if (airportPoints[i + 1] < airport_root.lat_min) {
        airport_root.lat_min = airportPoints[i + 1];
      } else if (airportPoints[i + 1] > airport_root.lat_max) {
        airport_root.lat_max = airportPoints[i + 1];
      }
    }

    // printf("map bounds: %f %f %f %f\n",root.lon_min, root.lon_max, root.lat_min, root.lat_max);

    Point currentPoint;
    Point nextPoint;

    for (int i = 0; i < airportPoints_count - 2; i += 2) {
      if (airportPoints[i] == 0)
        continue;
      if (airportPoints[i + 1] == 0)
        continue;
      if (airportPoints[i + 2] == 0)
        continue;
      if (airportPoints[i + 3] == 0)
        continue;
      currentPoint.lon = airportPoints[i];
      currentPoint.lat = airportPoints[i + 1];

      nextPoint.lon = airportPoints[i + 2];
      nextPoint.lat = airportPoints[i + 3];

      // printf("inserting [%f %f] -> [%f
      // %f]\n",currentPoint.lon,currentPoint.lat,nextPoint.lon,nextPoint.lat);

      QTInsert(&airport_root, new Line(currentPoint, nextPoint), 0);

      processed++;

      loaded = floor(100.0f * (float)processed / (float)total);
    }
  } else {
    printf("No airport file found\n");
  }

  //

  std::string line;
  std::ifstream infile("mapnames");

  while (std::getline(infile, line)) {
    float lon, lat;

    std::istringstream iss(line);

    iss >> lon;
    iss >> lat;

    std::string assemble;

    iss >> assemble;

    for (std::string s; iss >> s;) {
      assemble = assemble + " " + s;
    }

    // std::cout << "[" << x << "," << y << "] " << assemble << "\n";
    MapLabel* label = new MapLabel(lon, lat, assemble);
    mapnames.push_back(label);
  }

  std::cout << "Read " << mapnames.size() << " place names\n";

  infile.close();

  infile.open("airportnames");

  while (std::getline(infile, line)) {
    float lon, lat;

    std::istringstream iss(line);

    iss >> lon;
    iss >> lat;

    std::string assemble;

    iss >> assemble;

    for (std::string s; iss >> s;) {
      assemble = assemble + " " + s;
    }

    // std::cout << "[" << x << "," << y << "] " << assemble << "\n";
    MapLabel* label = new MapLabel(lon, lat, assemble);
    airportnames.push_back(label);
  }

  std::cout << "Read " << airportnames.size() << " airport names\n";

  infile.close();

  printf("done\n");

  loaded = 100;
}

Map::Map() {
  loaded = 0;

  landPoints_count = 0;
  landPoints = NULL;

  mapPoints_count = 0;
  mapPoints = NULL;

  countryPoints_count = 0;
  countryPoints = NULL;

  coastlinePoints_count = 0;
  coastlinePoints = NULL;

  riverPoints_count = 0;
  riverPoints = NULL;

  lakePoints_count = 0;
  lakePoints = NULL;

  airportPoints_count = 0;
  airportPoints = NULL;
}
