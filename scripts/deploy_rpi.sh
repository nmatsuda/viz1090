#!/bin/bash
# scripts/deploy_rpi.sh

# Configuration
RPI_HOST=${RPI_HOST:-"raspberrypi.local"}
RPI_USER=${RPI_USER:-"pi"}
RPI_DIR=${RPI_DIR:-"~/viz1090"}

# Check if cross-compiled binary exists
if [ ! -f "viz1090_rpi" ]; then
    echo "Error: viz1090_rpi binary not found. Run 'make rpi' first."
    exit 1
fi

# Ensure target directory exists
ssh ${RPI_USER}@${RPI_HOST} "mkdir -p ${RPI_DIR}"

# Copy binary and assets
echo "Copying files to ${RPI_USER}@${RPI_HOST}:${RPI_DIR}..."
scp viz1090_rpi ${RPI_USER}@${RPI_HOST}:${RPI_DIR}/viz1090
scp -r assets ${RPI_USER}@${RPI_HOST}:${RPI_DIR}/
scp -r scripts/getmap.sh scripts/mapconverter.py ${RPI_USER}@${RPI_HOST}:${RPI_DIR}/scripts/

# Check if map data exists, if not generate it
if ssh ${RPI_USER}@${RPI_HOST} "[ ! -f ${RPI_DIR}/mapdata.bin ]"; then
    echo "Map data not found on Raspberry Pi. Generating..."
    ssh ${RPI_USER}@${RPI_HOST} "cd ${RPI_DIR} && ./scripts/getmap.sh"
fi

echo "Deployment complete. Connect to Raspberry Pi and run viz1090."