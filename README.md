#spidr1090

### TODO
* change plane history from fixed array to linked list of planeObj, handle cleanup
* status box layout system
* status box tap for info
	* trails w/ altitudes
* menu system (toggles for UI elements)
* map system (automatically load open source)
* separate fully from view1090 (which interface?)


###HARDWARE

Notes:
Pi 3b+ and below are a little underpowerd for unoptimized map drawing
Pi 4 is smooth, may have USB issues with RTLSDR?
Up squared is good, but large
Up Core may be fine but bad connection options - try up core?

PiJuice is by far best battery option. FCC/CE certified!

Need to test stratux low power dongles. 987Mhz?

Waveshare 4.3" HDMI(B) is very good, slightly too large for handheld. Uses a lot of power, around 500ma
Pimoroni Hyperpixel 4.0 is the right size, but takes over default I2C pins so conflicts with PiJuice, unless some pin remapping. Also lower power, around 150ma
Adafruit PiTFT 2.8" capacitive touch is ok, but a little small. Not sure about power draw. Docs claim no multitouch?


####Pi Zero Version
Part | Link | Cost
--- | --- | ---
Raspberry Pi Zero W | | $10
Adafruit 2.4" PiTFT Hat| (https://www.adafruit.com/product/2455) | $35
NooElec Nano3 SDR | | $28
Adafruit GPS Hat | (https://www.adafruit.com/product/2324) | $45
Antenna | |
USB Jack | |
MicroSD Card | |
| | Total: $118+

####Battery Options
(https://en.wikipedia.org/wiki/List_of_battery_sizes)
Portrait Orientation:
18500 Batteries (18mm x 50mm), ~1000-2000mAh ea.
The Pi Zero configuration consumes are 500mAh

Landscape Orientation
18650 batteries (18mm x 65mm). ~2200-3500mAh ea. 
Adafruit pack + PowerBoost Charger, ~$40
Anker PowerCore 10000, ~$25

Other options
external recharger (maybe cheaper and smaller?) along with 18500/18650

Some 18500 / 18650 include current protection inside, such as:
http://www.ebay.com/itm/2-x-Panasonic-NCR-18500-2000mAh-LI-ION-RECHARGEABLE-BATTERY-PCB-button-top-case-/391378348412?epid=1830264923&hash=item5b1ff7a97c:g:ePgAAOSwFqJWswnj,
otherwise would require external protection circuit to be used in parallel. 



####Pi 3 Version
This gets you a slightly bigger screen, and slightly better performance using the SDR Smart rather than the Nano 3, but requires a ton of annoying port/header clipping and desoldering. 

Part | Link | Cost
--- | --- | ---
Raspberry Pi 3 | | $35
Adafruit 2.8" PiTFT Hat| https://www.adafruit.com/product/2423 | $45
NooElec SDR Smart | | $20
GlobalSat MicroGPS Dongle | (https://www.amazon.com/GlobalSat-ND-105C-Micro-USB-Receiver/dp/B00N32HKIW/ref=sr_1_3?ie=UTF8&qid=1505829420&sr=8-3&keywords=gps+dongle) | $30
Antenna | |
USB Jack | |
MicroSD Card | |
| | Total: $130+


alt screen https://www.amazon.com/3-5inch-RPi-LCD-Directly-pluggable-Displaying/dp/B01N48NOXI/ref=sr_1_26?ie=UTF8&qid=1505871836&sr=8-26&keywords=pi+3.5%22+tft, $30 instead of 45, larger, not cap touch

####Battery Options

Recommended: PiJuice


18650 batteries (18mm x 65mm). ~2200mAH ea. 
Adafruit pack + PowerBoost Charger, ~$40
http://www.ebay.com/itm/3-7-volts-6400-mAh-1S2P-18650-Li-Ion-Battery-Pack-PCB-protected-Panasonic-Cells-/221923032745?hash=item33aba4bea9:g:0-IAAOSw14xWLSr2

###INSTALLATION

Tested and working on Ubuntu 18.04, Raspbian Stretch

1. Install SDL and RTL-SDR libararies
```
sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-gfx-dev librtlsdr-dev
```
2. Download and build spidr
```
cd ~
git clone https://www.github.com/nmatsuda/spidr
cd spidr
make clean; make
```

3. Download and build dump1090 
```
cd ~
git clone http://www.github.com/MalcolmRobb/dump1090)
cd dump1090
make clean; make

4. Run
```
~/dump1090/dump1090
cd spidr
./view1090 --screensize 240 400 --fullsceen

### Runtime Options
