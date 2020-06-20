import fiona
from tqdm import tqdm 
import argparse
import os



bin_file = open("mapnames", "w")

parser = argparse.ArgumentParser(description='viz1090 Natural Earth Data Map Converter')
parser.add_argument("--minpop", default=100000, type=int, help="map simplification tolerance")
parser.add_argument("--scale", default="10m", choices=["10m","50m","110m"], type=str, help="map file scale")
parser.add_argument("mapfile", type=str, help="shapefile to load (e.g., from https://www.naturalearthdata.com/downloads/")	

args = parser.parse_args()

shapefile = fiona.open(args.mapfile)

count = 0

for i in tqdm(range(len(shapefile))):

	xcoord = shapefile[i]['geometry']['coordinates'][0]
	ycoord = shapefile[i]['geometry']['coordinates'][1]
	pop = shapefile[i]['properties']['POP_MIN']
	name = shapefile[i]['properties']['NAME']

	if pop > args.minpop:
		outstring = "{0} {1} {2}\n".format(xcoord, ycoord, name)
		bin_file.write(outstring)
		count = count + 1

bin_file.close()

print("Wrote %d place names" % count)