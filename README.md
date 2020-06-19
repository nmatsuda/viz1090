# viz1090

**This is work in progress**


There are some major fixes and cleanup that need to happen before a relase:
* Everything is a grab bag of C and C++, need to more consistently modernize
* A full refactor, especially View.cpp, necessary for many of the new features below.
* A working Android build, as this is the best way to run this on portable hardware.

There are also a lot of missing features:
* Map improvements
	* Labels, different colors/line weights for features
	* Tile prerenderer for improved performance
* In-application menus for view options and configuration
* Theming/colormaps (important as this is primarily intended to be eye candy!)
* Integration with handheld features like GPS, battery monitors, buttons/dials, etc. 

### BUILDING

Tested and working on Ubuntu 18.04, Raspbian Stretch / Buster, Windows Subsystem for Linux (with Ubuntu 18.04), and Mac

0. Install build essentials

```
sudo apt-get install build-essential
```

1. Install SDL and RTL-SDR libararies
```
sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-gfx-dev librtlsdr-dev
```

Note: On Raspbian the SDL2 package requires X to be running. See the Raspberry Pi section for notes on running from the terminal and other improvements.

2. Download and build spidr
```
cd ~
git clone https://www.github.com/nmatsuda/spidr
cd spidr
make clean; make
```

3. Download and process map data
Grab a shapefile with your desired level of detail from https://www.naturalearthdata.com/downloads

[This](https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/10m/cultural/ne_10m_admin_1_states_provinces.zip) is a good place to start.

Unzip and copy the .shp and .shx files. 

```
sudo apt install python3 python3-pip
pip3 install geopandas tqdm
python3 mapconverter.py ne_10m_admin_1_states_provinces.shp
```

This will produce a file mapdata.bin that viz1090 reads. If the file doesn't exist then visualizer will show planes and trails without any geography.

The default parameters for mapconverter should render resonably quickly on a Raspberri Pi 4. See the mapconverter section below for other options.


3. (Windows only)

As WSL does not have an X server built in, you will need to install a 3rd party X server, such as https://sourceforge.net/projects/vcxsrv/

	* run Xlaunch from the start menu
	* Uncheck "Use Native openGL"
	* Open the Ubuntu WSL terminal
	* Specify the X display to use
	```
	export DISPLAY=:0
	```
	* Start viz1090 as described below.

### RUNNING

1. Start dump1090 (http://www.github.com/MalcolmRobb/dump1090) locally in network mode:
```
dump1090 --net
```

2. Run viz1090 
```
./viz1090 --fullsceen --lat [your latitude] --lon [your longitude]
```

viz1090 will open an SDL window set to the resolution of your screen.

### RUNTIME OPTIONS

| Argument						| Description |
| ----------------------------- | ----------- |
| --server [domain name or ip]	| Specify a dump1090 server | 
| --port [port number]			| Specify dump1090 server port | 
| --metric						| Display metric units | 
| --lat                         | Specify your latitude in degrees | 
| --lon                         | Specify your longitiude in degrees | 
| --screensize [width] [height]	| Specify a resolution, otherwise use resolution of display | 
| --uiscale [scale]				| Scale up UI elements by integer amounts for high resolution screen | 
| --fullscreen					| Render fullscreen rather than in a window | 

### HARDWARE NOTES

This software was originally intended for Raspberry Pi devices, and it is currently optimized for the Raspberry Pi 4 with the following configuration:

* Raspberry Pi 4
* A display:
	* [Pimoroni HyperPixel 4.0 Display](https://shop.pimoroni.com/products/hyperpixel-4) \*best overall, but requires some rework to use battery monitoring features of the PiJuice mentioned below
	* [Waveshare 5.5" AMOLED](https://www.waveshare.com/5.5inch-hdmi-amoled.htm) \*this is very good screen but the Google Pixel 2 phone mentioned below has a very similar display for the same price (along with everything else you need in a nice package)
	* [Waveshare 4.3" HDMI(B)](https://www.waveshare.com/wiki/4.3inch_HDMI_LCD_(B))
	* [Adafruit 2.8" Capacitive Touch](https://www.adafruit.com/product/2423)
* A battery hat, such as:
	* [PiJuice Battery Hat](https://uk.pi-supply.com/products/pijuice-standard) \*I2C pins must be reworked to connect to the Hyperpixel nonstandard I2C breakout pins, unfortunately
	* [MakerFocus UPS Hat](https://www.amazon.com/Makerfocus-Raspberry-2500mAh-Lithium-Battery/dp/B01MQYX4UX) 
* Any USB SDR receiver:
	* [Noelec Nano V3](https://www.nooelec.com/store/nesdr-nano-three.html)
	* Stratux V2 \*very low power but hard to find


If running as a front end only, with a separate dump1090 server, the best option is to use an Android phone, such as the Pixel 2, which significantly outperforms a Raspberry Pi 4.

viz1090 has been tested on other boards such as the UP Core and UP Squared, but these boards have poor performance compared to a Raspberry Pi 4, along with worse software and peripheral support, so they are not recommended. viz1090 with a low resolution map will run on these boards or even a Raspberry Pi Zero, so these remain options with some tradeoffs.

Of course, a variety of other devices work well for this purpose - all of the development so far has been done on a touchscreen Dell XPS laptop.

### Credits

viz1090 is largely based on [dump1090](https://github.com/MalcolmRobb/dump1090) (Malcom Robb, Salvatore Sanfilippo)
