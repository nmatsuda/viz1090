#include "Aircraft.h"

#include "dump1090.h" //for Modes

class AircraftList {
	public:
		Aircraft *head;

		Aircraft *find(uint32_t addr);
		void update(Modes *modes);

		AircraftList();
		~AircraftList();
};
