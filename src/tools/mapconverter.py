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


def convert_polygon_ring(ring):
    """Convert a polygon ring (exterior or hole) to a flat list of coordinates."""
    outlist = []
    pointx = ring.coords.xy[0]
    pointy = ring.coords.xy[1]

    for j in range(len(pointx)):
        outlist.extend([float(pointx[j]), float(pointy[j])])

    outlist.extend([0, 0])  # Terminator
    return outlist


def extract_polygons(shapefile, tolerance):
    """Extract polygon data from shapefile geometries for filling."""
    print("Extracting polygons")
    outlist = []

    for i in tqdm(range(len(shapefile))):
        if tolerance > 0:
            simplified = shape(shapefile[i]['geometry']).simplify(
                tolerance, preserve_topology=False)
        else:
            simplified = shape(shapefile[i]['geometry'])

        if simplified.geom_type == "Polygon":
            # Extract exterior ring only (holes would need special handling)
            outlist.extend(convert_polygon_ring(simplified.exterior))
        elif simplified.geom_type == "MultiPolygon":
            for polygon in simplified.geoms:
                outlist.extend(convert_polygon_ring(polygon.exterior))
        else:
            print(f"Unsupported polygon type: {simplified.geom_type}")

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
        elif simplified.geom_type == "MultiLineString":
            for linestring in simplified.geoms:
                outlist.extend(convert_linestring(linestring))
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


def process_landfile(landfile, tolerance, output_path):
    """Process land polygons shapefile to binary format for filling."""
    if not os.path.exists(landfile):
        print(f"Warning: Land file not found: {landfile}, skipping landdata.bin")
        return False

    try:
        shapefile = fiona.open(landfile)
        outlist = extract_polygons(shapefile, tolerance)

        with open(output_path, "wb") as bin_file:
            np.asarray(outlist).astype(np.single).tofile(bin_file)

        print(f"Wrote {len(outlist) // 2} polygon vertices to {output_path}")
        return True
    except Exception as e:
        print(f"Error processing land file: {e}")
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


def process_countryfile(countryfile, disputedfile, tolerance, output_path):
    """Process country boundaries shapefile to binary format, optionally including disputed boundaries."""
    outlist = []

    # Process main country boundaries
    if countryfile and os.path.exists(countryfile):
        try:
            shapefile = fiona.open(countryfile)
            outlist.extend(extract_lines(shapefile, tolerance))
            print(f"Processed {len(shapefile)} features from country boundaries")
        except Exception as e:
            print(f"Error processing country file: {e}")
    elif countryfile:
        print(f"Warning: Country file not found: {countryfile}")

    # Process disputed boundaries and merge
    if disputedfile and os.path.exists(disputedfile):
        try:
            shapefile = fiona.open(disputedfile)
            outlist.extend(extract_lines(shapefile, tolerance))
            print(f"Processed {len(shapefile)} features from disputed boundaries")
        except Exception as e:
            print(f"Error processing disputed file: {e}")
    elif disputedfile:
        print(f"Warning: Disputed file not found: {disputedfile}")

    if not outlist:
        print("Warning: No country boundary data to write, skipping countrydata.bin")
        return False

    try:
        with open(output_path, "wb") as bin_file:
            np.asarray(outlist).astype(np.single).tofile(bin_file)
        print(f"Wrote {len(outlist) // 2} points to {output_path}")
        return True
    except Exception as e:
        print(f"Error writing country data: {e}")
        return False


def process_coastlinefile(coastlinefile, tolerance, output_path):
    """Process coastline shapefile to binary format."""
    if not os.path.exists(coastlinefile):
        print(f"Warning: Coastline file not found: {coastlinefile}, skipping coastlinedata.bin")
        return False

    try:
        shapefile = fiona.open(coastlinefile)
        outlist = extract_lines(shapefile, tolerance)

        with open(output_path, "wb") as bin_file:
            np.asarray(outlist).astype(np.single).tofile(bin_file)

        print(f"Wrote {len(outlist) // 2} points to {output_path}")
        return True
    except Exception as e:
        print(f"Error processing coastline file: {e}")
        return False


def process_riverfile(riverfile, tolerance, output_path):
    """Process rivers shapefile to binary format."""
    if not os.path.exists(riverfile):
        print(f"Warning: River file not found: {riverfile}, skipping riverdata.bin")
        return False

    try:
        shapefile = fiona.open(riverfile)
        outlist = extract_lines(shapefile, tolerance)

        with open(output_path, "wb") as bin_file:
            np.asarray(outlist).astype(np.single).tofile(bin_file)

        print(f"Wrote {len(outlist) // 2} points to {output_path}")
        return True
    except Exception as e:
        print(f"Error processing river file: {e}")
        return False


def process_lakefile(lakefile, tolerance, output_path):
    """Process lakes shapefile to binary format."""
    if not os.path.exists(lakefile):
        print(f"Warning: Lake file not found: {lakefile}, skipping lakedata.bin")
        return False

    try:
        shapefile = fiona.open(lakefile)
        outlist = extract_lines(shapefile, tolerance)

        with open(output_path, "wb") as bin_file:
            np.asarray(outlist).astype(np.single).tofile(bin_file)

        print(f"Wrote {len(outlist) // 2} points to {output_path}")
        return True
    except Exception as e:
        print(f"Error processing lake file: {e}")
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
    parser.add_argument("--landfile", type=str,
                        help="shapefile for land polygons (for filling)")
    parser.add_argument("--mapfile", type=str,
                        help="shapefile for main map (state/province boundaries)")
    parser.add_argument("--countryfile", type=str,
                        help="shapefile for country boundaries (national borders)")
    parser.add_argument("--disputedfile", type=str,
                        help="shapefile for disputed country boundaries (merged with countryfile)")
    parser.add_argument("--coastlinefile", type=str,
                        help="shapefile for coastlines (land-sea boundaries)")
    parser.add_argument("--riverfile", type=str,
                        help="shapefile for rivers and lake centerlines")
    parser.add_argument("--lakefile", type=str,
                        help="shapefile for lake boundaries")
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

    # Process land file (polygons for filling)
    if args.landfile is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "landdata.bin")
        if process_landfile(args.landfile, args.tolerance, output_path):
            success_count += 1

    # Process map file (state/province boundaries)
    if args.mapfile is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "mapdata.bin")
        if process_mapfile(args.mapfile, args.tolerance, output_path):
            success_count += 1

    # Process country file (national boundaries) with optional disputed boundaries
    if args.countryfile is not None or args.disputedfile is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "countrydata.bin")
        if process_countryfile(args.countryfile, args.disputedfile, args.tolerance, output_path):
            success_count += 1

    # Process coastline file
    if args.coastlinefile is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "coastlinedata.bin")
        if process_coastlinefile(args.coastlinefile, args.tolerance, output_path):
            success_count += 1

    # Process river file
    if args.riverfile is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "riverdata.bin")
        if process_riverfile(args.riverfile, args.tolerance, output_path):
            success_count += 1

    # Process lake file
    if args.lakefile is not None:
        total_count += 1
        output_path = os.path.join(args.output_dir, "lakedata.bin")
        if process_lakefile(args.lakefile, args.tolerance, output_path):
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
