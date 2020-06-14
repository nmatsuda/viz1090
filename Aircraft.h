 #include <stdint.h>

#include <ctime>
#include <vector> 
#include <chrono>

class Aircraft {
public:	
    uint32_t        addr;           // ICAO address
    char            flight[16];     // Flight number
    unsigned char   signalLevel[8]; // Last 8 Signal Amplitudes
    double          messageRate;
    int             altitude;       // Altitude
    int             speed;          // Velocity
    int             track;          // Angle of flight
    int             vert_rate;      // Vertical rate.
    time_t          seen;           // Time at which the last packet was received
    time_t          seenLatLon;           // Time at which the last packet was received
    time_t          prev_seen;
    double          lat, lon;       // Coordinated obtained from CPR encoded data
    
    //history

    std::vector <float>   lonHistory, latHistory, headingHistory;
    std::vector <std::chrono::high_resolution_clock::time_point> timestampHistory;

    // float           oldLon[TRAIL_LENGTH];
    // float           oldLat[TRAIL_LENGTH];
    // float           oldHeading[TRAIL_LENGTH];
    // time_t          oldSeen[TRAIL_LENGTH];
    // uint8_t         oldIdx; 
    std::chrono::high_resolution_clock::time_point        created;
    std::chrono::high_resolution_clock::time_point        msSeen;
    std::chrono::high_resolution_clock::time_point        msSeenLatLon;
    int             live;

    struct Aircraft *next;        // Next aircraft in our linked list

//// label stuff -> should go to aircraft icon  class

    int             x, y, cx, cy, w, h;
    float           ox, oy, dox, doy, ddox, ddoy;
    float           pressure;

/// methods

    Aircraft(uint32_t addr);  
    ~Aircraft();
};