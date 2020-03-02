from lxml import etree as ElementTree
import numpy as np
import sys

filename = sys.argv[1]

if(len(filename) == 0):
	print "No input filename given"
	exit()

parser = ElementTree.XMLParser(recover=True)
tree = ElementTree.parse(filename, parser)
polys = tree.xpath('//polygon')

bin_file = open("mapdata.bin", "wt")

outlist = []

for p in polys:
	currentPoints = (p.attrib['points']).replace(","," ").split()

	if(len(currentPoints) > 12): #remove little circles in the McCurley maps
		outlist.extend(currentPoints)
		outlist.extend(["0","0"])

np.asarray(outlist).astype(np.single).tofile(bin_file)
bin_file.close()