#!/bin/bash
../dump1090/dump1090 --fix --aggressive --net --quiet &
./viz1090 --fps --fullscreen --screensize 240 320 --lat 47.6 --lon -122.3
