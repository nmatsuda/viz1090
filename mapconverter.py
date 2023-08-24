import fiona
from shapely.geometry import shape
import numpy as np
from tqdm import tqdm 
import zipfile
from io import BytesIO
#from urllib.request import urlopen
import requests
import argparse
import os

def convertLinestring(linestring):
	outlist = []

	pointx = linestring.coords.xy[0]
	pointy = linestring.coords.xy[1]
			
	for j in range(len(pointx)):
		outlist.extend([float(pointx[j]),float(pointy[j])])

	outlist.extend([0,0])
	return outlist

def extractLines(shapefile, tolerance):
	print("Extracting map lines")
	outlist = []

	for i in tqdm(range(len(shapefile))):

		if(tolerance > 0):
			simplified = shape(shapefile[i]['geometry']).simplify(tolerance, preserve_topology=False)
		else:
			simplified =shape(shapefile[i]['geometry'])

		if(simplified.geom_type == "LineString"):
			outlist.extend(convertLinestring(simplified))
			
		elif(simplified.geom_type == "MultiPolygon" or simplified.geom_type == "Polygon"):

			if(simplified.boundary.geom_type == "MultiLineString"):
				for boundary in simplified.boundary.geoms:
					outlist.extend(convertLinestring(boundary))
			else:
				outlist.extend(convertLinestring(simplified.boundary))
	
		else:
			print("Unsupported type: " + simplified.geom_type)



	return outlist

parser = argparse.ArgumentParser(description='viz1090 Natural Earth Data Map Converter')
parser.add_argument("--mapfile", type=str, help="shapefile for main map")	
parser.add_argument("--mapnames", type=str, help="shapefile for map place names")
parser.add_argument("--airportfile", type=str, help="shapefile for airport runway outlines")
parser.add_argument("--airportnames", type=str, help="shapefile for airport IATA names")
parser.add_argument("--minpop", default=100000, type=int, help="map simplification tolerance")
parser.add_argument("--tolerance", default=0.001, type=float, help="map simplification tolerance")

args = parser.parse_args()

# mapfile
if args.mapfile is not None:
	shapefile = fiona.open(args.mapfile)

	outlist = extractLines(shapefile, args.tolerance)

	bin_file = open("mapdata.bin", "wb")
	np.asarray(outlist).astype(np.single).tofile(bin_file)
	bin_file.close()

	print("Wrote %d points" % (len(outlist) / 2))

# mapnames
bin_file = open("mapnames", "w")

if args.mapnames is not None:
	shapefile = fiona.open(args.mapnames)

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

#airportfile
if args.airportfile is not None:
	shapefile = fiona.open(args.airportfile)

	outlist = extractLines(shapefile, 0)

	bin_file = open("airportdata.bin", "wb")
	np.asarray(outlist).astype(np.single).tofile(bin_file)
	bin_file.close()

	print("Wrote %d points" % (len(outlist) / 2))	


#airportnames
if args.airportnames is not None:
	bin_file = open("airportnames", "w")

	shapefile = fiona.open(args.airportnames)

	count = 0

	for i in tqdm(range(len(shapefile))):

		xcoord = shapefile[i]['geometry']['coordinates'][0]
		ycoord = shapefile[i]['geometry']['coordinates'][1]
		name = shapefile[i]['properties']['iata_code']

		outstring = "{0} {1} {2}\n".format(xcoord, ycoord, name)
		bin_file.write(outstring)
		count = count + 1

	bin_file.close()

	print("Wrote %d airport names" % count)

