# viz1090

A Go-based visualizer for ADS-B aircraft data with real-time interactive display.

![viz1090 screenshot](screenshot.png)

## Features

- Real-time display of aircraft positions, altitude, speed, and other data
- Geographic map with coastlines, borders, and airports
- Interactive interface with zoom, pan, and aircraft selection
- Optimized performance for Raspberry Pi 4
- Cross-platform support (macOS for development, Raspberry Pi for deployment)

## Installation

### Prerequisites

#### On macOS (for development)

```bash
# Install dependencies
brew install sdl2 sdl2_ttf sdl2_gfx go
pip3 install fiona shapely tqdm
```

### Credits

viz1090 is largely based on [dump1090](https://github.com/MalcolmRobb/dump1090) (Malcom Robb, Salvatore Sanfilippo)
