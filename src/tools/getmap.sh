#!/bin/bash
#
# viz1090 Map Data Downloader
#
# Downloads Natural Earth shapefiles and converts them to viz1090 format.
# Handles download failures gracefully - missing data sources are skipped.
#
# Requirements:
#   - wget, unzip
#   - Python 3 with: fiona, shapely, numpy, tqdm
#
# Usage:
#   ./getmap.sh [output_dir]
#
# The output_dir defaults to the current directory.
#

set -e

#=============================================================================
# DATA SOURCE URLS
# Edit these URLs to change data sources
#=============================================================================

# Map geometry (state/province boundaries)
# Source: Natural Earth 10m Admin 1 States Provinces
URL_MAP="https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/10m_cultural/ne_10m_admin_1_states_provinces"

# Place names (cities, towns)
# Source: Natural Earth 10m Populated Places
URL_PLACES="https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/10m_cultural/ne_10m_populated_places"

# Airport names (IATA codes)
# Source: Natural Earth 10m Airports
URL_AIRPORTS="https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/10m_cultural/ne_10m_airports"

# Airport runway geometry
# Source: FAA Aeronautical Data Delivery Service via ArcGIS Hub
URL_RUNWAYS="https://hub.arcgis.com/api/v3/datasets/4d8fa46181aa470d809776c57a8ab1f6_0/downloads/data?format=shp&spatialRefId=4269&where=1%3D1"

#=============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${1:-.}"
MAPDATA_DIR="${OUTPUT_DIR}/mapdata"

echo "=== viz1090 Map Data Generator ==="
echo "Output directory: ${OUTPUT_DIR}"
echo ""

# Create directories
mkdir -p "${MAPDATA_DIR}"

# Download function with error handling
download_file() {
    local url="$1"
    local output="$2"

    if [[ -f "${output}" ]]; then
        echo "  Already exists: ${output}"
        return 0
    fi

    echo "  Downloading: ${url}"
    if wget --no-verbose -O "${output}" "${url}" 2>/dev/null; then
        echo "  Success: ${output}"
        return 0
    else
        echo "  Warning: Failed to download ${url}"
        rm -f "${output}"
        return 1
    fi
}

# Download a complete shapefile (all required extensions) from individual files
download_shapefile() {
    local base_url="$1"
    local base_name="$2"
    local dest_dir="$3"
    local success=true

    echo "Downloading shapefile: ${base_name}"

    # Required shapefile components
    for ext in shp shx dbf prj; do
        if ! download_file "${base_url}.${ext}" "${dest_dir}/${base_name}.${ext}"; then
            success=false
        fi
    done

    # Optional components (don't fail if missing)
    download_file "${base_url}.cpg" "${dest_dir}/${base_name}.cpg" || true

    if $success; then
        echo "  Shapefile complete: ${base_name}"
        return 0
    else
        echo "  Warning: Shapefile incomplete: ${base_name}"
        return 1
    fi
}

# Download and extract a shapefile from a ZIP archive
download_shapefile_zip() {
    local url="$1"
    local dest_dir="$2"
    local zip_name="$3"
    local zip_file="${dest_dir}/${zip_name}.zip"

    echo "Downloading shapefile archive: ${zip_name}"

    # Check if we already have extracted files
    if [[ -f "${dest_dir}/${zip_name}.shp" ]]; then
        echo "  Already exists: ${dest_dir}/${zip_name}.shp"
        return 0
    fi

    # Download the ZIP
    if ! download_file "${url}" "${zip_file}"; then
        return 1
    fi

    # Extract
    echo "  Extracting: ${zip_file}"
    if unzip -o -q "${zip_file}" -d "${dest_dir}"; then
        echo "  Extracted successfully"
        # Clean up ZIP file
        rm -f "${zip_file}"
        return 0
    else
        echo "  Warning: Failed to extract ${zip_file}"
        rm -f "${zip_file}"
        return 1
    fi
}

echo "Downloading map data..."
echo ""

# Download shapefiles from Natural Earth (GitHub)
download_shapefile "${URL_MAP}" "ne_10m_admin_1_states_provinces" "${MAPDATA_DIR}" || true
echo ""

download_shapefile "${URL_PLACES}" "ne_10m_populated_places" "${MAPDATA_DIR}" || true
echo ""

download_shapefile "${URL_AIRPORTS}" "ne_10m_airports" "${MAPDATA_DIR}" || true
echo ""

# Download runway data from FAA/ArcGIS (ZIP archive)
download_shapefile_zip "${URL_RUNWAYS}" "${MAPDATA_DIR}" "Runways" || true
echo ""

echo "Converting to viz1090 format..."

# Build arguments based on what files exist
CONVERTER_ARGS="--output-dir ${OUTPUT_DIR}"

if [[ -f "${MAPDATA_DIR}/ne_10m_admin_1_states_provinces.shp" ]]; then
    CONVERTER_ARGS="${CONVERTER_ARGS} --mapfile ${MAPDATA_DIR}/ne_10m_admin_1_states_provinces.shp"
fi

if [[ -f "${MAPDATA_DIR}/ne_10m_populated_places.shp" ]]; then
    CONVERTER_ARGS="${CONVERTER_ARGS} --mapnames ${MAPDATA_DIR}/ne_10m_populated_places.shp"
fi

if [[ -f "${MAPDATA_DIR}/Runways.shp" ]]; then
    CONVERTER_ARGS="${CONVERTER_ARGS} --airportfile ${MAPDATA_DIR}/Runways.shp"
fi

if [[ -f "${MAPDATA_DIR}/ne_10m_airports.shp" ]]; then
    CONVERTER_ARGS="${CONVERTER_ARGS} --airportnames ${MAPDATA_DIR}/ne_10m_airports.shp"
fi

# Run the converter
python3 "${SCRIPT_DIR}/mapconverter.py" ${CONVERTER_ARGS}

echo ""
echo "=== Map data generation complete ==="
echo "Generated files in: ${OUTPUT_DIR}"
ls -la "${OUTPUT_DIR}"/*.bin "${OUTPUT_DIR}"/mapnames "${OUTPUT_DIR}"/airportnames 2>/dev/null || true
