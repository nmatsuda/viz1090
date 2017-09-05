#
# When building a package or installing otherwise in the system, make
# sure that the variable PREFIX is defined, e.g. make PREFIX=/usr/local
#
PROGNAME=dump1090

ifdef PREFIX
BINDIR=$(PREFIX)/bin
SHAREDIR=$(PREFIX)/share/$(PROGNAME)
EXTRACFLAGS=-DHTMLPATH=\"$(SHAREDIR)\"
endif

CFLAGS=-O2 -g -Wall -W `pkg-config --cflags librtlsdr` -std=c99
LIBS=`pkg-config --libs librtlsdr` -lpthread -lm `sdl-config --libs` -lSDL -lSDL_ttf -lSDL_gfx -lwiringPi
CC=gcc


all: view1090

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRACFLAGS) -c $<

view1090: view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o input.o draw.o font.o init.o  
	$(CC) -g -o view1090 view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o input.o draw.o font.o init.o $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o view1090
