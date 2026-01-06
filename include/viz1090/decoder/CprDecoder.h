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

#ifndef VIZ1090_DECODER_CPR_DECODER_H
#define VIZ1090_DECODER_CPR_DECODER_H

#include <cmath>
#include <optional>

#include "../Types.h"

namespace viz1090::decoder {

/// Compact Position Reporting (CPR) decoder
///
/// Decodes aircraft positions from CPR-encoded latitude/longitude.
/// Supports both global (using odd+even frames) and relative decoding.
///
/// Algorithm from: http://www.lll.lu/~edward/edward/adsb/DecodingADSBposition.html
class CprDecoder {
public:
  /// CPR frame data
  struct CprFrame {
    int latitude;   // Raw CPR latitude (17-bit)
    int longitude;  // Raw CPR longitude (17-bit)
    uint64_t timestamp;
  };

  /// Decode position using global method (requires both odd and even frames)
  /// @param aEvenFrame Even CPR frame
  /// @param aOddFrame Odd CPR frame
  /// @param aUseOdd If true, use odd frame for final position
  /// @param aSurface If true, aircraft is on ground
  /// @param aRefLat Reference latitude for surface positions
  /// @param aRefLon Reference longitude for surface positions
  /// @return Decoded position, or nullopt if decode fails
  [[nodiscard]] static std::optional<Position> decodeGlobal(
      const CprFrame& aEvenFrame,
      const CprFrame& aOddFrame,
      bool aUseOdd,
      bool aSurface = false,
      double aRefLat = 0.0,
      double aRefLon = 0.0);

  /// Decode position using relative method (requires reference position)
  /// @param aFrame CPR frame to decode
  /// @param aOdd If true, frame is odd; if false, frame is even
  /// @param aRefLat Reference latitude
  /// @param aRefLon Reference longitude
  /// @param aSurface If true, aircraft is on ground
  /// @return Decoded position, or nullopt if decode fails
  [[nodiscard]] static std::optional<Position> decodeRelative(
      const CprFrame& aFrame,
      bool aOdd,
      double aRefLat,
      double aRefLon,
      bool aSurface = false);

private:
  // CPR constants
  static constexpr double kCprScale = 131072.0;  // 2^17

  // NL (Number of Longitude zones) lookup function
  [[nodiscard]] static int nlFunction(double aLat);

  // N function for longitude
  [[nodiscard]] static int nFunction(double aLat, bool aOdd) {
    int nl = nlFunction(aLat) - (aOdd ? 1 : 0);
    return (nl > 1) ? nl : 1;
  }

  // Dlon function (longitude zone width)
  [[nodiscard]] static double dlonFunction(double aLat, bool aOdd,
                                           bool aSurface) {
    double divisor = static_cast<double>(nFunction(aLat, aOdd));
    return (aSurface ? 90.0 : 360.0) / divisor;
  }

  // Modulo function that handles negative numbers correctly
  [[nodiscard]] static int modFunction(int a, int b) {
    int result = a % b;
    return (result < 0) ? result + b : result;
  }
};

// NL lookup table for latitude zones
// Based on the NL function from CPR specification
inline int
CprDecoder::nlFunction(double aLat) {
  aLat = std::abs(aLat);

  // Table of NL values for latitude thresholds
  // NL decreases as latitude increases
  if (aLat < 10.47047130) return 59;
  if (aLat < 14.82817437) return 58;
  if (aLat < 18.18626357) return 57;
  if (aLat < 21.02939493) return 56;
  if (aLat < 23.54504487) return 55;
  if (aLat < 25.82924707) return 54;
  if (aLat < 27.93898710) return 53;
  if (aLat < 29.91135686) return 52;
  if (aLat < 31.77209708) return 51;
  if (aLat < 33.53993436) return 50;
  if (aLat < 35.22899598) return 49;
  if (aLat < 36.85025108) return 48;
  if (aLat < 38.41241892) return 47;
  if (aLat < 39.92256684) return 46;
  if (aLat < 41.38651832) return 45;
  if (aLat < 42.80914012) return 44;
  if (aLat < 44.19454951) return 43;
  if (aLat < 45.54626723) return 42;
  if (aLat < 46.86733252) return 41;
  if (aLat < 48.16039128) return 40;
  if (aLat < 49.42776439) return 39;
  if (aLat < 50.67150166) return 38;
  if (aLat < 51.89342469) return 37;
  if (aLat < 53.09516153) return 36;
  if (aLat < 54.27817472) return 35;
  if (aLat < 55.44378444) return 34;
  if (aLat < 56.59318756) return 33;
  if (aLat < 57.72747354) return 32;
  if (aLat < 58.84763776) return 31;
  if (aLat < 59.95459277) return 30;
  if (aLat < 61.04917774) return 29;
  if (aLat < 62.13216659) return 28;
  if (aLat < 63.20427479) return 27;
  if (aLat < 64.26616523) return 26;
  if (aLat < 65.31845310) return 25;
  if (aLat < 66.36171008) return 24;
  if (aLat < 67.39646774) return 23;
  if (aLat < 68.42322022) return 22;
  if (aLat < 69.44242631) return 21;
  if (aLat < 70.45451075) return 20;
  if (aLat < 71.45986473) return 19;
  if (aLat < 72.45884545) return 18;
  if (aLat < 73.45177442) return 17;
  if (aLat < 74.43893416) return 16;
  if (aLat < 75.42056257) return 15;
  if (aLat < 76.39684391) return 14;
  if (aLat < 77.36789461) return 13;
  if (aLat < 78.33374083) return 12;
  if (aLat < 79.29428225) return 11;
  if (aLat < 80.24923213) return 10;
  if (aLat < 81.19801349) return 9;
  if (aLat < 82.13956981) return 8;
  if (aLat < 83.07199445) return 7;
  if (aLat < 83.99173563) return 6;
  if (aLat < 84.89166191) return 5;
  if (aLat < 85.75541621) return 4;
  if (aLat < 86.53536998) return 3;
  if (aLat < 87.00000000) return 2;
  return 1;
}

inline std::optional<Position>
CprDecoder::decodeGlobal(const CprFrame& aEvenFrame,
                         const CprFrame& aOddFrame,
                         bool aUseOdd,
                         bool aSurface,
                         double aRefLat,
                         double aRefLon) {
  double airDlat0 = (aSurface ? 90.0 : 360.0) / 60.0;
  double airDlat1 = (aSurface ? 90.0 : 360.0) / 59.0;

  double lat0 = static_cast<double>(aEvenFrame.latitude);
  double lat1 = static_cast<double>(aOddFrame.latitude);
  double lon0 = static_cast<double>(aEvenFrame.longitude);
  double lon1 = static_cast<double>(aOddFrame.longitude);

  // Compute the Latitude Index "j"
  int j = static_cast<int>(
      std::floor(((59.0 * lat0 - 60.0 * lat1) / kCprScale) + 0.5));

  double rlat0 = airDlat0 * (modFunction(j, 60) + lat0 / kCprScale);
  double rlat1 = airDlat1 * (modFunction(j, 59) + lat1 / kCprScale);

  if (aSurface) {
    // Adjust for surface position quadrant
    rlat0 += std::floor(aRefLat / 90.0) * 90.0;
    rlat1 += std::floor(aRefLat / 90.0) * 90.0;
  } else {
    if (rlat0 >= 270.0) rlat0 -= 360.0;
    if (rlat1 >= 270.0) rlat1 -= 360.0;
  }

  // Check latitude is in valid range
  if (rlat0 < -90.0 || rlat0 > 90.0 || rlat1 < -90.0 || rlat1 > 90.0) {
    return std::nullopt;
  }

  // Check both are in the same latitude zone
  if (nlFunction(rlat0) != nlFunction(rlat1)) {
    return std::nullopt;
  }

  Position result{};

  if (aUseOdd) {
    // Use odd packet
    int ni = nFunction(rlat1, true);
    int m = static_cast<int>(std::floor(
        ((lon0 * (nlFunction(rlat1) - 1) - lon1 * nlFunction(rlat1)) /
         kCprScale) +
        0.5));
    result.longitude =
        dlonFunction(rlat1, true, aSurface) * (modFunction(m, ni) + lon1 / kCprScale);
    result.latitude = rlat1;
  } else {
    // Use even packet
    int ni = nFunction(rlat0, false);
    int m = static_cast<int>(std::floor(
        ((lon0 * (nlFunction(rlat0) - 1) - lon1 * nlFunction(rlat0)) /
         kCprScale) +
        0.5));
    result.longitude =
        dlonFunction(rlat0, false, aSurface) * (modFunction(m, ni) + lon0 / kCprScale);
    result.latitude = rlat0;
  }

  if (aSurface) {
    result.longitude += std::floor(aRefLon / 90.0) * 90.0;
  } else if (result.longitude > 180.0) {
    result.longitude -= 360.0;
  }

  return result;
}

inline std::optional<Position>
CprDecoder::decodeRelative(const CprFrame& aFrame,
                           bool aOdd,
                           double aRefLat,
                           double aRefLon,
                           bool aSurface) {
  double airDlat = (aSurface ? 90.0 : 360.0) / (aOdd ? 59.0 : 60.0);
  double lat = static_cast<double>(aFrame.latitude);
  double lon = static_cast<double>(aFrame.longitude);

  // Compute the Latitude Index "j"
  int j = static_cast<int>(
      std::floor(aRefLat / airDlat) +
      std::trunc(0.5 + modFunction(static_cast<int>(aRefLat),
                                   static_cast<int>(airDlat)) /
                           airDlat -
                 lat / kCprScale));

  double rlat = airDlat * (j + lat / kCprScale);
  if (rlat >= 270.0) rlat -= 360.0;

  // Check latitude is in valid range
  if (rlat < -90.0 || rlat > 90.0) {
    return std::nullopt;
  }

  // Check answer is reasonable (no more than 1/2 cell away)
  if (std::abs(rlat - aRefLat) > (airDlat / 2.0)) {
    return std::nullopt;
  }

  // Compute the Longitude Index "m"
  double airDlon = dlonFunction(rlat, aOdd, aSurface);
  int m = static_cast<int>(
      std::floor(aRefLon / airDlon) +
      std::trunc(0.5 + modFunction(static_cast<int>(aRefLon),
                                   static_cast<int>(airDlon)) /
                           airDlon -
                 lon / kCprScale));

  double rlon = airDlon * (m + lon / kCprScale);
  if (rlon > 180.0) rlon -= 360.0;

  // Check answer is reasonable (no more than 1/2 cell away)
  if (std::abs(rlon - aRefLon) > (airDlon / 2.0)) {
    return std::nullopt;
  }

  return Position{rlat, rlon};
}

}  // namespace viz1090::decoder

#endif  // VIZ1090_DECODER_CPR_DECODER_H
