// dump1090, a Mode S messages decoder for RTLSDR devices.
//
// Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
//
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

#include "dump1090.h"
//
// ============================= Utility functions ==========================
//
static uint64_t mstime(void) {
    struct timeval tv;
    uint64_t mst;

    gettimeofday(&tv, NULL);
    mst = ((uint64_t)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}
//
//=========================================================================
//
// Add a new DF structure to the interactive mode linked list
//
void interactiveCreateDF(Modes *modes, struct aircraft *a, struct modesMessage *mm) {
    struct stDF *pDF = (struct stDF *) malloc(sizeof(*pDF));

    if (pDF) {
        // Default everything to zero/NULL
        memset(pDF, 0, sizeof(*pDF));

        // Now initialise things
        pDF->seen        = a->seen;
        pDF->llTimestamp = mm->timestampMsg;
        pDF->addr        = mm->addr;
        pDF->pAircraft   = a;
        memcpy(pDF->msg, mm->msg, MODES_LONG_MSG_BYTES);

        // if (!pthread_mutex_lock(&modes->pDF_mutex)) {
        //     if ((pDF->pNext = modes->pDF)) {
        //         modes->pDF->pPrev = pDF;
        //     }
        //     modes->pDF = pDF;
        //     pthread_mutex_unlock(&modes->pDF_mutex);
        // } else {
        //     free(pDF);
        // }
        if ((pDF->pNext = modes->pDF)) {
            modes->pDF->pPrev = pDF;
        }
        modes->pDF = pDF;

    }
}
//
// Remove stale DF's from the interactive mode linked list
//
void interactiveRemoveStaleDF( Modes *modes, time_t now) {
    struct stDF *pDF  = NULL;
    struct stDF *prev = NULL;

    pDF  = modes->pDF;
    while(pDF) {
        if ((now - pDF->seen) > modes->interactive_delete_ttl) {
            if (modes->pDF == pDF) {
                modes->pDF = NULL;
            } else {
                prev->pNext = NULL;
            }

            // All DF's in the list from here onwards will be time
            // expired, so delete them all
            while (pDF) {
                prev = pDF; pDF = pDF->pNext;
                free(prev);
            }

        } else {
            prev = pDF; pDF = pDF->pNext;
        }
    }
}

// struct stDF *interactiveFindDF(Modes *modes, uint32_t addr) {
//     struct stDF *pDF = NULL;

//     pDF = modes->pDF;
//     while(pDF) {
//         if (pDF->addr == addr) {
//             return (pDF);
//         }
//         pDF = pDF->pNext;
//     }


//     return (NULL);
// }
//
//========================= Interactive mode ===============================
//
// Return a new aircraft structure for the interactive mode linked list
// of aircraft
//
struct aircraft *interactiveCreateAircraft(struct modesMessage *mm) {
    struct aircraft *a = (struct aircraft *) malloc(sizeof(*a));

    // Default everything to zero/NULL
    memset(a, 0, sizeof(*a));

    // Now initialise things that should not be 0/NULL to their defaults
    a->addr = mm->addr;
    a->lat  = a->lon = 0.0;
    memset(a->signalLevel, mm->signalLevel, 8); // First time, initialise everything
                                                // to the first signal strength

    // mm->msgtype 32 is used to represent Mode A/C. These values can never change, so 
    // set them once here during initialisation, and don't bother to set them every 
    // time this ModeA/C is received again in the future
    if (mm->msgtype == 32) {
        int modeC      = ModeAToModeC(mm->modeA | mm->fs);
        a->modeACflags = MODEAC_MSG_FLAG;
        if (modeC < -12) {
            a->modeACflags |= MODEAC_MSG_MODEA_ONLY;
        } else {
            mm->altitude = modeC * 100;
            mm->bFlags  |= MODES_ACFLAGS_ALTITUDE_VALID;
        }
    }
    return (a);
}
//
//=========================================================================
//
// Return the aircraft with the specified address, or NULL if no aircraft
// exists with this address.
//
struct aircraft *interactiveFindAircraft(Modes *modes, uint32_t addr) {
    struct aircraft *a = modes->aircrafts;

    while(a) {
        if (a->addr == addr) return (a);
        a = a->next;
    }
    return (NULL);
}

//
//=========================================================================
//
// Receive new messages and populate the interactive mode with more info
//
struct aircraft *interactiveReceiveData(Modes *modes, struct modesMessage *mm) {
    struct aircraft *a, *aux;

    // Return if (checking crc) AND (not crcok) AND (not fixed)
    if (modes->check_crc && (mm->crcok == 0) && (mm->correctedbits == 0))
        return NULL;

    // Lookup our aircraft or create a new one
    a = interactiveFindAircraft(modes, mm->addr);
    if (!a) {                              // If it's a currently unknown aircraft....
        a = interactiveCreateAircraft(mm); // ., create a new record for it,
        a->next = modes->aircrafts;         // .. and put it at the head of the list
        modes->aircrafts = a;
    } else {
        /* If it is an already known aircraft, move it on head
         * so we keep aircrafts ordered by received message time.
         *
         * However move it on head only if at least one second elapsed
         * since the aircraft that is currently on head sent a message,
         * othewise with multiple aircrafts at the same time we have an
         * useless shuffle of positions on the screen. */
        if (0 && modes->aircrafts != a && (time(NULL) - a->seen) >= 1) {
            aux = modes->aircrafts;
            while(aux->next != a) aux = aux->next;
            /* Now we are a node before the aircraft to remove. */
            aux->next = aux->next->next; /* removed. */
            /* Add on head */
            a->next = modes->aircrafts;
            modes->aircrafts = a;
        }
    }

    a->signalLevel[a->messages & 7] = mm->signalLevel;// replace the 8th oldest signal strength
    a->seen      = time(NULL);
    a->timestamp = mm->timestampMsg;
    a->messages++;

    // If a (new) CALLSIGN has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_CALLSIGN_VALID) {
        memcpy(a->flight, mm->flight, sizeof(a->flight));
    }

    // If a (new) ALTITUDE has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_ALTITUDE_VALID) {
        if ( (a->modeCcount)                   // if we've a modeCcount already
          && (a->altitude  != mm->altitude ) ) // and Altitude has changed
//        && (a->modeC     != mm->modeC + 1)   // and Altitude not changed by +100 feet
//        && (a->modeC + 1 != mm->modeC    ) ) // and Altitude not changes by -100 feet
            {
            a->modeCcount   = 0;               //....zero the hit count
            a->modeACflags &= ~MODEAC_MSG_MODEC_HIT;
            }
        a->altitude = mm->altitude;
        a->modeC    = (mm->altitude + 49) / 100;
    }

    // If a (new) SQUAWK has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_SQUAWK_VALID) {
        if (a->modeA != mm->modeA) {
            a->modeAcount   = 0; // Squawk has changed, so zero the hit count
            a->modeACflags &= ~MODEAC_MSG_MODEA_HIT;
        }
        a->modeA = mm->modeA;
    }

    // If a (new) HEADING has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_HEADING_VALID) {
        a->track = mm->heading;
    }

    // If a (new) SPEED has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_SPEED_VALID) {
        a->speed = mm->velocity;
    }

    // If a (new) Vertical Descent rate has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_VERTRATE_VALID) {
        a->vert_rate = mm->vert_rate;
    }

    // if the Aircraft has landed or taken off since the last message, clear the even/odd CPR flags
    if ((mm->bFlags & MODES_ACFLAGS_AOG_VALID) && ((a->bFlags ^ mm->bFlags) & MODES_ACFLAGS_AOG)) {
        a->bFlags &= ~(MODES_ACFLAGS_LLBOTH_VALID | MODES_ACFLAGS_AOG);
    }

    // If we've got a new cprlat or cprlon
    if (mm->bFlags & MODES_ACFLAGS_LLEITHER_VALID) {
        int location_ok = 0;

        if (mm->bFlags & MODES_ACFLAGS_LLODD_VALID) {
            a->odd_cprlat  = mm->raw_latitude;
            a->odd_cprlon  = mm->raw_longitude;
            a->odd_cprtime = mstime();
        } else {
            a->even_cprlat  = mm->raw_latitude;
            a->even_cprlon  = mm->raw_longitude;
            a->even_cprtime = mstime();
        }

        // If we have enough recent data, try global CPR
        if (((mm->bFlags | a->bFlags) & MODES_ACFLAGS_LLEITHER_VALID) == MODES_ACFLAGS_LLBOTH_VALID && abs((int)(a->even_cprtime - a->odd_cprtime)) <= 10000) {
            if (decodeCPR(modes, a, (mm->bFlags & MODES_ACFLAGS_LLODD_VALID), (mm->bFlags & MODES_ACFLAGS_AOG)) == 0) {
                location_ok = 1;
            }
        }

        // Otherwise try relative CPR.
        if (!location_ok && decodeCPRrelative(modes, a, (mm->bFlags & MODES_ACFLAGS_LLODD_VALID), (mm->bFlags & MODES_ACFLAGS_AOG)) == 0) {
            location_ok = 1;
        }

        //If we sucessfully decoded, back copy the results to mm so that we can print them in list output
        if (location_ok) {
            mm->bFlags |= MODES_ACFLAGS_LATLON_VALID;
            mm->fLat    = a->lat;
            mm->fLon    = a->lon;
        }
    }

    // Update the aircrafts a->bFlags to reflect the newly received mm->bFlags;
    a->bFlags |= mm->bFlags;

    if (mm->msgtype == 32) {
        int flags = a->modeACflags;
        if ((flags & (MODEAC_MSG_MODEC_HIT | MODEAC_MSG_MODEC_OLD)) == MODEAC_MSG_MODEC_OLD) {
            //
            // This Mode-C doesn't currently hit any known Mode-S, but it used to because MODEAC_MSG_MODEC_OLD is
            // set  So the aircraft it used to match has either changed altitude, or gone out of our receiver range
            //
            // We've now received this Mode-A/C again, so it must be a new aircraft. It could be another aircraft
            // at the same Mode-C altitude, or it could be a new airctraft with a new Mods-A squawk.
            //
            // To avoid masking this aircraft from the interactive display, clear the MODEAC_MSG_MODES_OLD flag
            // and set messages to 1;
            //
            a->modeACflags = flags & ~MODEAC_MSG_MODEC_OLD;
            a->messages    = 1;
        }
    }

    // If we are Logging DF's, and it's not a Mode A/C
    if ((modes->bEnableDFLogging) && (mm->msgtype < 32)) {
        interactiveCreateDF(modes,a,mm);
    }

    return (a);
}

//
//=========================================================================
//
// When in interactive mode If we don't receive new nessages within
// MODES_INTERACTIVE_DELETE_TTL seconds we remove the aircraft from the list.
//
void interactiveRemoveStaleAircrafts(Modes *modes) {
    struct aircraft *a = modes->aircrafts;
    struct aircraft *prev = NULL;
    time_t now = time(NULL);

    // Only do cleanup once per second
    if (modes->last_cleanup_time != now) {
        modes->last_cleanup_time = now;

        interactiveRemoveStaleDF(modes,now);

        while(a) {
            if ((now - a->seen) > modes->interactive_delete_ttl) {
                // Remove the element from the linked list, with care
                // if we are removing the first element
                if (!prev) {
                    modes->aircrafts = a->next; free(a); a = modes->aircrafts;
                } else {
                    prev->next = a->next; free(a); a = prev->next;
                }
            } else {
                prev = a; a = a->next;
            }
        }
    }
}
//
//=========================================================================
//