#ifndef APPDATA_H
#define APPDATA_H

#include "view1090.h" //for Modes

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
	    int numPlanes;
	    double maxDist;
	    int totalCount;
	    double avgSig;
	    double sigAccumulate;
	    double msgRate;
	    double msgRateAccumulate;    
};

#endif