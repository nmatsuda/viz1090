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

#ifndef VIZ1090_DECODER_CRC_H
#define VIZ1090_DECODER_CRC_H

#include <array>
#include <cstdint>

#include "../Types.h"

namespace viz1090::decoder {

/// Mode S CRC-24 computation
///
/// Parity table for MODE S Messages. The table contains 112 elements,
/// every element corresponds to a bit set in the message, starting from
/// the first bit of actual data after the preamble.
///
/// The algorithm XORs all elements in this table for which the corresponding
/// bit in the message is set to 1.
class Crc {
public:
  /// Compute CRC-24 checksum for a Mode S message
  /// @param aMsg Message bytes
  /// @param aBits Number of bits (56 for short, 112 for long messages)
  /// @return 24-bit CRC syndrome (0 if valid)
  [[nodiscard]] static uint32_t compute(const uint8_t* aMsg, int aBits);

  /// Get message length in bits based on downlink format
  /// @param aType Downlink format (DF) value
  /// @return Number of bits (56 or 112)
  [[nodiscard]] static int messageLengthByType(int aType) {
    return (aType & 0x10) ? kLongMsgBits : kShortMsgBits;
  }

private:
  // CRC lookup table for 112-bit messages
  static constexpr std::array<uint32_t, 112> kChecksumTable = {{
      0x3935ea, 0x1c9af5, 0xf1b77e, 0x78dbbf, 0xc397db, 0x9e31e9, 0xb0e2f0,
      0x587178, 0x2c38bc, 0x161c5e, 0x0b0e2f, 0xfa7d13, 0x82c48d, 0xbe9842,
      0x5f4c21, 0xd05c14, 0x682e0a, 0x341705, 0xe5f186, 0x72f8c3, 0xc68665,
      0x9cb936, 0x4e5c9b, 0xd8d449, 0x939020, 0x49c810, 0x24e408, 0x127204,
      0x093902, 0x049c81, 0xfdb444, 0x7eda22, 0x3f6d11, 0xe04c8c, 0x702646,
      0x381323, 0xe3f395, 0x8e03ce, 0x4701e7, 0xdc7af7, 0x91c77f, 0xb719bb,
      0xa476d9, 0xadc168, 0x56e0b4, 0x2b705a, 0x15b82d, 0xf52612, 0x7a9309,
      0xc2b380, 0x6159c0, 0x30ace0, 0x185670, 0x0c2b38, 0x06159c, 0x030ace,
      0x018567, 0xff38b7, 0x80665f, 0xbfc92b, 0xa01e91, 0xaff54c, 0x57faa6,
      0x2bfd53, 0xea04ad, 0x8af852, 0x457c29, 0xdd4410, 0x6ea208, 0x375104,
      0x1ba882, 0x0dd441, 0xf91024, 0x7c8812, 0x3e4409, 0xe0d800, 0x706c00,
      0x383600, 0x1c1b00, 0x0e0d80, 0x0706c0, 0x038360, 0x01c1b0, 0x00e0d8,
      0x00706c, 0x003836, 0x001c1b, 0xfff409,
      // Last 24 elements are 0 (checksum portion)
      0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
      0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
      0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
      0x000000, 0x000000, 0x000000}};
};

// Inline implementation
inline uint32_t
Crc::compute(const uint8_t* aMsg, int aBits) {
  uint32_t crc = 0;
  int offset = (aBits == kLongMsgBits) ? 0 : (kLongMsgBits - kShortMsgBits);
  uint8_t theByte = *aMsg;
  const uint32_t* pCrcTable = &kChecksumTable[offset];

  // Don't include the checksum itself
  int bitsToProcess = aBits - 24;

  for (int j = 0; j < bitsToProcess; j++) {
    if ((j & 7) == 0) {
      theByte = *aMsg++;
    }

    // If bit is set, XOR with corresponding table entry
    if (theByte & 0x80) {
      crc ^= *pCrcTable;
    }
    pCrcTable++;
    theByte <<= 1;
  }

  // Get the message checksum
  uint32_t rem = (aMsg[0] << 16) | (aMsg[1] << 8) | aMsg[2];

  // Return 24-bit checksum syndrome
  return (crc ^ rem) & 0x00FFFFFF;
}

}  // namespace viz1090::decoder

#endif  // VIZ1090_DECODER_CRC_H
