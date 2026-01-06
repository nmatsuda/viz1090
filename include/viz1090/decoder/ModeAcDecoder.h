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

#ifndef VIZ1090_DECODER_MODE_AC_DECODER_H
#define VIZ1090_DECODER_MODE_AC_DECODER_H

#include <array>
#include <cstdint>
#include <optional>

#include "../Types.h"

namespace viz1090::decoder {

/// Mode A/C (SSR) decoder
///
/// Decodes Mode A (squawk) and Mode C (altitude) from secondary surveillance
/// radar responses.
class ModeAcDecoder {
public:
  /// Decoded Mode A/C result
  struct Result {
    SquawkCode modeA;              // Mode A squawk code
    std::optional<int> altitude;   // Mode C altitude in feet (if valid)
    SignalLevel signalLevel;       // Signal amplitude
    bool isModeC;                  // True if Mode C altitude is valid
  };

  /// Convert Mode A (squawk) to Mode C (altitude)
  /// @param aModeA Mode A code (Gillham encoded)
  /// @return Altitude in hundreds of feet, or nullopt if invalid
  [[nodiscard]] static std::optional<int> modeAToModeC(uint16_t aModeA);

  /// Decode Mode A code from raw value
  /// @param aRaw Raw 12-bit value
  /// @return Decoded squawk code in octal representation
  [[nodiscard]] static SquawkCode decodeModeA(uint16_t aRaw);

private:
  // Gray code to binary conversion table for Mode C
  static constexpr std::array<int, 512> kGrayToBinary = {{
      // This table converts 9-bit Gray code to binary
      // Each entry maps Gray code index to altitude in hundreds of feet
      0, 1, 3, 2, 6, 7, 5, 4, 12, 13, 15, 14, 10, 11, 9, 8,
      24, 25, 27, 26, 30, 31, 29, 28, 20, 21, 23, 22, 18, 19, 17, 16,
      48, 49, 51, 50, 54, 55, 53, 52, 60, 61, 63, 62, 58, 59, 57, 56,
      40, 41, 43, 42, 46, 47, 45, 44, 36, 37, 39, 38, 34, 35, 33, 32,
      96, 97, 99, 98, 102, 103, 101, 100, 108, 109, 111, 110, 106, 107, 105, 104,
      120, 121, 123, 122, 126, 127, 125, 124, 116, 117, 119, 118, 114, 115, 113, 112,
      80, 81, 83, 82, 86, 87, 85, 84, 92, 93, 95, 94, 90, 91, 89, 88,
      72, 73, 75, 74, 78, 79, 77, 76, 68, 69, 71, 70, 66, 67, 65, 64,
      192, 193, 195, 194, 198, 199, 197, 196, 204, 205, 207, 206, 202, 203, 201, 200,
      216, 217, 219, 218, 222, 223, 221, 220, 212, 213, 215, 214, 210, 211, 209, 208,
      240, 241, 243, 242, 246, 247, 245, 244, 252, 253, 255, 254, 250, 251, 249, 248,
      232, 233, 235, 234, 238, 239, 237, 236, 228, 229, 231, 230, 226, 227, 225, 224,
      160, 161, 163, 162, 166, 167, 165, 164, 172, 173, 175, 174, 170, 171, 169, 168,
      184, 185, 187, 186, 190, 191, 189, 188, 180, 181, 183, 182, 178, 179, 177, 176,
      144, 145, 147, 146, 150, 151, 149, 148, 156, 157, 159, 158, 154, 155, 153, 152,
      136, 137, 139, 138, 142, 143, 141, 140, 132, 133, 135, 134, 130, 131, 129, 128,
      384, 385, 387, 386, 390, 391, 389, 388, 396, 397, 399, 398, 394, 395, 393, 392,
      408, 409, 411, 410, 414, 415, 413, 412, 404, 405, 407, 406, 402, 403, 401, 400,
      432, 433, 435, 434, 438, 439, 437, 436, 444, 445, 447, 446, 442, 443, 441, 440,
      424, 425, 427, 426, 430, 431, 429, 428, 420, 421, 423, 422, 418, 419, 417, 416,
      480, 481, 483, 482, 486, 487, 485, 484, 492, 493, 495, 494, 490, 491, 489, 488,
      504, 505, 507, 506, 510, 511, 509, 508, 500, 501, 503, 502, 498, 499, 497, 496,
      464, 465, 467, 466, 470, 471, 469, 468, 476, 477, 479, 478, 474, 475, 473, 472,
      456, 457, 459, 458, 462, 463, 461, 460, 452, 453, 455, 454, 450, 451, 449, 448,
      320, 321, 323, 322, 326, 327, 325, 324, 332, 333, 335, 334, 330, 331, 329, 328,
      344, 345, 347, 346, 350, 351, 349, 348, 340, 341, 343, 342, 338, 339, 337, 336,
      368, 369, 371, 370, 374, 375, 373, 372, 380, 381, 383, 382, 378, 379, 377, 376,
      360, 361, 363, 362, 366, 367, 365, 364, 356, 357, 359, 358, 354, 355, 353, 352,
      288, 289, 291, 290, 294, 295, 293, 292, 300, 301, 303, 302, 298, 299, 297, 296,
      312, 313, 315, 314, 318, 319, 317, 316, 308, 309, 311, 310, 306, 307, 305, 304,
      272, 273, 275, 274, 278, 279, 277, 276, 284, 285, 287, 286, 282, 283, 281, 280,
      264, 265, 267, 266, 270, 271, 269, 268, 260, 261, 263, 262, 258, 259, 257, 256
  }};
};

inline std::optional<int>
ModeAcDecoder::modeAToModeC(uint16_t aModeA) {
  // Extract the relevant bits for Gillham decoding
  // C1, C2, C4 bits (hundreds)
  // A1, A2, A4 bits (thousands)
  // B1, B2, B4 bits (five-hundreds)
  // D1, D2, D4 bits (remaining)

  uint16_t c1 = (aModeA >> 4) & 1;
  uint16_t c2 = (aModeA >> 5) & 1;
  uint16_t c4 = (aModeA >> 6) & 1;
  uint16_t a1 = (aModeA >> 12) & 1;
  uint16_t a2 = (aModeA >> 13) & 1;
  uint16_t a4 = (aModeA >> 14) & 1;
  uint16_t b1 = (aModeA >> 8) & 1;
  uint16_t b2 = (aModeA >> 9) & 1;
  uint16_t b4 = (aModeA >> 10) & 1;
  uint16_t d1 = (aModeA >> 0) & 1;
  uint16_t d2 = (aModeA >> 1) & 1;
  uint16_t d4 = (aModeA >> 2) & 1;

  // Check for invalid codes (D1 set indicates Mode A only)
  if (d1) {
    return std::nullopt;
  }

  // Note: grayCode calculation removed - using separate 500ft/100ft decoding below
  // The Gillham code uses separate Gray code paths for different altitude ranges

  // Return altitude in hundreds of feet
  int fiveHundreds = 0;
  int oneHundreds = 0;

  // Decode the Gray code for 500ft increments
  uint16_t gray500 =
      (d2 << 7) | (d4 << 6) | (a1 << 5) | (a2 << 4) | (a4 << 3) |
      (b1 << 2) | (b2 << 1) | b4;

  if (gray500 < 512) {
    fiveHundreds = kGrayToBinary[gray500];
  } else {
    return std::nullopt;
  }

  // Decode C bits for 100ft increment
  uint16_t grayC = (c1 << 2) | (c2 << 1) | c4;
  oneHundreds = kGrayToBinary[grayC & 7];

  if (oneHundreds > 4) {
    oneHundreds = 4 - (oneHundreds - 5);
    fiveHundreds--;
  }

  int altitude = (fiveHundreds * 5) + oneHundreds - 13;

  return altitude;
}

inline SquawkCode
ModeAcDecoder::decodeModeA(uint16_t aRaw) {
  // Convert from interleaved format to standard octal squawk
  SquawkCode squawk = 0;

  // A bits (thousands)
  squawk |= ((aRaw >> 12) & 1) * 1000;
  squawk |= ((aRaw >> 13) & 1) * 2000;
  squawk |= ((aRaw >> 14) & 1) * 4000;

  // B bits (hundreds)
  squawk |= ((aRaw >> 8) & 1) * 100;
  squawk |= ((aRaw >> 9) & 1) * 200;
  squawk |= ((aRaw >> 10) & 1) * 400;

  // C bits (tens)
  squawk |= ((aRaw >> 4) & 1) * 10;
  squawk |= ((aRaw >> 5) & 1) * 20;
  squawk |= ((aRaw >> 6) & 1) * 40;

  // D bits (ones)
  squawk |= ((aRaw >> 0) & 1) * 1;
  squawk |= ((aRaw >> 1) & 1) * 2;
  squawk |= ((aRaw >> 2) & 1) * 4;

  return squawk;
}

}  // namespace viz1090::decoder

#endif  // VIZ1090_DECODER_MODE_AC_DECODER_H
