#ifndef AIRCRAFT_LABEL_H
#define AIRCRAFT_LABEL_H

#include "SDL2/SDL_ttf.h"
#include <chrono>
#include <string>

#include "ui/Label.h"
#include "style/Style.h"

class Aircraft;
class AircraftList;

class AircraftLabel {
public:
  void update();
  void clearAcceleration();
  void calculateForces(const AircraftList& aircraftList);
  void applyForces();
  void move(float dx, float dy);
  void syncToAircraftPosition();  // Reposition label based on aircraft's current screen position
  bool getIsChanging();

  void draw(SDL_Renderer* renderer, bool selected);

  AircraftLabel(Aircraft* p, bool& metric, int screen_width, int screen_height, TTF_Font* font,
                const Style& style);

  // Global label density multiplier (controls how aggressively labels are hidden)
  static float getDensityMult() { return densityMult_; }
  static void setDensityMult(float value);
  static void adjustDensityMult(float delta);

  // Flag to force immediate density recalculation (bypasses timing check)
  static bool densityChanged() { return densityChanged_; }
  static void clearDensityChanged() { densityChanged_ = false; }

  // Global toggle for showing/hiding all labels
  static bool getShowLabels() { return showLabels_; }
  static void setShowLabels(bool show) { showLabels_ = show; }
  static void toggleShowLabels() { showLabels_ = !showLabels_; }

private:
  SDL_Rect getFullRect(int labelLevel);
  float calculateDensity(const AircraftList& aircraftList, int labelLevel);

  Aircraft* p;

  Label flightLabel;
  Label altitudeLabel;
  Label speedLabel;
  Label debugLabel;

  float labelLevel;

  bool& metric;

  float x;
  float y;
  float w;
  float h;

  float target_w;
  float target_h;

  float dx;
  float dy;

  float x_buffer[15];
  float y_buffer[15];
  int buffer_idx;
  int buffer_length = 15;

  float ddx;
  float ddy;

  float opacity;
  float target_opacity;

  float pressure;

  int screen_width;
  int screen_height;

  bool isChanging;

  // Last known aircraft screen position (for detecting view changes)
  float lastAircraftX;
  float lastAircraftY;

  std::chrono::high_resolution_clock::time_point lastLevelChange;

  ///////////

  float label_force = 0.01f;
  float label_dist = 2.0f;
  float density_force = 0.01f;
  float attachment_force = 0.01f;
  float attachment_dist = 10.0f;
  float icon_force = 0.01f;
  float icon_dist = 15.0f;
  float boundary_force = 0.01f;
  float damping_force = 0.65f;
  float velocity_limit = 1.0f;
  float edge_margin = 15.0f;
  float drag_force = 0.00f;

  const Style& style;

  // Static density multiplier shared across all labels
  static float densityMult_;
  static bool densityChanged_;
  static bool showLabels_;
};

#endif  // AIRCRAFT_LABEL_H
