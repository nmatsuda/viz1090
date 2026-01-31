#!/bin/bash
#
# Post-image script for viz1090 Buildroot image
# Creates the bootable SD card image
#

set -e

BOARD_DIR="$(dirname $0)"
GENIMAGE_CFG="${BOARD_DIR}/genimage.cfg"
GENIMAGE_TMP="${BUILD_DIR}/genimage.tmp"

echo "=== viz1090 post-image script ==="

# Copy firmware files to binaries dir root for genimage
cp "${BINARIES_DIR}/rpi-firmware/fixup4.dat" "${BINARIES_DIR}/" 2>/dev/null || true
cp "${BINARIES_DIR}/rpi-firmware/start4.elf" "${BINARIES_DIR}/" 2>/dev/null || true

# Create cmdline.txt
cat > "${BINARIES_DIR}/cmdline.txt" << 'EOF'
root=/dev/mmcblk0p2 rootfstype=ext4 rootwait console=tty1 quiet loglevel=0 logo.nologo vt.global_cursor_default=0
EOF

# Copy config.txt
if [ -f "${BINARIES_DIR}/rpi-firmware/config.txt" ]; then
    cp "${BINARIES_DIR}/rpi-firmware/config.txt" "${BINARIES_DIR}/"
fi

# Ensure overlays directory exists in binaries
mkdir -p "${BINARIES_DIR}/overlays"

# Copy DTB overlays from rpi-firmware
if [ -d "${BINARIES_DIR}/rpi-firmware/overlays" ]; then
    cp -r "${BINARIES_DIR}/rpi-firmware/overlays/"* "${BINARIES_DIR}/overlays/" 2>/dev/null || true
fi

# Copy our custom overlays if they exist
if [ -d "${TARGET_DIR}/boot/overlays" ]; then
    cp -r "${TARGET_DIR}/boot/overlays/"*.dtbo "${BINARIES_DIR}/overlays/" 2>/dev/null || true
fi

echo "Creating SD card image..."

rm -rf "${GENIMAGE_TMP}"

genimage \
    --rootpath "${TARGET_DIR}" \
    --tmppath "${GENIMAGE_TMP}" \
    --inputpath "${BINARIES_DIR}" \
    --outputpath "${BINARIES_DIR}" \
    --config "${GENIMAGE_CFG}"

echo "=== SD card image created: ${BINARIES_DIR}/sdcard.img ==="
exit 0
