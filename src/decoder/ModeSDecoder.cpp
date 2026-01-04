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

#include "viz1090/decoder/ModeSDecoder.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace viz1090::decoder {

ModeSDecoder::ModeSDecoder(const Config& aConfig)
    : mConfig(aConfig), mErrorCorrection(aConfig.maxBitErrors) {}

std::optional<ModesMessage>
ModeSDecoder::decode(const uint8_t* aMsg, size_t aMsgLen, uint64_t aTimestamp,
                     SignalLevel aSignalLevel) {
  if (aMsgLen < kShortMsgBytes) {
    return std::nullopt;
  }

  ModesMessage mm;

  // Copy message data
  mm.setMsg(aMsg, std::min(aMsgLen, static_cast<size_t>(kLongMsgBytes)));

  // Get the message type (Downlink Format)
  int msgType = aMsg[0] >> 3;
  mm.setMsgType(msgType);
  mm.setMsgBits(Crc::messageLengthByType(msgType));

  // Compute CRC
  uint32_t crc = Crc::compute(aMsg, mm.msgBits());
  mm.setCrc(crc);

  // Try to fix bit errors for DF17/18 messages
  std::array<uint8_t, kLongMsgBytes> msgCopy;
  std::copy(aMsg, aMsg + std::min(aMsgLen, static_cast<size_t>(kLongMsgBytes)),
            msgCopy.begin());

  if (crc != 0 && mConfig.maxBitErrors > 0 &&
      (msgType == 17 || msgType == 18)) {
    // Use larger buffer to avoid potential overflow in fixBitErrors
    std::array<int8_t, kMaxBitErrors + 1> fixedBits{};
    int fixed =
        mErrorCorrection.fixBitErrors(msgCopy.data(), mm.msgBits(),
                                      mConfig.maxBitErrors, fixedBits.data());
    if (fixed > 0) {
      mm.setCorrectedBits(fixed);
      crc = Crc::compute(msgCopy.data(), mm.msgBits());
      mm.setCrc(crc);
      mm.setMsg(msgCopy.data(), kLongMsgBytes);
      mStats.bitErrorsFixed += fixed;
    }
  }

  mm.setCrcOk(crc == 0);

  // Check CRC if required
  if (mConfig.checkCrc && crc != 0) {
    // For some message types, CRC is XORed with ICAO address
    // Check if we've seen this address recently
    if (msgType == 11 || msgType == 17 || msgType == 18) {
      mStats.crcErrors++;
      return std::nullopt;
    }

    // For reply messages, extract address from CRC
    IcaoAddress addr = crc;
    if (!isIcaoRecentlySeen(addr)) {
      mStats.crcErrors++;
      return std::nullopt;
    }
    mStats.cacheHits++;
    mm.setAddr(addr);
  }

  // Extract ICAO address from message
  if (crc == 0) {
    IcaoAddress addr = (msgCopy[1] << 16) | (msgCopy[2] << 8) | msgCopy[3];
    mm.setAddr(addr);

    // Cache addresses from messages with good CRC
    if (msgType == 11 || msgType == 17 || msgType == 18) {
      addRecentlySeenIcao(addr);
    }
  }

  mm.setTimestamp(aTimestamp);
  mm.setSignalLevel(aSignalLevel);

  // Decode based on message type
  switch (msgType) {
    case 0:
      decodeDF0(mm, msgCopy.data());
      break;
    case 4:
      decodeDF4(mm, msgCopy.data());
      break;
    case 5:
      decodeDF5(mm, msgCopy.data());
      break;
    case 11:
      decodeDF11(mm, msgCopy.data());
      break;
    case 16:
      decodeDF16(mm, msgCopy.data());
      break;
    case 17:
      decodeDF17(mm, msgCopy.data());
      break;
    case 18:
      decodeDF18(mm, msgCopy.data());
      break;
    case 20:
      decodeDF20(mm, msgCopy.data());
      break;
    case 21:
      decodeDF21(mm, msgCopy.data());
      break;
    default:
      // Unknown message type
      break;
  }

  mStats.messagesDecoded++;
  return mm;
}

bool
ModeSDecoder::isIcaoRecentlySeen(IcaoAddress aAddr) const {
  auto it = mIcaoCache.find(aAddr);
  if (it == mIcaoCache.end()) {
    return false;
  }

  auto elapsed = Clock::now() - it->second;
  return elapsed <= mConfig.icaoCacheTtl;
}

void
ModeSDecoder::addRecentlySeenIcao(IcaoAddress aAddr) {
  mIcaoCache[aAddr] = Clock::now();
}

uint32_t
ModeSDecoder::hashIcaoAddress(IcaoAddress aAddr) {
  // Hash function for ICAO address cache
  uint32_t a = aAddr;
  a = ((a >> 16) ^ a) * 0x45d9f3b;
  a = ((a >> 16) ^ a) * 0x45d9f3b;
  a = ((a >> 16) ^ a);
  return a & (kIcaoCacheLen - 1);
}

// DF0: Short air-air surveillance
void
ModeSDecoder::decodeDF0(ModesMessage& aMm, const uint8_t* aMsg) {
  int altitude = ((aMsg[2] << 8) | aMsg[3]) & 0x1FFF;
  if (altitude > 0) {
    AltitudeUnit unit;
    aMm.setAltitude(decodeAc13Field(altitude, unit));
    aMm.setUnit(unit);
    aMm.addFlags(AircraftFlags::AltitudeValid);
  }
}

// DF4: Surveillance, altitude reply
void
ModeSDecoder::decodeDF4(ModesMessage& aMm, const uint8_t* aMsg) {
  int fs = (aMsg[0] & 7);
  aMm.setFlightStatus(fs);
  aMm.addFlags(AircraftFlags::FlightStatusValid);

  if (fs == 1 || fs == 3) {
    aMm.addFlags(AircraftFlags::OnGround | AircraftFlags::OnGroundValid);
  } else if (fs == 0 || fs == 2 || fs == 4) {
    aMm.addFlags(AircraftFlags::OnGroundValid);
  }

  int altitude = ((aMsg[2] << 8) | aMsg[3]) & 0x1FFF;
  if (altitude > 0) {
    AltitudeUnit unit;
    aMm.setAltitude(decodeAc13Field(altitude, unit));
    aMm.setUnit(unit);
    aMm.addFlags(AircraftFlags::AltitudeValid);
  }
}

// DF5: Surveillance, identity reply
void
ModeSDecoder::decodeDF5(ModesMessage& aMm, const uint8_t* aMsg) {
  int fs = (aMsg[0] & 7);
  aMm.setFlightStatus(fs);
  aMm.addFlags(AircraftFlags::FlightStatusValid);

  if (fs == 1 || fs == 3) {
    aMm.addFlags(AircraftFlags::OnGround | AircraftFlags::OnGroundValid);
  } else if (fs == 0 || fs == 2 || fs == 4) {
    aMm.addFlags(AircraftFlags::OnGroundValid);
  }

  int id13 = ((aMsg[2] << 8) | aMsg[3]) & 0x1FFF;
  aMm.setModeA(static_cast<SquawkCode>(decodeId13Field(id13)));
  aMm.addFlags(AircraftFlags::SquawkValid);
}

// DF11: All-call reply
void
ModeSDecoder::decodeDF11(ModesMessage& aMm, const uint8_t* aMsg) {
  int ca = aMsg[0] & 7;
  aMm.setCa(ca);

  if (ca == 4) {
    aMm.addFlags(AircraftFlags::OnGround | AircraftFlags::OnGroundValid);
  } else if (ca == 5) {
    aMm.addFlags(AircraftFlags::OnGroundValid);
  }

  // IID (Interrogator ID)
  IcaoAddress addr = (aMsg[1] << 16) | (aMsg[2] << 8) | aMsg[3];
  aMm.setAddr(addr);
}

// DF16: Long air-air surveillance
void
ModeSDecoder::decodeDF16(ModesMessage& aMm, const uint8_t* aMsg) {
  int altitude = ((aMsg[2] << 8) | aMsg[3]) & 0x1FFF;
  if (altitude > 0) {
    AltitudeUnit unit;
    aMm.setAltitude(decodeAc13Field(altitude, unit));
    aMm.setUnit(unit);
    aMm.addFlags(AircraftFlags::AltitudeValid);
  }
}

// DF17: Extended Squitter
void
ModeSDecoder::decodeDF17(ModesMessage& aMm, const uint8_t* aMsg) {
  int ca = aMsg[0] & 7;
  aMm.setCa(ca);

  IcaoAddress addr = (aMsg[1] << 16) | (aMsg[2] << 8) | aMsg[3];
  aMm.setAddr(addr);

  int meType = aMsg[4] >> 3;
  int meSub = aMsg[4] & 7;
  aMm.setMeType(meType);
  aMm.setMeSub(meSub);

  // Decode based on ME type
  if (meType >= 1 && meType <= 4) {
    decodeESAircraftId(aMm, aMsg);
  } else if (meType >= 5 && meType <= 8) {
    decodeESSurfacePosition(aMm, aMsg);
  } else if (meType >= 9 && meType <= 18) {
    decodeESAirbornePosition(aMm, aMsg);
  } else if (meType == 19) {
    decodeESAirborneVelocity(aMm, aMsg);
  } else if (meType >= 20 && meType <= 22) {
    decodeESAirbornePosition(aMm, aMsg);
  }
}

// DF18: Extended Squitter/Non-Transponder
void
ModeSDecoder::decodeDF18(ModesMessage& aMm, const uint8_t* aMsg) {
  // CF field instead of CA
  int cf = aMsg[0] & 7;
  aMm.setCa(cf);

  IcaoAddress addr = (aMsg[1] << 16) | (aMsg[2] << 8) | aMsg[3];
  aMm.setAddr(addr);

  // Same ME decoding as DF17
  if (cf == 0 || cf == 1 || cf == 6) {
    int meType = aMsg[4] >> 3;
    int meSub = aMsg[4] & 7;
    aMm.setMeType(meType);
    aMm.setMeSub(meSub);

    if (meType >= 1 && meType <= 4) {
      decodeESAircraftId(aMm, aMsg);
    } else if (meType >= 5 && meType <= 8) {
      decodeESSurfacePosition(aMm, aMsg);
    } else if (meType >= 9 && meType <= 18) {
      decodeESAirbornePosition(aMm, aMsg);
    } else if (meType == 19) {
      decodeESAirborneVelocity(aMm, aMsg);
    } else if (meType >= 20 && meType <= 22) {
      decodeESAirbornePosition(aMm, aMsg);
    }
  }
}

// DF20: Comm-B, altitude reply
void
ModeSDecoder::decodeDF20(ModesMessage& aMm, const uint8_t* aMsg) {
  int fs = (aMsg[0] & 7);
  aMm.setFlightStatus(fs);
  aMm.addFlags(AircraftFlags::FlightStatusValid);

  if (fs == 1 || fs == 3) {
    aMm.addFlags(AircraftFlags::OnGround | AircraftFlags::OnGroundValid);
  } else if (fs == 0 || fs == 2 || fs == 4) {
    aMm.addFlags(AircraftFlags::OnGroundValid);
  }

  int altitude = ((aMsg[2] << 8) | aMsg[3]) & 0x1FFF;
  if (altitude > 0) {
    AltitudeUnit unit;
    aMm.setAltitude(decodeAc13Field(altitude, unit));
    aMm.setUnit(unit);
    aMm.addFlags(AircraftFlags::AltitudeValid);
  }
}

// DF21: Comm-B, identity reply
void
ModeSDecoder::decodeDF21(ModesMessage& aMm, const uint8_t* aMsg) {
  int fs = (aMsg[0] & 7);
  aMm.setFlightStatus(fs);
  aMm.addFlags(AircraftFlags::FlightStatusValid);

  if (fs == 1 || fs == 3) {
    aMm.addFlags(AircraftFlags::OnGround | AircraftFlags::OnGroundValid);
  } else if (fs == 0 || fs == 2 || fs == 4) {
    aMm.addFlags(AircraftFlags::OnGroundValid);
  }

  int id13 = ((aMsg[2] << 8) | aMsg[3]) & 0x1FFF;
  aMm.setModeA(static_cast<SquawkCode>(decodeId13Field(id13)));
  aMm.addFlags(AircraftFlags::SquawkValid);
}

// Decode aircraft identification (ME type 1-4)
void
ModeSDecoder::decodeESAircraftId(ModesMessage& aMm, const uint8_t* aMsg) {
  char flight[9];

  flight[0] = kAisCharset[aMsg[5] >> 2];
  flight[1] = kAisCharset[((aMsg[5] & 3) << 4) | (aMsg[6] >> 4)];
  flight[2] = kAisCharset[((aMsg[6] & 15) << 2) | (aMsg[7] >> 6)];
  flight[3] = kAisCharset[aMsg[7] & 63];
  flight[4] = kAisCharset[aMsg[8] >> 2];
  flight[5] = kAisCharset[((aMsg[8] & 3) << 4) | (aMsg[9] >> 4)];
  flight[6] = kAisCharset[((aMsg[9] & 15) << 2) | (aMsg[10] >> 6)];
  flight[7] = kAisCharset[aMsg[10] & 63];
  flight[8] = '\0';

  aMm.setFlight(flight);
  aMm.addFlags(AircraftFlags::CallsignValid);
}

// Decode surface position (ME type 5-8)
void
ModeSDecoder::decodeESSurfacePosition(ModesMessage& aMm, const uint8_t* aMsg) {
  aMm.addFlags(AircraftFlags::OnGround | AircraftFlags::OnGroundValid);

  int movement = ((aMsg[4] & 7) << 4) | (aMsg[5] >> 4);
  if (movement > 0 && movement < 125) {
    aMm.setVelocity(decodeMovementField(movement));
    aMm.addFlags(AircraftFlags::SpeedValid);
  }

  int trackStatus = (aMsg[5] >> 3) & 1;
  if (trackStatus) {
    int track = ((aMsg[5] & 7) << 4) | (aMsg[6] >> 4);
    aMm.setHeading(static_cast<int>(track * 360.0 / 128.0));
    aMm.addFlags(AircraftFlags::HeadingValid);
  }

  int fflag = (aMsg[6] >> 2) & 1;  // Odd/even frame
  int rawLat = ((aMsg[6] & 3) << 15) | (aMsg[7] << 7) | (aMsg[8] >> 1);
  int rawLon = ((aMsg[8] & 1) << 16) | (aMsg[9] << 8) | aMsg[10];

  aMm.setRawLatitude(rawLat);
  aMm.setRawLongitude(rawLon);

  if (fflag) {
    aMm.addFlags(AircraftFlags::OddLatLonValid);
  } else {
    aMm.addFlags(AircraftFlags::EvenLatLonValid);
  }
}

// Decode airborne position (ME type 9-18, 20-22)
void
ModeSDecoder::decodeESAirbornePosition(ModesMessage& aMm, const uint8_t* aMsg) {
  // meType already set by caller, not needed here
  int ac12 = ((aMsg[5] << 4) | (aMsg[6] >> 4)) & 0xFFF;

  if (ac12 > 0) {
    AltitudeUnit unit;
    int altitude = decodeAc12Field(ac12, unit);
    if (altitude != 0) {
      aMm.setAltitude(altitude);
      aMm.setUnit(unit);
      aMm.addFlags(AircraftFlags::AltitudeValid);
    }
  }

  int fflag = (aMsg[6] >> 2) & 1;  // Odd/even frame
  int rawLat = ((aMsg[6] & 3) << 15) | (aMsg[7] << 7) | (aMsg[8] >> 1);
  int rawLon = ((aMsg[8] & 1) << 16) | (aMsg[9] << 8) | aMsg[10];

  aMm.setRawLatitude(rawLat);
  aMm.setRawLongitude(rawLon);

  if (fflag) {
    aMm.addFlags(AircraftFlags::OddLatLonValid);
  } else {
    aMm.addFlags(AircraftFlags::EvenLatLonValid);
  }
}

// Decode airborne velocity (ME type 19)
void
ModeSDecoder::decodeESAirborneVelocity(ModesMessage& aMm, const uint8_t* aMsg) {
  int meSub = aMsg[4] & 7;

  if (meSub == 1 || meSub == 2) {
    // Ground speed
    int ewDir = (aMsg[5] >> 2) & 1;
    int ewVel = ((aMsg[5] & 3) << 8) | aMsg[6];
    int nsDir = (aMsg[7] >> 7) & 1;
    int nsVel = ((aMsg[7] & 127) << 3) | (aMsg[8] >> 5);

    if (ewVel > 0) {
      ewVel--;
      if (ewDir) ewVel = -ewVel;
      aMm.setEwVelocity(ewVel);
      aMm.addFlags(AircraftFlags::EwSpeedValid);
    }

    if (nsVel > 0) {
      nsVel--;
      if (nsDir) nsVel = -nsVel;
      aMm.setNsVelocity(nsVel);
      aMm.addFlags(AircraftFlags::NsSpeedValid);
    }

    if (ewVel != 0 || nsVel != 0) {
      // Compute speed and heading from components
      int speed = static_cast<int>(
          std::sqrt(static_cast<double>(ewVel * ewVel + nsVel * nsVel)));
      aMm.setVelocity(speed);
      aMm.addFlags(AircraftFlags::SpeedValid);

      if (speed > 0) {
        int heading = static_cast<int>(
            std::atan2(static_cast<double>(ewVel), static_cast<double>(nsVel)) *
            180.0 / M_PI);
        if (heading < 0) heading += 360;
        aMm.setHeading(heading);
        aMm.addFlags(AircraftFlags::HeadingValid);
      }

      aMm.addFlags(AircraftFlags::NsEwSpeedValid);
    }

  } else if (meSub == 3 || meSub == 4) {
    // Airspeed
    int headingStatus = (aMsg[5] >> 2) & 1;
    if (headingStatus) {
      int heading = ((aMsg[5] & 3) << 8) | aMsg[6];
      heading = static_cast<int>(heading * 360.0 / 1024.0);
      aMm.setHeading(heading);
      aMm.addFlags(AircraftFlags::HeadingValid);
    }

    int airspeed = ((aMsg[7] & 127) << 3) | (aMsg[8] >> 5);
    if (airspeed > 0) {
      airspeed--;
      if (meSub == 4) {
        airspeed *= 4;  // Supersonic
      }
      aMm.setVelocity(airspeed);
      aMm.addFlags(AircraftFlags::SpeedValid);
    }
  }

  // Vertical rate
  int vertRateSign = (aMsg[8] >> 3) & 1;
  int vertRate = ((aMsg[8] & 7) << 6) | (aMsg[9] >> 2);
  if (vertRate > 0) {
    vertRate = (vertRate - 1) * 64;
    if (vertRateSign) vertRate = -vertRate;
    aMm.setVertRate(vertRate);
    aMm.addFlags(AircraftFlags::VertRateValid);
  }
}

// Decode 13-bit altitude field
int
ModeSDecoder::decodeAc13Field(int aField, AltitudeUnit& aUnit) {
  int mBit = aField & 0x0040;  // Meters bit
  int qBit = aField & 0x0010;  // 25ft encoding bit

  if (!mBit) {
    aUnit = AltitudeUnit::Feet;
    if (qBit) {
      // N is the 11 bit integer from removal of Q and M bits
      int n =
          ((aField & 0x1F80) >> 2) | ((aField & 0x0020) >> 1) | (aField & 0x000F);
      return (n * 25) - 1000;
    } else {
      // Gillham coded altitude
      int n = decodeId13Field(aField);
      // Convert Mode A to Mode C (handled by ModeAC decoder)
      // For now, just use the raw value
      if (n < -12) n = 0;
      return 100 * n;
    }
  } else {
    aUnit = AltitudeUnit::Meters;
    // Meter altitude not commonly used
    return 0;
  }
}

// Decode 12-bit altitude field
int
ModeSDecoder::decodeAc12Field(int aField, AltitudeUnit& aUnit) {
  int qBit = aField & 0x10;

  aUnit = AltitudeUnit::Feet;
  if (qBit) {
    // N is the 11 bit integer from removal of Q bit
    int n = ((aField & 0x0FE0) >> 1) | (aField & 0x000F);
    return (n * 25) - 1000;
  } else {
    // Make N a 13 bit Gillham coded altitude
    int n = ((aField & 0x0FC0) << 1) | (aField & 0x003F);
    n = decodeId13Field(n);
    if (n < -12) n = 0;
    return 100 * n;
  }
}

// Decode 13-bit identity field (squawk)
int
ModeSDecoder::decodeId13Field(int aField) {
  int hex = 0;

  if (aField & 0x1000) hex |= 0x0010;  // C1
  if (aField & 0x0800) hex |= 0x1000;  // A1
  if (aField & 0x0400) hex |= 0x0020;  // C2
  if (aField & 0x0200) hex |= 0x2000;  // A2
  if (aField & 0x0100) hex |= 0x0040;  // C4
  if (aField & 0x0080) hex |= 0x4000;  // A4
  if (aField & 0x0020) hex |= 0x0100;  // B1
  if (aField & 0x0010) hex |= 0x0001;  // D1
  if (aField & 0x0008) hex |= 0x0200;  // B2
  if (aField & 0x0004) hex |= 0x0002;  // D2
  if (aField & 0x0002) hex |= 0x0400;  // B4
  if (aField & 0x0001) hex |= 0x0004;  // D4

  return hex;
}

// Decode ground movement field
int
ModeSDecoder::decodeMovementField(int aMovement) {
  if (aMovement > 123) return 199;
  if (aMovement > 108) return ((aMovement - 108) * 5) + 100;
  if (aMovement > 93) return ((aMovement - 93) * 2) + 70;
  if (aMovement > 38) return (aMovement - 38) + 15;
  if (aMovement > 12) return ((aMovement - 11) >> 1) + 2;
  if (aMovement > 8) return ((aMovement - 6) >> 2) + 1;
  return 0;
}

}  // namespace viz1090::decoder
