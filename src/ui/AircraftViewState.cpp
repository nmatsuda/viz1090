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

#include "ui/AircraftViewState.h"
#include "ui/AircraftLabel.h"

namespace viz1090 {
namespace ui {

AircraftViewState::~AircraftViewState() = default;

AircraftViewState&
AircraftViewStateMap::getOrCreate(uint32_t addr) {
  return states_[addr];
}

AircraftViewState*
AircraftViewStateMap::get(uint32_t addr) {
  auto it = states_.find(addr);
  return (it != states_.end()) ? &it->second : nullptr;
}

const AircraftViewState*
AircraftViewStateMap::get(uint32_t addr) const {
  auto it = states_.find(addr);
  return (it != states_.end()) ? &it->second : nullptr;
}

void
AircraftViewStateMap::removeStale(const std::unordered_map<uint32_t, bool>& activeAddrs) {
  for (auto it = states_.begin(); it != states_.end();) {
    if (activeAddrs.find(it->first) == activeAddrs.end()) {
      it = states_.erase(it);
    } else {
      ++it;
    }
  }
}

void
AircraftViewStateMap::clear() {
  states_.clear();
}

}  // namespace ui
}  // namespace viz1090
