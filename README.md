# SmartWatering
Arduino based project in charge of taking care of my green plants 

This readme is subject to evolve.

Basically, I am using 3 components and a few meters of water hose
- [A I2C soil moisture sensor] (https://www.tindie.com/products/miceuz/i2c-soil-moisture-sensor/)
- [An Arduino nano]	(https://www.arduino.cc/en/Main/ArduinoBoardNano)
- [A water flow meter] (http://www.dx.com/p/hs01-high-precision-flow-meter-white-black-226937#.Vo2cj1JN8Vc)
- [An USB submersible water pump] (http://www.dx.com/p/at-usb-1020-usb-powered-pet-fish-tank-submersible-pump-black-dc-3-5-9v-337664#.Vo2ckFJN8Vc)

Currently, the program does some kind of trivial task:
1) Initialize the hardware
2) Wait some time 
3) Proceed to a moisture measurement
4) if moisture above threshold, goto 2), else goto 5)
5) Turn the pump ON, wait until the desired quantity of water has been watered AND check that timeout did not happen
6) Turn the pump OFF, goto 2)

I will upload some pictures as well as a more detailed README soon.