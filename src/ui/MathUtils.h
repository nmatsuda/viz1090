// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
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

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <SDL2/SDL.h>
#include <chrono>
#include <cmath>

namespace viz1090 {

using fmilliseconds = std::chrono::duration<float, std::milli>;
using fseconds = std::chrono::duration<float>;

inline std::chrono::high_resolution_clock::time_point now() {
  return std::chrono::high_resolution_clock::now();
}

inline float elapsed(std::chrono::high_resolution_clock::time_point ref) {
  return fmilliseconds{now() - ref}.count();
}

inline float elapsed_s(std::chrono::high_resolution_clock::time_point ref) {
  return fseconds{now() - ref}.count();
}

inline float clamp(float in, float min, float max) {
  if (in < min) return min;
  if (in > max) return max;
  return in;
}

inline float lerp(float a, float b, float factor) {
  float f = clamp(factor, 0.0f, 1.0f);
  return (1.0f - f) * a + f * b;
}

inline float lerpAngle(float a, float b, float factor) {
  float diff = std::fabs(b - a);
  if (diff > 180.0f) {
    if (b > a) {
      a += 360.0f;
    } else {
      b += 360.0f;
    }
  }

  float value = a + ((b - a) * factor);

  if (value >= 0.0f && value <= 360.0f)
    return value;

  return std::fmod(value, 360.0f);
}

inline SDL_Color lerpColor(SDL_Color aColor, SDL_Color bColor, float factor) {
  float f = clamp(factor, 0.0f, 1.0f);
  SDL_Color out;
  out.r = static_cast<Uint8>((1.0f - f) * aColor.r + f * bColor.r);
  out.g = static_cast<Uint8>((1.0f - f) * aColor.g + f * bColor.g);
  out.b = static_cast<Uint8>((1.0f - f) * aColor.b + f * bColor.b);
  out.a = 255;
  return out;
}

inline void crossProduct(float* result, const float* u, const float* w) {
  result[0] = u[1] * w[2] - u[2] * w[1];
  result[1] = u[2] * w[0] - u[0] * w[2];
  result[2] = u[0] * w[1] - u[1] * w[0];
}

template <typename T>
inline int sign(T val) {
  return (T(0) < val) - (val < T(0));
}

inline SDL_Color makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
  SDL_Color c;
  c.r = r;
  c.g = g;
  c.b = b;
  c.a = a;
  return c;
}

}  // namespace viz1090

#endif  // MATH_UTILS_H
