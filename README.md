# SmartWatering [0.2.2]
Arduino based project in charge of taking care of my green plants 

This readme is subject to evolve.

Basically, I am using 5 parts and a few meters of water hose
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/IMG_0279.JPG "Essential parts")
- [A I2C soil moisture sensor] (https://www.tindie.com/products/miceuz/i2c-soil-moisture-sensor/)
- [An Arduino nano]	(https://www.arduino.cc/en/Main/ArduinoBoardNano)
- [A water flow meter] (http://www.dx.com/p/hs01-high-precision-flow-meter-white-black-226937#.Vo2cj1JN8Vc)
- [An USB submersible water pump] (http://www.dx.com/p/at-usb-1020-usb-powered-pet-fish-tank-submersible-pump-black-dc-3-5-9v-337664#.Vo2ckFJN8Vc)
- An USB power switch (Mosfet, Relay, Integrated switch IC, ...)

Currently, the program does some kind of trivial task:
- 1) Initialize the hardware
- 2) Wait some time 
- 3) Proceed to a moisture measurement
- 4) if moisture above threshold, goto 2), else goto 5)
- 5) Turn the pump ON, wait until the desired quantity of water has been watered AND check that timeout did not happen
- 6) Turn the pump OFF, goto 2)

Modifying the Arduino 
---------------------
The water pump will not work correctly if you power it from the "5V" pin of the Arduino Nano. The reason is, it is not 5V, but ~4.2V because of the reverse current protection diode.
Instead, it is far better to solder a real 5V wire direct on the VUSB (see picture).

![Alt text](https://github.com/yerpj/SmartWatering/blob/master/IMG_0281.JPG "A real 5V wire is used to power the water pump")

Connecting everything 
---------------------
Actually the wiring is straightforward. 
On the Arduino nano, there is 2 external interrupts, on pin D2 and D3. Here, I used D2 to count the pulses coming out the water flow meter
You will also notice the 2 10k pull-up resistors on the I2C bus. They are mandatory, do not forget them.
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/wiring.png "wiring")

When the electric wiring is done, you can now assemble the parts along the water hose (5mm think, respective to the diameter of the Water pump and flow sensor).
(Note, you can pick up some hose on Amazon like [this one](http://www.amazon.co.uk/PVC-Plastic-Pipe-Aquarium-Quality/dp/B00QKQ92ZW)).
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/hose.png "Hose and Moisture sensor")

The Code 
--------
```
/* Smart Watering project
 * JRY 2016
 * Revision: 0.2.2
 * Board: Arduino Nano
 * Pins:
 *    A4->SDA
 *    A5->SCL
 *    D2->Water counter interrupt
 *    D4->Water pump control
 *    
 *    
 *    History:
 *    0.1.0 basic port
 *    0.1.1 getCapa implemented
 *    0.1.5 moisture polling OK
 *    0.2.0 each 5 sec a smile is sent over serial. Happy= no need to feed ; Unhappy= need to feed
 *    0.2.1 Pump driver implemented, LiquidSensor interrupt implemented, feed() function in progress
 *    0.2.2 Feed function implemented, with timeout. Moisture threshold, Watering quantity and Watering timeout are #defined
 */
#include <Wire.h>

#define MOISTURE_SENSOR_I2C_ADDR 0x20
#define CAPACITIVE_MEAS_AVERAGING 10
#define CAPACITIVE_THRESHOLD 400
#define WATERING_BASE_QUANTITY 30
#define WATERING_TIMEOUT 5000 //milliseconds
#define LED 13
#define PUMP 4
#define LSENSOR 2

volatile int LSensorCount=0;
volatile bool IsWatering=false;
volatile int LastMoistureMeasured=0;

int getCapa(void){
  Wire.beginTransmission(MOISTURE_SENSOR_I2C_ADDR); // give address
  Wire.write(0x00);            // sends instruction byte
  Wire.endTransmission();     // stop transmitting
  Wire.requestFrom(MOISTURE_SENSOR_I2C_ADDR, 2);
  while (!Wire.available());
  char a=Wire.read(); // receive a byte as character
  while (!Wire.available());
  char b=Wire.read(); // receive a byte as character
  return ((a<<8)+b);
}

int MoistureValue(void){
  int acc=0;
  getCapa();//dummy Capa read in order to finally get a current value
  for(int i=0;i<CAPACITIVE_MEAS_AVERAGING;i++)
  {
    acc+=getCapa();
  }
  acc=acc/CAPACITIVE_MEAS_AVERAGING;
  LastMoistureMeasured=acc;
  return acc;
}

bool isHungry(void){
  if(MoistureValue()<CAPACITIVE_THRESHOLD)
    return true;
  return false;
}

void LSensorIRQ(void)
{
  LSensorCount++;
  digitalWrite(LED,!digitalRead(LED));
}

void Feed(void)
{
  int qty=0;
  int timeout=0;
  LSensorCount=0;//clear current value
  digitalWrite(PUMP,0);//turn pump ON
  while(LSensorCount<WATERING_BASE_QUANTITY && timeout<WATERING_TIMEOUT)
  {
    delay(200);
    timeout+=200;
  }
  digitalWrite(PUMP,1);//turn pump OFF 
  if(timeout>=WATERING_TIMEOUT)
    Serial.println("Timeout occured in Feed() function."); 
}

void setup() {
  //moisture sensor init
  Wire.begin();
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(PUMP, OUTPUT);
  digitalWrite(PUMP, 1);
  attachInterrupt(digitalPinToInterrupt(LSENSOR), LSensorIRQ, FALLING); 
}

void loop() {  
  int CapaValue=isHungry();
  Serial.println("Moisture value:");
  Serial.print(LastMoistureMeasured);
  if(CapaValue){
    Serial.println("\n:-(");
    Feed();
  }
  else{
    Serial.println("\n:-)");
  }
  delay(10000); 
}
```

The nameless chapter 
--------------------
I will upload some pictures as well as a more detailed README soon.
