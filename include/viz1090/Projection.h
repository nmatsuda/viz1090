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

#ifndef VIZ1090_PROJECTION_H
#define VIZ1090_PROJECTION_H

#include <cmath>

#include "Types.h"

namespace viz1090 {

/// Screen coordinate
struct ScreenPoint {
  int x;
  int y;
};

/// Floating-point offset from center
struct Offset {
  float dx;
  float dy;
};

/// Map projection for converting between geographic and screen coordinates
///
/// Uses a simple equirectangular projection with latitude correction.
class Projection {
public:
  /// Kilometers per degree of latitude
  static constexpr double kKmPerDegree = 111.195;  // 6371.0 * M_PI / 180.0

  Projection() = default;

  /// Set the screen dimensions
  void setScreenSize(int aWidth, int aHeight) {
    mScreenWidth = aWidth;
    mScreenHeight = aHeight;
    updateMultipliers();
  }

  /// Set the map center position
  void setCenter(double aLat, double aLon) {
    mCenterLat = aLat;
    mCenterLon = aLon;
    updateMultipliers();
  }

  /// Set the visible distance from center (in km)
  void setMaxDistance(double aDistance) {
    mMaxDistance = aDistance;
    updateMultipliers();
  }

  /// Get current center latitude
  [[nodiscard]] double centerLat() const { return mCenterLat; }

  /// Get current center longitude
  [[nodiscard]] double centerLon() const { return mCenterLon; }

  /// Get current max distance
  [[nodiscard]] double maxDistance() const { return mMaxDistance; }

  /// Convert geographic coordinates to screen coordinates
  /// @param aPos Geographic position (lat/lon)
  /// @return Screen coordinates (pixels from top-left)
  [[nodiscard]] ScreenPoint toScreen(const Position& aPos) const {
    Offset offset = toOffset(aPos);
    return toScreen(offset);
  }

  /// Convert geographic coordinates to offset from center
  /// @param aPos Geographic position
  /// @return Offset in projected units
  [[nodiscard]] Offset toOffset(const Position& aPos) const {
    return Offset{
        static_cast<float>((aPos.longitude - mCenterLon) * mDxMult),
        static_cast<float>((mCenterLat - aPos.latitude) * mDyMult)};
  }

  /// Convert offset to screen coordinates
  /// @param aOffset Offset from center
  /// @return Screen coordinates
  [[nodiscard]] ScreenPoint toScreen(const Offset& aOffset) const {
    return ScreenPoint{
        static_cast<int>(mScreenWidth / 2 + aOffset.dx),
        static_cast<int>(mScreenHeight / 2 + aOffset.dy)};
  }

  /// Convert screen coordinates to geographic position
  /// @param aScreen Screen coordinates
  /// @return Geographic position
  [[nodiscard]] Position toPosition(const ScreenPoint& aScreen) const {
    float dx = static_cast<float>(aScreen.x - mScreenWidth / 2);
    float dy = static_cast<float>(aScreen.y - mScreenHeight / 2);

    return Position{
        mCenterLat - dy / mDyMult,
        mCenterLon + dx / mDxMult};
  }

  /// Convert a distance to screen pixels
  /// @param aDistanceKm Distance in kilometers
  /// @return Distance in pixels
  [[nodiscard]] int distanceToPixels(double aDistanceKm) const {
    double degreesLat = aDistanceKm / kKmPerDegree;
    return static_cast<int>(degreesLat * mDyMult);
  }

  /// Check if screen point is within visible bounds
  [[nodiscard]] bool isVisible(const ScreenPoint& aPoint) const {
    return aPoint.x >= 0 && aPoint.x < mScreenWidth && aPoint.y >= 0 &&
           aPoint.y < mScreenHeight;
  }

  /// Check if screen point is within specified bounds
  [[nodiscard]] bool isVisible(const ScreenPoint& aPoint, int aLeft, int aTop,
                               int aRight, int aBottom) const {
    return aPoint.x >= aLeft && aPoint.x < aRight && aPoint.y >= aTop &&
           aPoint.y < aBottom;
  }

  /// Get visible latitude range
  [[nodiscard]] double visibleLatRange() const {
    return static_cast<double>(mScreenHeight) / mDyMult;
  }

  /// Get visible longitude range
  [[nodiscard]] double visibleLonRange() const {
    return static_cast<double>(mScreenWidth) / mDxMult;
  }

private:
  void updateMultipliers() {
    if (mMaxDistance <= 0 || mScreenHeight <= 0) {
      mDxMult = 1.0f;
      mDyMult = 1.0f;
      return;
    }

    // Calculate pixels per degree of latitude
    double screenRadius = static_cast<double>(mScreenHeight) / 2.0;
    double degreesVisible = mMaxDistance / kKmPerDegree;
    mDyMult = static_cast<float>(screenRadius / degreesVisible);

    // Adjust longitude multiplier for latitude (Mercator-like correction)
    double latRadians = mCenterLat * M_PI / 180.0;
    mDxMult = mDyMult * static_cast<float>(std::cos(latRadians));
  }

  int mScreenWidth = 800;
  int mScreenHeight = 600;
  double mCenterLat = 0.0;
  double mCenterLon = 0.0;
  double mMaxDistance = 200.0;  // km
  float mDxMult = 1.0f;
  float mDyMult = 1.0f;
};

}  // namespace viz1090

#endif  // VIZ1090_PROJECTION_H
