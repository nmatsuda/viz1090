#ifndef AIRCRAFTDATA_H
#define AIRCRAFTDATA_H

#include "dump1090.h"
#include "view1090.h"
#include "structs.h"

#include "AircraftList.h"

class AircraftData {
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
		AircraftData();

		AircraftList aircraftList;
		Aircraft *selectedAircraft;
		Modes modes;

		char server[32];
};

#endif