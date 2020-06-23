#!/bin/bash

wget https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/10m/cultural/ne_10m_admin_1_states_provinces.zip
wget https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/10m/cultural/ne_10m_populated_places.zip
wget https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/10m/cultural/ne_10m_airports.zip

#this may not be up to date
wget https://opendata.arcgis.com/datasets/4d8fa46181aa470d809776c57a8ab1f6_0.zip  

unzip '*.zip'

python3 mapconverter.py --mapfile ne_10m_admin_1_states_provinces.shp --mapnames ne_10m_populated_places.shp --airportfile Runways.shp --airportnames ne_10m_airports.shp 