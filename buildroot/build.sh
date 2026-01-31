#!/bin/bash
#
# viz1090 Buildroot Image Builder
# Targets: Raspberry Pi CM4 with Waveshare CM4-DISP-BASE-2.8A display
#
# Usage: ./build.sh [options]
#   --clean       Clean buildroot build directory
#   --menuconfig  Open buildroot menuconfig
#   --rebuild     Force rebuild of viz1090 package
#   --help        Show this help
#
# NOTE: This script must be run on Linux or WSL2, not Windows directly.
#

set -e

# Check for native Windows (not WSL)
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ -n "$WINDIR" && -z "$WSL_DISTRO_NAME" ]]; then
    echo "ERROR: Buildroot cannot run on Windows directly."
    echo ""
    echo "Please use WSL2 (Windows Subsystem for Linux):"
    echo "  1. Open PowerShell as Administrator"
    echo "  2. Run: wsl --install"
    echo "  3. Reboot and set up Ubuntu"
    echo "  4. In WSL, navigate to this directory and run ./build.sh"
    echo ""
    echo "Your Windows path /c/Users/... is accessible in WSL as /mnt/c/Users/..."
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VIZ1090_ROOT="$(dirname "$SCRIPT_DIR")"
BUILDROOT_VERSION="2024.02.1"
BUILDROOT_DIR="$SCRIPT_DIR/buildroot-$BUILDROOT_VERSION"
BUILDROOT_URL="https://buildroot.org/downloads/buildroot-$BUILDROOT_VERSION.tar.gz"
OUTPUT_DIR="$SCRIPT_DIR/output"
DEFCONFIG="viz1090_cm4_disp28_defconfig"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

show_help() {
    head -15 "$0" | tail -10
    exit 0
}

check_dependencies() {
    info "Checking build dependencies..."

    local missing=()

    for cmd in wget tar make gcc g++ patch cpio unzip rsync bc python3; do
        if ! command -v $cmd &> /dev/null; then
            missing+=($cmd)
        fi
    done

    if [ ${#missing[@]} -ne 0 ]; then
        error "Missing dependencies: ${missing[*]}
Please install them:
  Ubuntu/Debian: sudo apt-get install build-essential wget tar patch cpio unzip rsync bc python3 libncurses-dev
  Fedora: sudo dnf install @development-tools wget tar patch cpio unzip rsync bc python3 ncurses-devel
  Arch: sudo pacman -S base-devel wget tar patch cpio unzip rsync bc python"
    fi

    info "All dependencies found"
}

download_buildroot() {
    if [ -d "$BUILDROOT_DIR" ]; then
        info "Buildroot already downloaded"
        return
    fi

    info "Downloading Buildroot $BUILDROOT_VERSION..."
    wget -q --show-progress -O "$SCRIPT_DIR/buildroot.tar.gz" "$BUILDROOT_URL"

    info "Extracting Buildroot..."
    tar -xzf "$SCRIPT_DIR/buildroot.tar.gz" -C "$SCRIPT_DIR"
    rm "$SCRIPT_DIR/buildroot.tar.gz"

    info "Buildroot extracted to $BUILDROOT_DIR"
}

setup_external_tree() {
    info "Setting up external package tree..."

    # Create external tree structure
    mkdir -p "$SCRIPT_DIR/external/package"
    mkdir -p "$SCRIPT_DIR/external/configs"

    # Link our packages
    ln -sf "$SCRIPT_DIR/package/viz1090" "$SCRIPT_DIR/external/package/viz1090"
    ln -sf "$SCRIPT_DIR/package/dump1090" "$SCRIPT_DIR/external/package/dump1090"

    # Copy defconfig
    cp "$SCRIPT_DIR/configs/$DEFCONFIG" "$SCRIPT_DIR/external/configs/"

    # Create external.desc
    cat > "$SCRIPT_DIR/external/external.desc" << 'EOF'
name: VIZ1090
desc: viz1090 ADS-B visualization system for Raspberry Pi CM4
EOF

    # Create external.mk
    cat > "$SCRIPT_DIR/external/external.mk" << 'EOF'
include $(sort $(wildcard $(BR2_EXTERNAL_VIZ1090_PATH)/package/*/*.mk))
EOF

    # Create Config.in
    cat > "$SCRIPT_DIR/external/Config.in" << 'EOF'
source "$BR2_EXTERNAL_VIZ1090_PATH/package/dump1090/Config.in"
source "$BR2_EXTERNAL_VIZ1090_PATH/package/viz1090/Config.in"
EOF

    info "External tree configured"
}

setup_overlay() {
    info "Setting up root filesystem overlay..."

    local overlay_dir="$SCRIPT_DIR/overlay"

    # Copy viz1090 resources
    mkdir -p "$overlay_dir/opt/viz1090/font"
    mkdir -p "$overlay_dir/opt/viz1090/themes"
    mkdir -p "$overlay_dir/opt/viz1090/mapdata"

    cp -r "$VIZ1090_ROOT/font/"* "$overlay_dir/opt/viz1090/font/" 2>/dev/null || true
    cp -r "$VIZ1090_ROOT/themes/"* "$overlay_dir/opt/viz1090/themes/" 2>/dev/null || true

    # Copy mapdata if it exists
    if [ -d "$VIZ1090_ROOT/mapdata" ] && [ "$(ls -A "$VIZ1090_ROOT/mapdata" 2>/dev/null)" ]; then
        cp -r "$VIZ1090_ROOT/mapdata/"* "$overlay_dir/opt/viz1090/mapdata/" 2>/dev/null || true
    fi

    info "Overlay configured"
}

configure_buildroot() {
    info "Configuring Buildroot..."

    cd "$BUILDROOT_DIR"

    # Use external tree and our defconfig
    make BR2_EXTERNAL="$SCRIPT_DIR/external" O="$OUTPUT_DIR" "${DEFCONFIG}"

    info "Buildroot configured with $DEFCONFIG"
}

build_image() {
    info "Building image (this may take 30-60 minutes on first build)..."

    cd "$BUILDROOT_DIR"

    make BR2_EXTERNAL="$SCRIPT_DIR/external" O="$OUTPUT_DIR" -j$(nproc)

    info "Build complete!"
    info "SD card image: $OUTPUT_DIR/images/sdcard.img"
    echo ""
    info "To write to SD card:"
    echo "  Linux:   sudo dd if=$OUTPUT_DIR/images/sdcard.img of=/dev/sdX bs=4M status=progress"
    echo "  macOS:   sudo dd if=$OUTPUT_DIR/images/sdcard.img of=/dev/rdiskN bs=4m"
    echo "  Windows: Use Raspberry Pi Imager or balenaEtcher"
}

do_clean() {
    info "Cleaning build directory..."
    if [ -d "$OUTPUT_DIR" ]; then
        rm -rf "$OUTPUT_DIR"
        info "Build directory cleaned"
    else
        info "Nothing to clean"
    fi
}

do_menuconfig() {
    if [ ! -d "$BUILDROOT_DIR" ]; then
        download_buildroot
        setup_external_tree
    fi

    cd "$BUILDROOT_DIR"

    if [ ! -f "$OUTPUT_DIR/.config" ]; then
        make BR2_EXTERNAL="$SCRIPT_DIR/external" O="$OUTPUT_DIR" "${DEFCONFIG}"
    fi

    make BR2_EXTERNAL="$SCRIPT_DIR/external" O="$OUTPUT_DIR" menuconfig
}

do_rebuild_viz1090() {
    info "Rebuilding viz1090 package..."
    cd "$BUILDROOT_DIR"
    make BR2_EXTERNAL="$SCRIPT_DIR/external" O="$OUTPUT_DIR" viz1090-dirclean
    make BR2_EXTERNAL="$SCRIPT_DIR/external" O="$OUTPUT_DIR" viz1090-rebuild
    make BR2_EXTERNAL="$SCRIPT_DIR/external" O="$OUTPUT_DIR"
}

# Parse arguments
case "${1:-}" in
    --help|-h)
        show_help
        ;;
    --clean)
        do_clean
        exit 0
        ;;
    --menuconfig)
        do_menuconfig
        exit 0
        ;;
    --rebuild)
        do_rebuild_viz1090
        exit 0
        ;;
esac

# Main build sequence
info "=== viz1090 Buildroot Image Builder ==="
info "Target: Raspberry Pi CM4 + Waveshare 2.8\" DPI Display"
echo ""

check_dependencies
download_buildroot
setup_external_tree
setup_overlay
configure_buildroot
build_image
