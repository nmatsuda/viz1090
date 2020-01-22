from lxml import etree as ElementTree

parser = ElementTree.XMLParser(recover=True)
tree = ElementTree.parse('all.svg', parser)
polys = tree.xpath('//polygon')

text_file = open("test.txt", "wt")

for p in polys:
	currentPoints = p.attrib['points']
	currentPoints = currentPoints.replace(" ", ",")
	text_file.write(currentPoints + ",0,0")

text_file.close()
