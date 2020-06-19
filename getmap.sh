#!/bin/bash

wget https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/10m/cultural/ne_10m_admin_1_states_provinces.zip
unzip ne_10m_admin_1_states_provinces.zip
python3 mapconverter.py ne_10m_admin_1_states_provinces.shp
