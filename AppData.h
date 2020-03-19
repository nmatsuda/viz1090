#ifndef APPDATA_H
#define APPDATA_H

#include "dump1090.h"
#include "view1090.h"
#include "structs.h"

#include "AircraftList.h"

class AppData {
	private:
		//from view1090.c
	
		int setupConnection(struct client *c);

		//

	    struct client *c;
	    int fd;
        char pk_buf[8];

	public:
		void initialize();
		void connect();
		void disconnect();
		void update();
		void updateStatus();
		AppData();

		AircraftList aircraftList;
		Modes modes;

		char server[32];

	    int numVisiblePlanes;
	    double maxDist;
	    int totalCount;
	    double sigAccumulate;
	    double msgRateAccumulate;    
};

#endif