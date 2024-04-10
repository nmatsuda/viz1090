#!/bin/bash

mkdir -p mapdata
pushd mapdata > /dev/null

wget --no-verbose https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_1_states_provinces.zip
wget --no-verbose https://naciscdn.org/naturalearth/10m/cultural/ne_10m_populated_places.zip
wget --no-verbose https://naciscdn.org/naturalearth/10m/cultural/ne_10m_airports.zip

#this may not be up to date
wget --no-verbose https://opendata.arcgis.com/datasets/4d8fa46181aa470d809776c57a8ab1f6_0.zip

for file in *.zip; do
    unzip -o "${file}"
    rm "${file}"
done

popd > /dev/null

python3 mapconverter.py \
	--mapfile mapdata/ne_10m_admin_1_states_provinces.shp \
	--mapnames mapdata/ne_10m_populated_places.shp \
	--airportfile mapdata/Runways.shp \
	--airportnames mapdata/ne_10m_airports.shp 
