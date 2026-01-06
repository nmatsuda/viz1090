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

#ifndef VIZ1090_MODES_MESSAGE_H
#define VIZ1090_MODES_MESSAGE_H

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

#include "Types.h"

namespace viz1090 {

/// Decoded Mode S message
///
/// This class represents a decoded ADS-B/Mode S message with all extracted
/// fields. Uses std::optional for fields that may not be present in all
/// message types.
class ModesMessage {
public:
  ModesMessage() = default;

  // Raw message data
  [[nodiscard]] const std::array<uint8_t, kLongMsgBytes>& msg() const {
    return mMsg;
  }
  void setMsg(const uint8_t* aData, size_t aLen);

  // Message metadata
  [[nodiscard]] int msgBits() const { return mMsgBits; }
  void setMsgBits(int aBits) { mMsgBits = aBits; }

  [[nodiscard]] DownlinkFormat msgType() const { return mMsgType; }
  void setMsgType(DownlinkFormat aType) { mMsgType = aType; }
  void setMsgType(int aType) {
    mMsgType = static_cast<DownlinkFormat>(aType);
  }

  [[nodiscard]] bool crcOk() const { return mCrcOk; }
  void setCrcOk(bool aOk) { mCrcOk = aOk; }

  [[nodiscard]] uint32_t crc() const { return mCrc; }
  void setCrc(uint32_t aCrc) { mCrc = aCrc; }

  [[nodiscard]] int correctedBits() const { return mCorrectedBits; }
  void setCorrectedBits(int aBits) { mCorrectedBits = aBits; }

  [[nodiscard]] IcaoAddress addr() const { return mAddr; }
  void setAddr(IcaoAddress aAddr) { mAddr = aAddr; }

  [[nodiscard]] bool phaseCorrected() const { return mPhaseCorrected; }
  void setPhaseCorrected(bool aCorrected) { mPhaseCorrected = aCorrected; }

  [[nodiscard]] uint64_t timestamp() const { return mTimestamp; }
  void setTimestamp(uint64_t aTimestamp) { mTimestamp = aTimestamp; }

  [[nodiscard]] bool remote() const { return mRemote; }
  void setRemote(bool aRemote) { mRemote = aRemote; }

  [[nodiscard]] SignalLevel signalLevel() const { return mSignalLevel; }
  void setSignalLevel(SignalLevel aLevel) { mSignalLevel = aLevel; }

  // DF 11 fields
  [[nodiscard]] std::optional<int> ca() const { return mCa; }
  void setCa(int aCa) { mCa = aCa; }

  [[nodiscard]] std::optional<int> iid() const { return mIid; }
  void setIid(int aIid) { mIid = aIid; }

  // DF 17/18 Extended Squitter fields
  [[nodiscard]] std::optional<int> meType() const { return mMeType; }
  void setMeType(int aType) { mMeType = aType; }

  [[nodiscard]] std::optional<int> meSub() const { return mMeSub; }
  void setMeSub(int aSub) { mMeSub = aSub; }

  [[nodiscard]] std::optional<int> heading() const { return mHeading; }
  void setHeading(int aHeading) { mHeading = aHeading; }

  [[nodiscard]] std::optional<int> rawLatitude() const { return mRawLatitude; }
  void setRawLatitude(int aLat) { mRawLatitude = aLat; }

  [[nodiscard]] std::optional<int> rawLongitude() const {
    return mRawLongitude;
  }
  void setRawLongitude(int aLon) { mRawLongitude = aLon; }

  [[nodiscard]] std::optional<Position> position() const { return mPosition; }
  void setPosition(double aLat, double aLon) {
    mPosition = Position{aLat, aLon};
  }
  void setPosition(const Position& aPos) { mPosition = aPos; }

  [[nodiscard]] std::string_view flight() const {
    return std::string_view(mFlight.data());
  }
  void setFlight(const char* aFlight);

  [[nodiscard]] std::optional<int> ewVelocity() const { return mEwVelocity; }
  void setEwVelocity(int aVel) { mEwVelocity = aVel; }

  [[nodiscard]] std::optional<int> nsVelocity() const { return mNsVelocity; }
  void setNsVelocity(int aVel) { mNsVelocity = aVel; }

  [[nodiscard]] std::optional<int> vertRate() const { return mVertRate; }
  void setVertRate(int aRate) { mVertRate = aRate; }

  [[nodiscard]] std::optional<int> velocity() const { return mVelocity; }
  void setVelocity(int aVel) { mVelocity = aVel; }

  // DF 4/5/20/21 fields
  [[nodiscard]] std::optional<int> flightStatus() const {
    return mFlightStatus;
  }
  void setFlightStatus(int aStatus) { mFlightStatus = aStatus; }

  [[nodiscard]] std::optional<SquawkCode> modeA() const { return mModeA; }
  void setModeA(SquawkCode aSquawk) { mModeA = aSquawk; }

  // Common fields
  [[nodiscard]] std::optional<int> altitude() const { return mAltitude; }
  void setAltitude(int aAlt) { mAltitude = aAlt; }

  [[nodiscard]] AltitudeUnit unit() const { return mUnit; }
  void setUnit(AltitudeUnit aUnit) { mUnit = aUnit; }

  [[nodiscard]] AircraftFlags flags() const { return mFlags; }
  void setFlags(AircraftFlags aFlags) { mFlags = aFlags; }
  void addFlags(AircraftFlags aFlags) { mFlags |= aFlags; }

  [[nodiscard]] bool hasFlag(AircraftFlags aFlag) const {
    return viz1090::hasFlag(mFlags, aFlag);
  }

private:
  // Raw message
  std::array<uint8_t, kLongMsgBytes> mMsg{};
  int mMsgBits = 0;
  DownlinkFormat mMsgType = DownlinkFormat::ShortAirSurveillance;
  bool mCrcOk = false;
  uint32_t mCrc = 0;
  int mCorrectedBits = 0;
  IcaoAddress mAddr = 0;
  bool mPhaseCorrected = false;
  uint64_t mTimestamp = 0;
  bool mRemote = false;
  SignalLevel mSignalLevel = 0;

  // DF 11
  std::optional<int> mCa;
  std::optional<int> mIid;

  // DF 17/18
  std::optional<int> mMeType;
  std::optional<int> mMeSub;
  std::optional<int> mHeading;
  std::optional<int> mRawLatitude;
  std::optional<int> mRawLongitude;
  std::optional<Position> mPosition;
  std::array<char, 16> mFlight{};
  std::optional<int> mEwVelocity;
  std::optional<int> mNsVelocity;
  std::optional<int> mVertRate;
  std::optional<int> mVelocity;

  // DF 4/5/20/21
  std::optional<int> mFlightStatus;
  std::optional<SquawkCode> mModeA;

  // Common
  std::optional<int> mAltitude;
  AltitudeUnit mUnit = AltitudeUnit::Feet;
  AircraftFlags mFlags = AircraftFlags::None;
};

// Inline implementations
inline void
ModesMessage::setMsg(const uint8_t* aData, size_t aLen) {
  size_t copyLen = std::min(aLen, mMsg.size());
  std::copy(aData, aData + copyLen, mMsg.begin());
}

inline void
ModesMessage::setFlight(const char* aFlight) {
  if (aFlight) {
    size_t len = std::min(strlen(aFlight), mFlight.size() - 1);
    std::copy(aFlight, aFlight + len, mFlight.begin());
    mFlight[len] = '\0';
  }
}

}  // namespace viz1090

#endif  // VIZ1090_MODES_MESSAGE_H
