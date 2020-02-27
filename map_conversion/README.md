Very hacky map pipeline:

using latlon maps from sourced from http://www.mccurley.org/svg/

get all.svg

run  mapconverted.py

**at this point you have to manually format into a c array and get the length.**

now compile write2bin
gcc write2bin.c allstates.c -o write2bin

and run write2bin

this will produce mapdata.bin that the main program reads in


** this should be one python that generates the binary file **

** then it should be grabbed from mapbox or something **
