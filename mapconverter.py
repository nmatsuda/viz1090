import geopandas
import numpy as np
from tqdm import tqdm 

outlist = []

tolerance = .05

coast = geopandas.read_file("ne_10m_coastline.shp")

coast_simple = coast['geometry'].simplify(tolerance, preserve_topology=False)

for i in tqdm(range(len(coast_simple))):
	pointx = coast_simple[i].coords.xy[0]
	pointy = coast_simple[i].coords.xy[1]

	for j in range(len(pointx)):
		outlist.extend([float(pointx[j]),float(pointy[j])])

	outlist.extend([0,0])

bin_file = open("mapdata.bin", "wb")
np.asarray(outlist).astype(np.single).tofile(bin_file)
bin_file.close()

print("Wrote %d points" % (len(outlist) / 2))

# import json
# import numpy as np
# import sys
# from tqdm import tqdm 
# import argparse

# parser = argparse.ArgumentParser(description='viz1090 SVG Map Converter')
# parser.add_argument("--resolution", default=250, type=int, help="downsample resolution")
# parser.add_argument("file", nargs="+", help="filename")

# args = parser.parse_args()

# if(len(args.file) == 0):
# 	print("No input filename given")
# 	exit()

# bin_file = open("mapdata.bin", "wb")

# outlist = []

# resolution = args.resolution

# for file in args.file:
# 	with open(file, "r") as read_file:
# 	    data = json.load(read_file)



# 	print("Reading points")
# 	for i in tqdm(range(len(data['features']))):


# 		if(data['features'][i]['geometry']['type'] == 'LineString'):
# 			prevx = 0
# 			prevy = 0

# 			temp = []

# 			for currentPoint in data['features'][i]['geometry']['coordinates']:

# 				currentx = float(int(resolution * float(currentPoint[0]))) / resolution		
# 				currenty = float(int(resolution * float(currentPoint[1]))) / resolution

# 				if(currentx != prevx or currenty != prevy):
# 					temp.extend([currentx,currenty])

# 				prevx = currentx
# 				prevy = currenty
# 			temp.extend(["0","0"])
# 		else:
# 			prevx = 0
# 			prevy = 0

# 			temp = []

# 			for currentLine in data['features'][i]['geometry']['coordinates']:
# 				for currentPoint in currentLine:
						
# 					currentx = float(int(resolution * float(currentPoint[0]))) / resolution		
# 					currenty = float(int(resolution * float(currentPoint[1]))) / resolution

# 					if(currentx != prevx or currenty != prevy):
# 						temp.extend([currentx,currenty])

# 					prevx = currentx
# 					prevy = currenty

# 				temp.extend(["0","0"])


# 		outlist.extend(temp)

# np.asarray(outlist).astype(np.single).tofile(bin_file)
# bin_file.close()

# print("Wrote %d points" % (len(outlist) / 2))
