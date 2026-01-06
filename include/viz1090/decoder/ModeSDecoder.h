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

#ifndef VIZ1090_DECODER_MODE_S_DECODER_H
#define VIZ1090_DECODER_MODE_S_DECODER_H

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>

#include "../ModesMessage.h"
#include "../Types.h"
#include "CprDecoder.h"
#include "Crc.h"
#include "ErrorCorrection.h"

namespace viz1090::decoder {

/// Mode S message decoder
///
/// Decodes raw Mode S messages into structured ModesMessage objects.
/// Supports error correction and ICAO address caching.
class ModeSDecoder {
public:
  /// Decoder configuration
  struct Config {
    int maxBitErrors;
    bool checkCrc;
    Seconds icaoCacheTtl;

    Config()
        : maxBitErrors(kMaxBitErrors),
          checkCrc(true),
          icaoCacheTtl(kIcaoCacheTtl) {}
  };

  explicit ModeSDecoder(const Config& aConfig = Config{});

  /// Decode a raw Mode S message
  /// @param aMsg Raw message bytes (7 or 14 bytes)
  /// @param aMsgLen Length of message in bytes
  /// @param aTimestamp Message timestamp
  /// @param aSignalLevel Signal amplitude
  /// @return Decoded message, or nullopt if decode fails
  [[nodiscard]] std::optional<ModesMessage> decode(const uint8_t* aMsg,
                                                   size_t aMsgLen,
                                                   uint64_t aTimestamp = 0,
                                                   SignalLevel aSignalLevel = 0);

  /// Check if an ICAO address was recently seen
  /// @param aAddr ICAO address to check
  /// @return true if address is in cache
  [[nodiscard]] bool isIcaoRecentlySeen(IcaoAddress aAddr) const;

  /// Add an ICAO address to the cache
  /// @param aAddr ICAO address to add
  void addRecentlySeenIcao(IcaoAddress aAddr);

  /// Get decoder statistics
  struct Stats {
    uint64_t messagesDecoded = 0;
    uint64_t crcErrors = 0;
    uint64_t bitErrorsFixed = 0;
    uint64_t cacheHits = 0;
  };
  [[nodiscard]] const Stats& stats() const { return mStats; }

private:
  // Decode different message types
  void decodeDF0(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeDF4(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeDF5(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeDF11(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeDF16(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeDF17(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeDF18(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeDF20(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeDF21(ModesMessage& aMm, const uint8_t* aMsg);

  // Decode Extended Squitter (DF17/18) subtypes
  void decodeESAircraftId(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeESSurfacePosition(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeESAirbornePosition(ModesMessage& aMm, const uint8_t* aMsg);
  void decodeESAirborneVelocity(ModesMessage& aMm, const uint8_t* aMsg);

  // Altitude decoding
  [[nodiscard]] static int decodeAc13Field(int aField, AltitudeUnit& aUnit);
  [[nodiscard]] static int decodeAc12Field(int aField, AltitudeUnit& aUnit);
  [[nodiscard]] static int decodeId13Field(int aField);
  [[nodiscard]] static int decodeMovementField(int aMovement);

  // ICAO cache hash function
  [[nodiscard]] static uint32_t hashIcaoAddress(IcaoAddress aAddr);

  Config mConfig;
  ErrorCorrection mErrorCorrection;

  // ICAO address cache: address -> timestamp
  mutable std::unordered_map<IcaoAddress, TimePoint> mIcaoCache;

  Stats mStats;

  // AIS charset for flight ID decoding
  static constexpr char kAisCharset[] =
      "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? "
      "???????????????0123456789??????";
};

}  // namespace viz1090::decoder

#endif  // VIZ1090_DECODER_MODE_S_DECODER_H
