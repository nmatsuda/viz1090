from lxml import etree as ElementTree
import numpy as np
import sys
from tqdm import tqdm 

filename = sys.argv[1]

if(len(filename) == 0):
	print("No input filename given")
	exit()

parser = ElementTree.XMLParser(recover=True)
tree = ElementTree.parse(filename, parser)
polys = tree.xpath('//polygon')

bin_file = open("mapdata.bin", "wb")

outlist = []


precision = 2

print("Reading points")
#for i in tqdm(range(len(polys))):
for i in range(40):
	p = polys[i]
	currentPoints = (p.attrib['points']).replace(","," ").split()

	if(len(currentPoints) == 14): #remove little circles in the McCurley maps
		continue

	prevx = 0
	prevy = 0

	temp = []

	for i in range(int(len(currentPoints)/2)):
		currentPoints[2 * i + 0] = "%.*f" % (precision, float(currentPoints[2 * i + 0]))
		currentPoints[2 * i + 1] = "%.*f" % (precision, float(currentPoints[2 * i + 1]))

		if(currentPoints[2 * i + 0] != prevx and currentPoints[2 * i + 1] != prevy):
			temp.extend([currentPoints[2 * i + 0],currentPoints[2 * i + 1]])
	
		prevx = currentPoints[2 * i + 0]
		prevy = currentPoints[2 * i + 1] 


	if(len(currentPoints) > 14): 
		outlist.extend(temp)
		outlist.extend(["0","0"])


np.asarray(outlist).astype(np.single).tofile(bin_file)
bin_file.close()

print("Wrote %d points" % (len(outlist) / 2))