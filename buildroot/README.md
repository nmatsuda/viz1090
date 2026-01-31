# viz1090 Buildroot Image

This directory contains everything needed to build a minimal, fast-booting Linux image for the Raspberry Pi CM4 with the Waveshare CM4-DISP-BASE-2.8A display.

## Overview

### What This Does

Instead of running viz1090 on a full Raspberry Pi OS (which takes 30-60 seconds to boot), this creates a custom minimal Linux image that:

- Boots in **~4-5 seconds** from power-on to aircraft display
- Has a **~50MB root filesystem** (vs ~2GB for Raspberry Pi OS)
- **Auto-starts** dump1090 and viz1090 on boot
- Includes only the software needed to run viz1090
- Supports the Waveshare 2.8" DPI capacitive touchscreen

### Boot Sequence

| Phase | Time | What Happens |
|-------|------|--------------|
| Firmware | ~0.5s | Raspberry Pi bootloader loads kernel |
| Kernel | ~2s | Minimal Linux kernel initializes |
| Init | ~1s | BusyBox init starts services |
| Network | ~0.5s | Ethernet DHCP (parallel) |
| dump1090 | ~0.5s | ADS-B decoder starts |
| viz1090 | ~0.5s | Display application starts |
| **Total** | **~4-5s** | Aircraft visible on screen |

### Why Buildroot?

Buildroot creates a custom Linux distribution containing only what you need:

- **Custom kernel**: Only drivers for CM4, display, USB, and networking
- **Minimal userspace**: BusyBox instead of full GNU coreutils
- **No package manager**: Everything is built into the image
- **Cross-compiled**: Build on a fast PC, not on the Pi itself
- **Reproducible**: Same source always produces the same image

---

## Prerequisites

### Build Machine Requirements

You need a **Linux system** to build. Windows users must use WSL2.

| Requirement | Minimum | Recommended |
|-------------|---------|-------------|
| OS | Ubuntu 20.04 / Debian 11 | Ubuntu 22.04+ |
| RAM | 4 GB | 8+ GB |
| Disk Space | 15 GB | 25+ GB |
| CPU | 2 cores | 4+ cores |

### Installing Build Dependencies

#### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    wget \
    git \
    tar \
    patch \
    cpio \
    unzip \
    rsync \
    bc \
    bzip2 \
    xz-utils \
    python3 \
    python3-setuptools \
    perl \
    file \
    libncurses-dev \
    libssl-dev
```

#### Fedora

```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install \
    wget git tar patch cpio unzip rsync bc \
    python3 perl file ncurses-devel openssl-devel
```

#### Arch Linux

```bash
sudo pacman -S \
    base-devel wget git tar patch cpio unzip rsync bc \
    python perl file ncurses openssl
```

### Windows Users: Setting Up WSL2

1. Open PowerShell as Administrator:
   ```powershell
   wsl --install
   ```

2. Reboot when prompted

3. After reboot, Ubuntu will install automatically. Create a username/password.

4. In Ubuntu (WSL), install dependencies:
   ```bash
   sudo apt-get update && sudo apt-get install -y \
       build-essential wget git tar patch cpio unzip rsync bc \
       python3 perl file libncurses-dev libssl-dev
   ```

5. Access your Windows files at `/mnt/c/Users/YourName/...`

---

## Building the Image

### Quick Start

```bash
# Navigate to the buildroot directory
cd /path/to/viz1090/buildroot

# For WSL users, if your repo is in Windows:
cd /mnt/c/Users/YourName/git/viz1090/buildroot

# Make the build script executable
chmod +x build.sh

# Run the build
./build.sh
```

The first build downloads Buildroot and compiles everything from source. This takes **30-60 minutes** depending on your machine. Subsequent builds are much faster due to caching.

### Build Script Options

```bash
./build.sh              # Full build
./build.sh --clean      # Delete build directory and start fresh
./build.sh --menuconfig # Open Buildroot configuration menu
./build.sh --rebuild    # Rebuild only viz1090 package (fast)
./build.sh --help       # Show help
```

### Build Output

After a successful build:

```
buildroot/output/images/
├── sdcard.img          # Complete SD card image (write this to SD card)
├── Image               # Linux kernel
├── bcm2711-rpi-cm4.dtb # Device tree
├── rootfs.ext4         # Root filesystem
└── boot.vfat           # Boot partition
```

---

## Writing to SD Card

### Finding Your SD Card Device

**Linux:**
```bash
# Before inserting SD card
lsblk

# Insert SD card, then run again
lsblk

# The new device is your SD card (e.g., /dev/sdb or /dev/mmcblk0)
```

**macOS:**
```bash
# Before inserting
diskutil list

# After inserting
diskutil list

# Look for the new disk (e.g., /dev/disk2)
```

### Writing the Image

**Linux:**
```bash
# Replace /dev/sdX with your actual device!
sudo dd if=output/images/sdcard.img of=/dev/sdX bs=4M status=progress
sync
```

**macOS:**
```bash
# Unmount first
diskutil unmountDisk /dev/diskN

# Write (use rdisk for faster writes)
sudo dd if=output/images/sdcard.img of=/dev/rdiskN bs=4m
sync
```

**Windows:**
- Use [Raspberry Pi Imager](https://www.raspberrypi.com/software/)
- Or [balenaEtcher](https://etcher.balena.io/)
- Select "Use custom" and choose `sdcard.img`

### Verifying the Write

After writing, you can verify:
```bash
# Linux
sudo dd if=/dev/sdX bs=4M count=100 | md5sum
dd if=output/images/sdcard.img bs=4M count=100 | md5sum
# Should match
```

---

## Hardware Setup

### Target Hardware

| Component | Model |
|-----------|-------|
| Computer | Raspberry Pi Compute Module 4 |
| Carrier Board | Waveshare CM4-DISP-BASE-2.8A |
| Display | 2.8" IPS 480x640 DPI with capacitive touch |
| ADS-B Receiver | Any RTL-SDR USB dongle (e.g., Nooelec NESDR) |

### Connections

```
┌─────────────────────────────────────────┐
│         CM4-DISP-BASE-2.8A              │
│  ┌─────────────────────────────────┐    │
│  │                                 │    │
│  │     2.8" Display (built-in)    │    │
│  │         480 x 640              │    │
│  │                                 │    │
│  └─────────────────────────────────┘    │
│                                         │
│  [USB] ◄── RTL-SDR dongle + antenna     │
│  [ETH] ◄── Ethernet cable (optional)    │
│  [PWR] ◄── 5V power supply              │
│                                         │
└─────────────────────────────────────────┘
```

### Display Specifications

The Waveshare CM4-DISP-BASE-2.8A uses a DPI (parallel RGB) interface:

| Parameter | Value |
|-----------|-------|
| Size | 2.8 inches |
| Resolution | 480 x 640 (portrait) |
| Interface | DPI (not HDMI) |
| Touch | Capacitive (Goodix GT911) |
| Touch Interface | I2C |

---

## Configuration

### viz1090 Settings

After writing the SD card, mount the second partition (ext4) and edit:

```
/opt/viz1090/viz1090.conf
```

Configuration options:

```bash
# Server settings
SERVER=127.0.0.1    # IP of dump1090 server (localhost if running locally)
PORT=30005          # Beast protocol port

# Display settings
FULLSCREEN=1        # 1 = fullscreen, 0 = windowed

# Map center (set to your location for best view)
LAT=37.7749         # Latitude (decimal degrees)
LON=-122.4194       # Longitude (decimal degrees)

# Units
METRIC=0            # 0 = imperial (feet, knots), 1 = metric (meters, km/h)
```

### Using a Remote dump1090 Server

If your ADS-B receiver is on a different machine:

1. Edit `viz1090.conf`:
   ```bash
   SERVER=192.168.1.100    # IP of the machine running dump1090
   ```

2. Disable local dump1090 (optional, saves resources):
   ```bash
   # SSH into the Pi
   ssh root@viz1090

   # Disable dump1090 service
   mv /etc/init.d/S80dump1090 /etc/init.d/disabled.S80dump1090

   # Reboot
   reboot
   ```

### Network Configuration

The system uses DHCP by default. For a static IP, edit `/etc/network/interfaces`:

```bash
auto eth0
iface eth0 inet static
    address 192.168.1.50
    netmask 255.255.255.0
    gateway 192.168.1.1
```

---

## Accessing the System

### SSH Access

SSH is enabled by default:

```bash
ssh root@viz1090
# or by IP
ssh root@192.168.1.x
```

**Default credentials:**
- Username: `root`
- Password: (none - passwordless login)

**Set a password after first login:**
```bash
passwd
```

### Serial Console (for debugging)

Connect a USB-to-serial adapter to the CM4's UART pins:
- GPIO 14 (TXD) → RX on adapter
- GPIO 15 (RXD) → TX on adapter
- GND → GND

```bash
# Linux/macOS
screen /dev/ttyUSB0 115200

# Or use minicom
minicom -D /dev/ttyUSB0 -b 115200
```

---

## Directory Structure

```
buildroot/
├── build.sh                     # Main build script
├── README.md                    # This file
├── .gitignore                   # Excludes build outputs from git
│
├── configs/
│   └── viz1090_cm4_disp28_defconfig  # Buildroot configuration
│
├── package/                     # Custom Buildroot packages
│   ├── dump1090/
│   │   ├── Config.in           # Package menu entry
│   │   └── dump1090.mk         # Build instructions
│   └── viz1090/
│       ├── Config.in
│       └── viz1090.mk
│
├── overlay/                     # Files copied to root filesystem
│   ├── boot/overlays/
│   │   └── goodix.dtbo.dts     # Touchscreen device tree overlay
│   └── etc/
│       ├── init.d/
│       │   ├── S40network      # Start networking
│       │   ├── S80dump1090     # Start ADS-B decoder
│       │   └── S90viz1090      # Start visualization
│       └── inittab             # Init configuration
│
├── patches/                     # Patches for packages (if needed)
│
├── rpi-config.txt              # Raspberry Pi boot configuration
├── kernel-config-fragment      # Linux kernel config additions
├── genimage.cfg                # SD card partition layout
├── post-build.sh               # Runs after packages are built
└── post-image.sh               # Creates the final SD card image
```

---

## Customization

### Adding Map Data

For better visualization, generate map data on your development machine:

```bash
# In the main viz1090 directory
cd /path/to/viz1090

# Install Python dependencies
pip3 install fiona shapely numpy tqdm

# Generate map data
python3 scripts/getmap.py

# Copy to buildroot overlay
cp mapdata.bin mapnames airportdata.bin airportnames \
   buildroot/overlay/opt/viz1090/
```

Then rebuild:
```bash
cd buildroot
./build.sh --rebuild
```

### Changing Themes

Copy theme files to the overlay:
```bash
cp mytheme.json buildroot/overlay/opt/viz1090/themes/
```

### Modifying Kernel Configuration

```bash
./build.sh --menuconfig
# Navigate to: Kernel → Linux Kernel Configuration
# Make changes, save, exit
./build.sh
```

Or edit `kernel-config-fragment` for persistent changes.

### Adding Packages

1. Run menuconfig:
   ```bash
   ./build.sh --menuconfig
   ```

2. Navigate to "Target packages" and enable what you need

3. Rebuild:
   ```bash
   ./build.sh
   ```

---

## Troubleshooting

### Build Fails

**"Missing dependencies"**
```bash
# Install all build dependencies (see Prerequisites section)
sudo apt-get install build-essential wget git ...
```

**"Permission denied"**
```bash
# Make scripts executable
chmod +x build.sh post-build.sh post-image.sh
```

**"No space left on device"**
```bash
# Buildroot needs ~15-25GB. Clean old builds:
./build.sh --clean
```

### Display Not Working

1. **Check boot config**: Verify `/boot/config.txt` on the SD card has the DPI settings

2. **Serial console debug**: Connect serial adapter and watch boot messages

3. **Check DRM status**:
   ```bash
   ssh root@viz1090
   dmesg | grep -i drm
   ls -la /dev/dri/
   ```

4. **Try without viz1090**:
   ```bash
   # Stop viz1090
   /etc/init.d/S90viz1090 stop

   # Test SDL2
   SDL_VIDEODRIVER=kmsdrm /opt/viz1090/viz1090 --fullscreen
   ```

### No Aircraft Data

1. **Check RTL-SDR**:
   ```bash
   lsusb | grep -i rtl
   # Should show: Realtek Semiconductor Corp. RTL2838...
   ```

2. **Check dump1090**:
   ```bash
   /etc/init.d/S80dump1090 status
   cat /var/log/dump1090.log
   ```

3. **Test dump1090 manually**:
   ```bash
   /etc/init.d/S80dump1090 stop
   dump1090 --net --interactive
   # You should see aircraft if your antenna is connected
   ```

4. **Check network connection to dump1090**:
   ```bash
   nc -v 127.0.0.1 30005
   # Should connect. Ctrl+C to exit.
   ```

### viz1090 Crashes

1. **Check logs**:
   ```bash
   cat /var/log/viz1090.log
   ```

2. **Verify fonts exist**:
   ```bash
   ls -la /opt/viz1090/font/
   ```

3. **Run manually with output**:
   ```bash
   /etc/init.d/S90viz1090 stop
   cd /opt/viz1090
   SDL_VIDEODRIVER=kmsdrm ./viz1090 --fullscreen
   ```

### Network Issues

1. **Check link**:
   ```bash
   ip link show eth0
   # Should show "state UP"
   ```

2. **Check IP**:
   ```bash
   ip addr show eth0
   ```

3. **Check DHCP**:
   ```bash
   cat /var/run/dhcpcd.pid
   ps aux | grep dhcp
   ```

4. **Manual DHCP**:
   ```bash
   dhcpcd eth0
   ```

---

## Development Workflow

### Rebuilding Just viz1090

After making changes to viz1090 source code:

```bash
./build.sh --rebuild
```

This is much faster than a full rebuild (~2-5 minutes vs 30-60 minutes).

### Testing Changes Quickly

Instead of reflashing the SD card each time:

1. SSH into the running Pi
2. Stop viz1090: `/etc/init.d/S90viz1090 stop`
3. Copy new binary: `scp viz1090 root@viz1090:/opt/viz1090/`
4. Start viz1090: `/etc/init.d/S90viz1090 start`

### Full Rebuild

If you change kernel options or add packages:

```bash
./build.sh --clean
./build.sh
```

---

## Technical Details

### Included Software

| Package | Version | Purpose |
|---------|---------|---------|
| Linux Kernel | 6.1.x (RPi fork) | Operating system kernel |
| BusyBox | 1.36.x | Core utilities (shell, init, etc.) |
| SDL2 | 2.28.x | Graphics library |
| SDL2_ttf | 2.20.x | Font rendering |
| SDL2_gfx | 1.0.4 | Graphics primitives |
| Mesa | 23.x | OpenGL ES / DRM drivers |
| RTL-SDR | 2.0.x | SDR USB driver library |
| dump1090 | 9.0 | ADS-B decoder |
| viz1090 | (current) | This application |
| OpenSSH | 9.x | Remote access |
| dhcpcd | 10.x | DHCP client |

### Memory Usage

| Component | RAM Usage |
|-----------|-----------|
| Kernel | ~30 MB |
| System services | ~10 MB |
| dump1090 | ~15 MB |
| viz1090 | ~40 MB |
| **Total** | **~95 MB** |

The CM4 has 1-8 GB RAM, so there's plenty of headroom.

### Boot Optimization Details

The fast boot is achieved through:

1. **Kernel**: Disabled unused drivers (WiFi, Bluetooth, sound, cameras)
2. **Init**: BusyBox init instead of systemd
3. **Services**: Only essential services start
4. **Console**: No login prompt on display (viz1090 takes over)
5. **Boot config**: `quiet` kernel parameter suppresses boot messages

---

## License

This buildroot configuration is part of viz1090 and is released under the MIT License.

The software it builds (Linux kernel, BusyBox, SDL2, etc.) retains its original licenses.
