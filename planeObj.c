#include "structs.h"
#include "dump1090.h"

static uint64_t mstime(void) {
    struct timeval tv;
    uint64_t mst;

    gettimeofday(&tv, NULL);
    mst = ((uint64_t)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}

struct planeObj *findPlaneObj(uint32_t addr) {
    struct planeObj *p = planes;

    while(p) {
        if (p->addr == addr) return (p);
        p = p->next;
    }
    return (NULL);
}

struct planeObj *createPlaneObj(struct aircraft *a) {
    struct planeObj *p = (struct planeObj *) malloc(sizeof(*p));

    memset(p, 0, sizeof(*p));

    p->addr = a->addr;
    p->created = 0;
    p->oldIdx = 0;
    p->prev_seen = 0;

    p->x = 0;
    p->y = 0;
    p->cx = 0;
    p->cy = 0;

    p->ox = 0;
    p->oy = 0;
    p->dox = 0;
    p->doy  = 0;
    p->ddox = 0;
    p->ddoy = 0;

    memset(p->oldLon, 0, sizeof(p->oldLon));
    memset(p->oldLat, 0, sizeof(p->oldLat));    
    memset(p->oldHeading, 0, sizeof(p->oldHeading));    

    return (p);
}

void updatePlanes() {
    struct aircraft *a = Modes.aircrafts;

    struct planeObj *p = planes;

    while(p) {
        p->live = 0;
        p = p->next;
    }

    while(a) {

        p = findPlaneObj(a->addr);
        if (!p) {
            p = createPlaneObj(a);
            p->next = planes;       
            planes = p;      
        } else {
            p->prev_seen = p->seen;
        }

        p->live = 1;

        if(p->seen == a->seen) {
            a = a->next;
            continue;
        }

        p->seen = a->seen;            
        p->msSeen = mstime();

        if((p->seen - p->prev_seen) > 0) {
                p->messageRate = 1.0 / (double)(p->seen - p->prev_seen);
        }

        memcpy(p->flight, a->flight, sizeof(p->flight));
        memcpy(p->signalLevel, a->signalLevel, sizeof(p->signalLevel));

        p->altitude = a->altitude;
        p->speed =  a->speed;          
        p->track = a->track;         
        p->vert_rate = a->vert_rate;    
        p->lon = a->lon;
        p->lat = a->lat;

        if(p->seenLatLon < a->seenLatLon) {
            p->msSeenLatLon = mstime();

            p->oldIdx = (p->oldIdx+1) % 32;

            p->oldLon[p->oldIdx] = p->lon;
            p->oldLat[p->oldIdx] = p->lat;

            p->oldHeading[p->oldIdx] = p->track;

            p->oldSeen[p->oldIdx] = p->seenLatLon;
        }
        
        p->seenLatLon = a->seenLatLon;
    
        a = a->next;
    }

    p = planes;
    struct planeObj *prev = NULL;

    while(p) {
        if(!p->live) {
            if (!prev) {
                planes = p->next; 
                free(p); 
                p = planes; 
            } else {
                prev->next = p->next; 
                free(p); 
                p = prev->next;
            }
        } else {
            prev = p;
            p = p->next;
        }
    }
}