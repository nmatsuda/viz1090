#!/bin/bash
#
# viz1090 Map Data Downloader
#
# Downloads Natural Earth shapefiles and converts them to viz1090 format.
# Handles download failures gracefully - missing data sources are skipped.
#
# Requirements:
#   - wget
#   - Python 3 with: fiona, shapely, numpy, tqdm
#
# Usage:
#   ./getmap.sh [output_dir]
#
# The output_dir defaults to the current directory.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${1:-.}"
MAPDATA_DIR="${OUTPUT_DIR}/mapdata"

# GitHub raw URL base for Natural Earth vector data
GITHUB_BASE="https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/10m_cultural"

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

# Download a complete shapefile (all required extensions)
download_shapefile() {
    local base_url="$1"
    local base_name="$2"
    local dest_dir="$3"
    local success=true

    echo "Downloading shapefile: ${base_name}"

    # Required shapefile components
    for ext in shp shx dbf prj; do
        if ! download_file "${base_url}/${base_name}.${ext}" "${dest_dir}/${base_name}.${ext}"; then
            success=false
        fi
    done

    # Optional components (don't fail if missing)
    download_file "${base_url}/${base_name}.cpg" "${dest_dir}/${base_name}.cpg" || true

    if $success; then
        echo "  Shapefile complete: ${base_name}"
        return 0
    else
        echo "  Warning: Shapefile incomplete: ${base_name}"
        return 1
    fi
}

echo "Downloading Natural Earth data from GitHub..."
echo ""

# Download shapefiles from GitHub
download_shapefile "${GITHUB_BASE}" "ne_10m_admin_1_states_provinces" "${MAPDATA_DIR}" || true
echo ""

download_shapefile "${GITHUB_BASE}" "ne_10m_populated_places" "${MAPDATA_DIR}" || true
echo ""

download_shapefile "${GITHUB_BASE}" "ne_10m_airports" "${MAPDATA_DIR}" || true
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
