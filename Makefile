#
# When building a package or installing otherwise in the system, make
# sure that the variable PREFIX is defined, e.g. make PREFIX=/usr/local
#

CFLAGS=-O2 -Wno-write-strings
LIBS=-lm -lSDL2 -lSDL2_ttf -lSDL2_gfx
CC=g++

all: viz1090

%.o: %.c %.cpp
	$(CC) $(CFLAGS) $(EXTRACFLAGS) -c $<

viz1090: viz1090.o AppData.o AircraftList.o Aircraft.o anet.o interactive.o mode_ac.o mode_s.o net_io.o Input.o View.o Map.o parula.o monokai.o 
	$(CC) -g -o viz1090 viz1090.o AppData.o AircraftList.o Aircraft.o anet.o interactive.o mode_ac.o mode_s.o net_io.o Input.o View.o Map.o parula.o monokai.o $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o viz1090
