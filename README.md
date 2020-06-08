# viz1090


### BUILDING

Tested and working on Ubuntu 18.04, Raspbian Stretch / Buster, Windows Subsystem for Linux (with Ubuntu 18.04)

0. Install build essentials

```
sudo apt-get install build-essentials
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
Until more comprehensive map source (e.g., Mapbox) is integrated, map1090 uses the lat/lon SVG files from https://www.mccurley.org

The getmap.sh pulls the svg file for the contiguous 48 US states and produces a binary file for map1090 to read.

```
sudo apt install python3 python3-pip
pip3 install lxml numpy
./getmap.sh
```

3. (optional for Windows)

As WSL does not have an X server built in, you will need to install a 3rd party X server, such as https://sourceforge.net/projects/vcxsrv/

When running vcxsrv Xlaunch, make sure to **uncheck "Use Native openGL"**

### RUNNING

1. Start dump1090 (http://www.github.com/MalcolmRobb/dump1090) locally in network mode:
```
dump1090 --net
```

2. Run map1090 
```
./view1090 --fullsceen --lat [your latitude] --lon [your longitude]
```

map1090 will open an SDL window set to the resolution of your screen.

### RUNTIME OPTIONS

--server [domain name or ip]	Specify a dump1090 server. Renamed from the view1090 "--net-bo-ip-addr" argument
--port [port number]			Specify dump1090 server port. Renamed from the view1090 "--net-bo-port" argument
--metric						Display metric units rather than imperial.

--lat                           Specify your latitude in degrees
--lon                           Specify your longitiude in degrees

--screensize [width] [height]	Specify a specific resolution to pass to SDL_RenderSetLogicalSize, otherwise use resolution of display
--uiscale [scale]				Scale up UI elements by integer amounts for high resolution screen
--fullscreen					Render fullscreen rather than in a window

### HARDWARE NOTES

map1090 is designed to be portable and work on a variety of systems, however it is intended to be used on a handheld device. 

The software was originally develped for Raspberry Pi devices, and it is currently optimized for the Raspberry Pi 4 with the following configuration:

* Raspberry Pi 4
* A display:
	* [Pimoroni HyperPixel 4.0 Display] (https://shop.pimoroni.com/products/hyperpixel-4) \*best overall, but requires some rework to use battery monitoring features of the PiJuice mentioned below
	* [Waveshare 5.5" AMOLED] (https://www.waveshare.com/5.5inch-hdmi-amoled.htm) \*this is very good screen but the Google Pixel 2 phone mentioned below has a very similar display for the same price (along with everything else you need in a nice package)
	* [Waveshare 4.3" HDMI(B)] (https://www.waveshare.com/wiki/4.3inch_HDMI_LCD_(B))
	* [Adafruit 2.8" Capacitive Touch] (https://www.adafruit.com/product/2423)
* A battery hat, such as:
	* [PiJuice Battery Hat] (https://uk.pi-supply.com/products/pijuice-standard) \*I2C pins must be reworked to connect to the Hyperpixel nonstandard I2C breakout pins, unfortunately
	* [MakerFocus UPS Hat] (https://www.amazon.com/Makerfocus-Raspberry-2500mAh-Lithium-Battery/dp/B01MQYX4UX) 
* Any USB SDR receiver:
	* [Noelec Nano V3] (https://www.nooelec.com/store/nesdr-nano-three.html)
	* Stratux V2 \*very low power but hard to find


If running as a front end only, with a separate dump1090 server, the best option is to use an Android phone, such as the Pixel 2, which significantly outperforms a Raspberry Pi 4.

map1090 has been tested on other boards such as the UP Core and UP Squared, but these boards have significantly poorer performance than the Raspberry Pi 4 with less software and peripheral support, so they are not recommended. With low resolution maps the software will run on these boards or even a Raspberry Pi Zero, so these remain options with some tradeoffs.

Of course, a variety of other devices work well for this purpose - all of the development so far has been done on a touchscreen Dell XPS laptop.

