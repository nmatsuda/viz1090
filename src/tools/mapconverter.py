#!/usr/bin/env python3
"""
viz1090 Natural Earth Data Map Converter

Converts Natural Earth shapefiles to binary format for viz1090.
Handles missing input files gracefully by skipping the associated output.
"""

import argparse
import os
import sys

import fiona
import numpy as np
from shapely.geometry import shape
from tqdm import tqdm


def convert_linestring(linestring):
    """Convert a LineString to a flat list of coordinates."""
    outlist = []
    pointx = linestring.coords.xy[0]
    pointy = linestring.coords.xy[1]

    for j in range(len(pointx)):
        outlist.extend([float(pointx[j]), float(pointy[j])])

    outlist.extend([0, 0])  # Terminator
    return outlist


def extract_lines(shapefile, tolerance):
    """Extract line data from shapefile geometries."""
    print("Extracting map lines")
    outlist = []

    for i in tqdm(range(len(shapefile))):
        if tolerance > 0:
            simplified = shape(shapefile[i]['geometry']).simplify(
                tolerance, preserve_topology=False)
        else:
            simplified = shape(shapefile[i]['geometry'])

        if simplified.geom_type == "LineString":
            outlist.extend(convert_linestring(simplified))
        elif simplified.geom_type in ("MultiPolygon", "Polygon"):
            if simplified.boundary.geom_type == "MultiLineString":
                for boundary in simplified.boundary.geoms:
                    outlist.extend(convert_linestring(boundary))
            else:
                outlist.extend(convert_linestring(simplified.boundary))
        else:
            print(f"Unsupported type: {simplified.geom_type}")

    return outlist


def process_mapfile(mapfile, tolerance, output_path):
    """Process main map shapefile to binary format."""
    if not os.path.exists(mapfile):
        print(f"Warning: Map file not found: {mapfile}, skipping mapdata.bin")
        return False

    try:
        shapefile = fiona.open(mapfile)
        outlist = extract_lines(shapefile, tolerance)

        with open(output_path, "wb") as bin_file:
            np.asarray(outlist).astype(np.single).tofile(bin_file)

        print(f"Wrote {len(outlist) // 2} points to {output_path}")
        return True
    except Exception as e:
        print(f"Error processing map file: {e}")
        return False


def process_mapnames(mapnames_file, minpop, output_path):
    """Process populated places shapefile to text format."""
    if not os.path.exists(mapnames_file):
        print(f"Warning: Map names file not found: {mapnames_file}, skipping mapnames")
        return False

    try:
        shapefile = fiona.open(mapnames_file)
        count = 0

        with open(output_path, "w") as out_file:
            for i in tqdm(range(len(shapefile))):
                xcoord = shapefile[i]['geometry']['coordinates'][0]
                ycoord = shapefile[i]['geometry']['coordinates'][1]
                pop = shapefile[i]['properties']['POP_MIN']
                name = shapefile[i]['properties']['NAME']

                if pop > minpop:
                    out_file.write(f"{xcoord} {ycoord} {name}\n")
                    count += 1

        print(f"Wrote {count} place names to {output_path}")
        return True
    except Exception as e:
        print(f"Error processing map names: {e}")
        return False


def process_airportfile(airportfile, output_path):
    """Process airport runway shapefile to binary format."""
    if not os.path.exists(airportfile):
        print(f"Warning: Airport file not found: {airportfile}, skipping airportdata.bin")
        return False

    try:
        shapefile = fiona.open(airportfile)
        outlist = extract_lines(shapefile, 0)

        with open(output_path, "wb") as bin_file:
            np.asarray(outlist).astype(np.single).tofile(bin_file)

        print(f"Wrote {len(outlist) // 2} points to {output_path}")
        return True
    except Exception as e:
        print(f"Error processing airport file: {e}")
        return False


def process_airportnames(airportnames_file, output_path):
    """Process airport names shapefile to text format."""
    if not os.path.exists(airportnames_file):
        print(f"Warning: Airport names file not found: {airportnames_file}, skipping airportnames")
        return False

    try:
        shapefile = fiona.open(airportnames_file)
        count = 0

        with open(output_path, "w") as out_file:
            for i in tqdm(range(len(shapefile))):
                xcoord = shapefile[i]['geometry']['coordinates'][0]
                ycoord = shapefile[i]['geometry']['coordinates'][1]
                name = shapefile[i]['properties']['iata_code']

                out_file.write(f"{xcoord} {ycoord} {name}\n")
                count += 1

        print(f"Wrote {count} airport names to {output_path}")
        return True
    except Exception as e:
        print(f"Error processing airport names: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(
        description='viz1090 Natural Earth Data Map Converter')
    parser.add_argument("--mapfile", type=str,
                        help="shapefile for main map")
    parser.add_argument("--mapnames", type=str,
                        help="shapefile for map place names")
    parser.add_argument("--airportfile", type=str,
                        help="shapefile for airport runway outlines")
    parser.add_argument("--airportnames", type=str,
                        help="shapefile for airport IATA names")
    parser.add_argument("--minpop", default=100000, type=int,
                        help="minimum population for place names")
    parser.add_argument("--tolerance", default=0.001, type=float,
                        help="map simplification tolerance")
    parser.add_argument("--output-dir", default=".",
                        help="output directory for generated files")

    args = parser.parse_args()

    # Ensure output directory exists
    os.makedirs(args.output_dir, exist_ok=True)

    success_count = 0
    total_count = 0

    # Process map file
    if args.mapfile is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "mapdata.bin")
        if process_mapfile(args.mapfile, args.tolerance, output_path):
            success_count += 1

    # Process map names
    if args.mapnames is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "mapnames")
        if process_mapnames(args.mapnames, args.minpop, output_path):
            success_count += 1

    # Process airport file
    if args.airportfile is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "airportdata.bin")
        if process_airportfile(args.airportfile, output_path):
            success_count += 1

    # Process airport names
    if args.airportnames is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "airportnames")
        if process_airportnames(args.airportnames, output_path):
            success_count += 1

    print(f"\nCompleted: {success_count}/{total_count} outputs generated successfully")

    # Return success if at least one output was generated
    return 0 if success_count > 0 else 1


if __name__ == "__main__":
    sys.exit(main())
