# SmartWatering 
Particle Core based project in charge of taking care of my green plants 

This readme is subject to evolve.

Basically, I am using 5 parts and a few meters of water hose
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/IMG_0279.JPG "Essential parts")
- [A I2C soil moisture sensor] (https://www.tindie.com/products/miceuz/i2c-soil-moisture-sensor/)
- [A Particle Core](https://www.particle.io/)
- [A water flow meter] (http://www.dx.com/p/hs01-high-precision-flow-meter-white-black-226937#.Vo2cj1JN8Vc)
- [An USB submersible water pump] (http://www.dx.com/p/at-usb-1020-usb-powered-pet-fish-tank-submersible-pump-black-dc-3-5-9v-337664#.Vo2ckFJN8Vc)
- An USB power switch (Mosfet, Relay, Integrated switch IC, ...)

The program (at the time of writing, version is 1.0.0 ) does some kind of trivial task:
- 1) Initialize the hardware
- 2) Wait some time 
- 3) Proceed to a moisture measurement
- 4) if moisture under threshold, goto 2), else goto 5)
- 5) Turn the pump ON, wait until the desired quantity of water has been watered AND check that timeout did not happen
- 6) Turn the pump OFF, goto 2)

Modifying the Particle 
---------------------
The water pump will not work with a voltage under 5V. In order to power it, I soldered a wire straight on the VUSB pin of the USB header. 
The following picture illustrates where to solder the wire on the former Arduino Nano. 
You can do the same with the Particle Core diode (black component between the USB header and the VIN pin).

![Alt text](https://github.com/yerpj/SmartWatering/blob/master/IMG_0281.JPG "A real 5V wire is used to power the water pump")

Connecting everything 
---------------------
Actually the wiring is straightforward. 
Remember that on the Arduinos as well as on the Particle Core, you can not use any pin in interrupt mode.
Here, I used D3 to count the pulses coming out the water flow meter.
You will also notice the 2 10k pull-up resistors on the I2C bus. They are mandatory, do not forget them.
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/wiring.jpg "wiring")

When the electric wiring is done, you can now assemble the parts along the water hose (5mm think, respective to the diameter of the Water pump and flow sensor).
(Note, you can pick up some hose on Amazon like [this one](http://www.amazon.co.uk/PVC-Plastic-Pipe-Aquarium-Quality/dp/B00QKQ92ZW)) .

![Alt text](https://github.com/yerpj/SmartWatering/blob/master/Hose.jpg "Hose and Moisture sensor")

Command Line Interface (CLI)
----------------------------

Since revision 0.3.0, the CLI is implemented. It allows to interract with the system through the USB Serial port.
Since revision 0.4.0, the luminosity sensor is supported
Since revision 0.5.0, the entire project has been ported to a [Particle Core](https://www.particle.io/)

The CLI is based on two main mechanisms:
- synchronous request-response
- asynchronous notification

Responses coming from the Particle Coree are JSON objects passed as strings.

Synchronous request-response CMD list:

The requests are of the form "sw"+<CMD>.
- "sw info"
- "sw temp"
- "sw moisture"
- "sw start"
- "sw lumi"


Asynchronous notification:

As for the synchronous mechanism, the answers are JSON-formatted. There are a few fields, helping the JSON parsing when interracting with multiple objects:
"device":		"sw" (means Smart Watering)
"type":			"data" (this is currently the only notification type)
"dataPoint": 	the effective datapoint included in the notification
"dataValue":	the value of "dataPoint"

example: 
```{"device":"sw","type":"data","dataPoint":"moisture","dataValue":"365"}```
```{"device":"sw","type":"data","dataPoint":"pump","dataValue":"ON"}```
```{"device":"sw","type":"data","dataPoint":"systemError","dataValue":"Pump problem"}```

The Particle Core proceed to a moisture measurement every WATERING_TIMEOUT [ms], sends the measured value to the serial port, then proceed to a watering, if needed.

You can see in the following picture what the CLI looks like:
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/CLI.JPG "Command Line Interface")


Web interraction
----------------
A specific "CloudRequest" ressource allows a user to interract remotely with the device.
By POSTing to https://api.particle.io/v1/devices/[DEVICE_ID]/CloudRequest , user can basically interract the same way as for the CLI, with a specific request parameter:
args={COMMAND}, COMMAND being one of the strings below
- "temp" : 		retrieve the soil temperature
- "lumi" : 		retrieve the ambiant luminosity
- "moisture": 	retrieve the soil moisture
- "start":		start a watering cycle


The Prototype
-------------

In order to prevent any damage or flood, I made a prototype around a jug of water. The USB water pump rests at the water bottom. The end of the hose directly flows into the jug.
As the moisture sensor is not put into water, it will sense something like a "dry earth", and therefore the system will trigger a watering cycle every WATERING_TIMEOUT [ms]. 
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/Prototype.JPG "Prototype")

The First Smart Watering Can
----------------------------

From code version 1.0.0, every desired feature is implemented. It's now time to enclose every wire into my brand new watering can.


![Alt text](https://github.com/yerpj/SmartWatering/blob/master/Integration.JPG "The electronics is sitting in the showerhead")

![Alt text](https://github.com/yerpj/SmartWatering/blob/master/SmartWateringCan.JPG "Final packaging")

![Alt text](https://github.com/yerpj/SmartWatering/blob/master/FinalSetup.JPG "The watering can is destinated to live next to the plant.")


The Code 
--------
[HERE](https://github.com/yerpj/SmartWatering/tree/master/smartWatering_1.0.0)
