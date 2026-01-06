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

#ifndef VIZ1090_DECODER_ERROR_CORRECTION_H
#define VIZ1090_DECODER_ERROR_CORRECTION_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "../Types.h"
#include "Crc.h"

namespace viz1090::decoder {

/// Bit error correction for Mode S messages
///
/// Uses syndrome lookup tables for efficient correction of 1-bit and 2-bit
/// errors. Based on the linear property of CRC: crc(m^e) = crc(m)^crc(e)
class ErrorCorrection {
public:
  /// Information about a correctable error
  struct ErrorInfo {
    uint32_t syndrome;                         // CRC syndrome for this error
    int8_t bits;                               // Number of bits to fix (1 or 2)
    std::array<int16_t, kMaxBitErrors> positions;  // Bit positions to flip

    bool operator<(const ErrorInfo& aOther) const {
      return syndrome < aOther.syndrome;
    }
  };

  /// Initialize error tables
  /// @param aMaxErrors Maximum number of bit errors to correct (1 or 2)
  explicit ErrorCorrection(int aMaxErrors = kMaxBitErrors);

  /// Attempt to fix bit errors in a message
  /// @param aMsg Message bytes (will be modified if errors are fixed)
  /// @param aBits Message length in bits
  /// @param aMaxFix Maximum number of bits to fix
  /// @param aFixedBits Output array of fixed bit positions (can be nullptr)
  /// @return Number of bits fixed (0 if no fixable error found)
  int fixBitErrors(uint8_t* aMsg, int aBits, int aMaxFix,
                   int8_t* aFixedBits = nullptr);

  /// Check if error tables are initialized
  [[nodiscard]] bool isInitialized() const { return mInitialized; }

private:
  void initErrorTable(int aMaxErrors);

  std::vector<ErrorInfo> mErrorTable;
  bool mInitialized = false;
};

// Implementation

inline ErrorCorrection::ErrorCorrection(int aMaxErrors) {
  initErrorTable(aMaxErrors);
}

inline void
ErrorCorrection::initErrorTable(int aMaxErrors) {
  // Calculate table size
  // Single bit errors: 112 - 5 = 107 entries (skip first 5 bits = DF type)
  // Double bit errors: 107 * 106 / 2 = 5671 entries
  size_t tableSize = kLongMsgBits - 5;
  if (aMaxErrors > 1) {
    tableSize += (kLongMsgBits - 5) * (kLongMsgBits - 6) / 2;
  }
  mErrorTable.reserve(tableSize);

  std::array<uint8_t, kLongMsgBytes> msg{};

  // Add all possible single and double bit errors
  // Don't include errors in first 5 bits (DF type)
  for (int i = 5; i < kLongMsgBits; i++) {
    int bytepos0 = i >> 3;
    uint8_t mask0 = 1 << (7 - (i & 7));

    msg[bytepos0] ^= mask0;  // Create error
    uint32_t crc = Crc::compute(msg.data(), kLongMsgBits);

    ErrorInfo info{};
    info.syndrome = crc;
    info.bits = 1;
    info.positions[0] = static_cast<int16_t>(i);
    info.positions[1] = -1;
    mErrorTable.push_back(info);

    if (aMaxErrors > 1) {
      for (int j = i + 1; j < kLongMsgBits; j++) {
        int bytepos1 = j >> 3;
        uint8_t mask1 = 1 << (7 - (j & 7));

        msg[bytepos1] ^= mask1;  // Create second error
        crc = Crc::compute(msg.data(), kLongMsgBits);

        ErrorInfo info2{};
        info2.syndrome = crc;
        info2.bits = 2;
        info2.positions[0] = static_cast<int16_t>(i);
        info2.positions[1] = static_cast<int16_t>(j);
        mErrorTable.push_back(info2);

        msg[bytepos1] ^= mask1;  // Revert second error
      }
    }
    msg[bytepos0] ^= mask0;  // Revert first error
  }

  // Sort for binary search
  std::sort(mErrorTable.begin(), mErrorTable.end());
  mInitialized = true;
}

inline int
ErrorCorrection::fixBitErrors(uint8_t* aMsg, int aBits, int aMaxFix,
                              int8_t* aFixedBits) {
  if (!mInitialized || mErrorTable.empty()) {
    return 0;
  }

  // Compute syndrome
  ErrorInfo search{};
  search.syndrome = Crc::compute(aMsg, aBits);

  // Binary search for syndrome
  auto it = std::lower_bound(
      mErrorTable.begin(), mErrorTable.end(), search,
      [](const ErrorInfo& a, const ErrorInfo& b) {
        return a.syndrome < b.syndrome;
      });

  if (it == mErrorTable.end() || it->syndrome != search.syndrome) {
    return 0;  // No syndrome found
  }

  // Check if the syndrome fixes more bits than we allow
  if (aMaxFix < it->bits) {
    return 0;
  }

  // Check that all bit positions lie inside the message length
  int offset = kLongMsgBits - aBits;
  for (int i = 0; i < it->bits; i++) {
    int bitpos = it->positions[i] - offset;
    if (bitpos < 0 || bitpos >= aBits) {
      return 0;
    }
  }

  // Fix the bits
  int result = 0;
  for (int i = 0; i < it->bits; i++) {
    int bitpos = it->positions[i] - offset;
    aMsg[bitpos >> 3] ^= (1 << (7 - (bitpos & 7)));
    if (aFixedBits) {
      aFixedBits[result] = static_cast<int8_t>(bitpos);
    }
    result++;
  }

  return result;
}

}  // namespace viz1090::decoder

#endif  // VIZ1090_DECODER_ERROR_CORRECTION_H
