#include "Aircraft.h"

#include "dump1090.h" //for struct aircaft


Aircraft::Aircraft(struct aircraft *a) {
    addr = a->addr;
    created = 0;
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
}


Aircraft::~Aircraft() {
}