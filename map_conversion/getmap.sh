#!/bin/bash

wget -O all.svg.gz https://www.mccurley.org/svg/data/allzips.svgz
gunzip all.svg.gz
python mapconverter.py all.svg