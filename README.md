# viz1090

**This is work in progress**
There are a lot of missing pieces in this implementation so far:
* A proper map system yet. Eventually map data should be pulled from Mapbox or similar.
* In-application menus or configuration yet.
* Theming/colormaps (important as this is primarily intended to be eye candy!)
* Integration with handheld features like GPS, battery monitors, buttons/dials, etc. 
* Android build is currently broken

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
Until more comprehensive map source (e.g., Mapbox) is integrated, viz1090 uses the lat/lon SVG files from https://www.mccurley.org

The getmap.sh pulls the svg file for the contiguous 48 US states and produces a binary file for viz1090 to read.

```
sudo apt install python3 python3-pip
pip3 install lxml numpy tqdm
./getmap.sh
```

The mapconverter script called by getmap.sh downsamples the file to render resonably quickly on a Raspberri Pi 4. If you are on a slower device (e.g, a Raspberry Pi 3), you may want to try something like:

```
python3 mapconverter.py --resolution 64 all.svg
```

On the other hand, if you are on a modern desktop or laptop, you can use something higher (but you probably don't need the full 6 digit precision of the McCurley SVG file):


```
python3 mapconverter.py --resolution 8192 all.svg
```


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
