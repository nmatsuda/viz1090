#
# When building a package or installing otherwise in the system, make
# sure that the variable PREFIX is defined, e.g. make PREFIX=/usr/local
#

CXXFLAGS=-O2 -std=c++11 -g
LIBS= -lm -lSDL2 -lSDL2_ttf -lSDL2_gfx -lpthread -g
CXX=g++

all: viz1090

%.o: %.c %.cpp
	$(CXX) $(CXXFLAGS) $(EXTRACFLAGS) -c $<

viz1090: viz1090.o AppData.o AircraftList.o Aircraft.o Label.o AircraftLabel.o anet.o interactive.o mode_ac.o mode_s.o net_io.o Input.o View.o Map.o parula.o monokai.o 
	$(CXX) -o viz1090 viz1090.o AppData.o AircraftList.o Aircraft.o Label.o AircraftLabel.o anet.o interactive.o mode_ac.o mode_s.o net_io.o Input.o View.o Map.o parula.o monokai.o $(LIBS) $(LDFLAGS)

clean:
	rm -f \
		airportdata.bin \
		airportnames \
		mapdata/* \
		mapdata.bin \
		mapnames \
		*.o \
		viz1090
