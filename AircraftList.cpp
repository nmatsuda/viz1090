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
//

#include "AircraftList.h"

#include <cstring>

#include "viz1090/decoder/CprDecoder.h"

// Maximum time between even and odd CPR frames for global decode (10 seconds)
static constexpr uint64_t kCprMaxTimeDiff = 10000000;  // microseconds

static std::chrono::high_resolution_clock::time_point
now() {
  return std::chrono::high_resolution_clock::now();
}

Aircraft*
AircraftList::find(uint32_t aAddr) {
  Aircraft* p = head;

  while (p) {
    if (p->addr == aAddr)
      return p;
    p = p->next;
  }
  return nullptr;
}

Aircraft*
AircraftList::findOrCreate(uint32_t aAddr) {
  Aircraft* p = find(aAddr);
  if (!p) {
    p = new Aircraft(aAddr);
    p->next = head;
    head = p;
  }
  return p;
}

void
AircraftList::updateFromMessage(const viz1090::ModesMessage& aMsg) {
  // Only process messages with valid ICAO addresses
  if (aMsg.addr() == 0) {
    return;
  }

  Aircraft* p = findOrCreate(aMsg.addr());

  auto currentTime = now();
  p->prev_seen = p->seen;
  p->seen = std::time(nullptr);
  p->msSeen = currentTime;
  p->live = 1;

  // Calculate message rate
  if ((p->seen - p->prev_seen) > 0) {
    p->messageRate = 1.0f / static_cast<float>(p->seen - p->prev_seen);
  }

  // Update signal level (rolling buffer)
  auto sigLevel = aMsg.signalLevel();
  std::memmove(p->signalLevel + 1, p->signalLevel, sizeof(p->signalLevel) - 1);
  p->signalLevel[0] = static_cast<unsigned char>(sigLevel);

  // Update flight callsign
  auto flight = aMsg.flight();
  if (!flight.empty()) {
    std::memset(p->flight, 0, sizeof(p->flight));
    std::strncpy(p->flight, flight.data(),
                 std::min(flight.size(), sizeof(p->flight) - 1));
  }

  // Update altitude
  if (aMsg.altitude().has_value()) {
    p->altitude = *aMsg.altitude();
  }

  // Update velocity
  if (aMsg.velocity().has_value()) {
    p->speed = *aMsg.velocity();
  }

  // Update heading/track
  if (aMsg.heading().has_value()) {
    p->track = *aMsg.heading();
  }

  // Update vertical rate
  if (aMsg.vertRate().has_value()) {
    p->vert_rate = *aMsg.vertRate();
  }

  // Handle CPR position decoding
  bool isOddFrame = aMsg.hasFlag(viz1090::AircraftFlags::OddLatLonValid);
  bool isEvenFrame = aMsg.hasFlag(viz1090::AircraftFlags::EvenLatLonValid);

  if ((isOddFrame || isEvenFrame) && aMsg.rawLatitude().has_value() &&
      aMsg.rawLongitude().has_value()) {
    uint64_t msgTime = aMsg.timestamp();

    if (isOddFrame) {
      p->oddCprLat = *aMsg.rawLatitude();
      p->oddCprLon = *aMsg.rawLongitude();
      p->oddCprTime = msgTime;
      p->cprOddValid = true;
    } else {
      p->evenCprLat = *aMsg.rawLatitude();
      p->evenCprLon = *aMsg.rawLongitude();
      p->evenCprTime = msgTime;
      p->cprEvenValid = true;
    }

    // Try to decode position if we have both frames
    if (p->cprOddValid && p->cprEvenValid) {
      // Check time difference between frames (must be within 10 seconds)
      uint64_t timeDiff = (p->oddCprTime > p->evenCprTime)
                              ? (p->oddCprTime - p->evenCprTime)
                              : (p->evenCprTime - p->oddCprTime);

      if (timeDiff < kCprMaxTimeDiff) {
        viz1090::decoder::CprDecoder::CprFrame evenFrame{
            p->evenCprLat, p->evenCprLon, p->evenCprTime};
        viz1090::decoder::CprDecoder::CprFrame oddFrame{
            p->oddCprLat, p->oddCprLon, p->oddCprTime};

        // Use the most recent frame for final position
        bool useOdd = (p->oddCprTime > p->evenCprTime);

        auto decoded = viz1090::decoder::CprDecoder::decodeGlobal(
            evenFrame, oddFrame, useOdd);

        if (decoded) {
          if (p->lon == 0.0f && p->lat == 0.0f) {
            p->created = currentTime;
          }

          if (p->lon != static_cast<float>(decoded->longitude) ||
              p->lat != static_cast<float>(decoded->latitude)) {
            p->lon = static_cast<float>(decoded->longitude);
            p->lat = static_cast<float>(decoded->latitude);
            p->msSeenLatLon = currentTime;
            p->seenLatLon = std::time(nullptr);

            // Record position history
            p->lonHistory.push_back(p->lon);
            p->latHistory.push_back(p->lat);
            p->headingHistory.push_back(static_cast<float>(p->track));
            p->timestampHistory.push_back(currentTime);
          }
        }
      }
    }
  }

  // Also handle pre-decoded positions (if decoder already set them)
  if (aMsg.position().has_value()) {
    auto pos = *aMsg.position();

    if (p->lon == 0.0f && p->lat == 0.0f) {
      p->created = currentTime;
    }

    if (p->lon != static_cast<float>(pos.longitude) ||
        p->lat != static_cast<float>(pos.latitude)) {
      p->lon = static_cast<float>(pos.longitude);
      p->lat = static_cast<float>(pos.latitude);
      p->msSeenLatLon = currentTime;
      p->seenLatLon = std::time(nullptr);

      // Record position history
      p->lonHistory.push_back(p->lon);
      p->latHistory.push_back(p->lat);
      p->headingHistory.push_back(static_cast<float>(p->track));
      p->timestampHistory.push_back(currentTime);
    }
  }
}

void
AircraftList::removeStale(std::chrono::seconds aTtl) {
  auto currentTime = now();

  Aircraft* p = head;
  Aircraft* prev = nullptr;

  while (p) {
    auto age = std::chrono::duration_cast<std::chrono::seconds>(
        currentTime - p->msSeen);

    if (age > aTtl) {
      if (!prev) {
        head = p->next;
        delete p;
        p = head;
      } else {
        prev->next = p->next;
        delete p;
        p = prev->next;
      }
    } else {
      prev = p;
      p = p->next;
    }
  }
}

AircraftList::AircraftList() : head(nullptr) {}

AircraftList::~AircraftList() {
  while (head != nullptr) {
    Aircraft* temp = head;
    head = head->next;
    delete temp;
  }
}
