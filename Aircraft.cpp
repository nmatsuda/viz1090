#include "Aircraft.h"

static auto now() {
    return std::chrono::high_resolution_clock::now();
}

Aircraft::Aircraft(uint32_t addr) {
    this->addr = addr;
    // created = now();
    prev_seen = 0;

    x = 0;
    y = 0;
    cx = 0;
    cy = 0;

    ox = 0;
    oy = 0;
    dox = 0;
    doy  = 0;
    ddox = 0;
    ddoy = 0;

    lon = 0;
    lat = 0;
}


Aircraft::~Aircraft() {
}