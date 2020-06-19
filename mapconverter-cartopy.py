import cartopy.io.shapereader as shpreader
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

		simplified = shapefile[i].geometry.simplify(tolerance, preserve_topology=False)

		if(simplified.geom_type == "LineString"):
			outlist.extend(convertLinestring(simplified))
			
		elif(simplified.geom_type == "MultiPolygon" or simplified.geom_type == "Polygon"):

			if(simplified.boundary.geom_type == "MultiLineString"):
				for boundary in simplified.boundary:
					outlist.extend(convertLinestring(boundary))
			else:
				outlist.extend(convertLinestring(simplified.boundary))
	
		else:
			print("Unsupported type: " + simplified.geom_type)



	return outlist

parser = argparse.ArgumentParser(description='viz1090 Natural Earth Data Map Converter')
parser.add_argument("--tolerance", default=0.001, type=float, help="map simplification tolerance")
parser.add_argument("--scale", default="10m", choices=["10m","50m","110m"], type=str, help="map file scale")
parser.add_argument("mapfile", type=str, help="shapefile to load (e.g., from https://www.naturalearthdata.com/downloads/")	

args = parser.parse_args()

shapefile = list(shpreader.Reader(args.mapfile).records())

outlist = extractLines(shapefile, args.tolerance)

bin_file = open("mapdata.bin", "wb")
np.asarray(outlist).astype(np.single).tofile(bin_file)
bin_file.close()

print("Wrote %d points" % (len(outlist) / 2))