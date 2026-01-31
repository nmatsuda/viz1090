#!/bin/bash
#
# Post-build script for viz1090 Buildroot image
# This runs after packages are installed but before the filesystem image is created
#

set -e

TARGET_DIR="$1"
SCRIPT_DIR="$(dirname "$0")"
BR2_EXTERNAL="$(cd "$SCRIPT_DIR" && pwd)"

echo "=== viz1090 post-build script ==="

# Create necessary directories
mkdir -p "$TARGET_DIR/opt/viz1090"
mkdir -p "$TARGET_DIR/boot/overlays"

# Compile device tree overlay for Goodix touchscreen if DTS exists
if [ -f "$BR2_EXTERNAL/overlay/boot/overlays/goodix.dtbo.dts" ]; then
    echo "Compiling Goodix touchscreen overlay..."
    if command -v dtc &> /dev/null; then
        dtc -@ -I dts -O dtb \
            -o "$TARGET_DIR/boot/overlays/goodix.dtbo" \
            "$BR2_EXTERNAL/overlay/boot/overlays/goodix.dtbo.dts" 2>/dev/null || \
            echo "Warning: Could not compile goodix.dtbo (dtc may not support overlays)"
    fi
fi

# Set permissions on init scripts
chmod +x "$TARGET_DIR/etc/init.d/"S* 2>/dev/null || true

# Create udev rules for RTL-SDR
mkdir -p "$TARGET_DIR/etc/udev/rules.d"
cat > "$TARGET_DIR/etc/udev/rules.d/20-rtlsdr.rules" << 'EOF'
# RTL-SDR USB device rules
SUBSYSTEM=="usb", ATTRS{idVendor}=="0bda", ATTRS{idProduct}=="2832", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0bda", ATTRS{idProduct}=="2838", MODE="0666"
EOF

# Configure network interface
mkdir -p "$TARGET_DIR/etc/network"
cat > "$TARGET_DIR/etc/network/interfaces" << 'EOF'
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
EOF

# Configure hostname resolution
echo "viz1090" > "$TARGET_DIR/etc/hostname"
cat > "$TARGET_DIR/etc/hosts" << 'EOF'
127.0.0.1	localhost
127.0.1.1	viz1090
::1		localhost ip6-localhost ip6-loopback
EOF

# Create cmdline.txt for fast boot
cat > "$TARGET_DIR/../images/rpi-firmware/cmdline.txt" 2>/dev/null << 'EOF' || true
root=/dev/mmcblk0p2 rootfstype=ext4 rootwait console=tty1 quiet loglevel=0 logo.nologo vt.global_cursor_default=0 plymouth.ignore-serial-consoles
EOF

# Create viz1090 configuration file
cat > "$TARGET_DIR/opt/viz1090/viz1090.conf" << 'EOF'
# viz1090 configuration
# These are passed as command line arguments

# Server settings (localhost for dump1090 running locally)
SERVER=127.0.0.1
PORT=30005

# Display settings
FULLSCREEN=1

# Map center (set to your location)
# LAT=37.7749
# LON=-122.4194
# METRIC=0
EOF

echo "=== Post-build complete ==="
