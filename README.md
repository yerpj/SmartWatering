# SmartWatering 
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
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/wiring.jpg "wiring")

When the electric wiring is done, you can now assemble the parts along the water hose (5mm think, respective to the diameter of the Water pump and flow sensor).
(Note, you can pick up some hose on Amazon like [this one](http://www.amazon.co.uk/PVC-Plastic-Pipe-Aquarium-Quality/dp/B00QKQ92ZW)) .

![Alt text](https://github.com/yerpj/SmartWatering/blob/master/Hose.jpg "Hose and Moisture sensor")

Command Line Interface (CLI)
----------------------------

Since revision 0.3.0, the CLI is implemented. It allows to interract with the system through the USB Serial port.
Since revision 0.4.0, the luminosity sensor is supported

The CLI is based on two main mechanisms:
- synchronous request-response
- asynchronous notification

Responses coming from the Arduino are JSON objects passed as strings.

Synchronous request-response CMD list:

The requests are of the form "sw"+<CMD>.
- "sw info"
- "sw temperature"
- "sw moisture"
- "sw start"
- "sw lumi"


Asynchronous notification:

As for the synchronous mechanism, the answers are JSON-formatted:

{"moisture":"123"}

{"pumpState":true/false}

{"systemError":"problem detail"}


The Arduino proceed to a moisture measurement every WATERING_TIMEOUT [ms], sends the measured value to the serial port, then proceed to a watering, if needed.

You can see in the following picture what the CLI looks like:
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/CLI.JPG "Command Line Interface")

The Prototype
-------------

In order to prevent any damage or flood, I made a prototype around a jug of water. The USB water pump rests at the water bottom. The end of the hose directly flows into the jug.
As the moisture sensor is not put into water, it will sense something like a "dry earth", and therefore the system will trigger a watering cycle every WATERING_TIMEOUT [ms]. 
![Alt text](https://github.com/yerpj/SmartWatering/blob/master/Prototype.JPG "Prototype")


The Code 
--------
```
/* Smart Watering project
 * JRY 2016*/
#define REVISION "0.4.0"
#define AUTHOR "JRY"
 /* Board: Arduino Nano
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
 *    0.3.0 Command line interface (CLI) implemented. JSON based messages. available cmds: "sw info", "sw moisture", "sw temperature"
 *    0.3.1 Added a timestamp in the response of the "sw info" request as well as on pumpState notification
 *    0.3.2 Added a "start" command, that forces a water-cycle
 *    0.4.0 Implementation of the Luminosity driver & request
 */
#include <Wire.h>

//#define DEBUG_MODE

#ifdef DEBUG_MODE
  #define DEBUG(x) Serial.println(x)
#else
  #define DEBUG(x) 
#endif

#define MOISTURE_SENSOR_I2C_ADDR 0x20
#define CAPACITIVE_MEAS_AVERAGING 10
#define CAPACITIVE_THRESHOLD 400
#define WATERING_BASE_QUANTITY 30
#define WATERING_TIMEOUT 5000 //milliseconds
#define LED 13
#define PUMP 4
#define LSENSOR 2
#define MAINLOOP_BASE_DELAY_MS 100
#define MOISTURE_CHECK_DELAY_MS 30000

#define CLI_CMD1 "info"     //get metadata about the system
#define CLI_CMD2 "moisture" //require the moisture value
#define CLI_CMD3 "temp"     //require the temperature value
#define CLI_CMD4 "start"    //start a watering cycle
#define CLI_CMD5 "lumi"     //require the luminosity value

#define RXBUFLEN 200
char *RXStrTerminatorOffset=0;
char RXStr[RXBUFLEN];
char RXPtr=0;


volatile int LSensorCount=0;
volatile bool IsWatering=false;
volatile int LastMoistureMeasured=0;
volatile unsigned long int MoistureDelayAccu=0;
char tmp[100];

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

unsigned int getTemp(void){
  Wire.beginTransmission(MOISTURE_SENSOR_I2C_ADDR); // give address
  Wire.write(0x05);            // sends instruction byte
  Wire.endTransmission();     // stop transmitting
  Wire.requestFrom(MOISTURE_SENSOR_I2C_ADDR, 2);
  while (!Wire.available());
  unsigned char a=Wire.read(); // receive a byte as character
  while (!Wire.available());
  unsigned char b=Wire.read(); // receive a byte as character
  return (a*256+b)/10;
}

unsigned int getLumi(void){
  Wire.beginTransmission(MOISTURE_SENSOR_I2C_ADDR); // give address
  Wire.write(0x03);            // sends instruction byte
  Wire.endTransmission();     // stop transmitting
  delay(200);
  Wire.beginTransmission(MOISTURE_SENSOR_I2C_ADDR); // give address
  Wire.write(0x04);            // retrieve the last Luminosity value 
  Wire.endTransmission();     // stop transmitting
  Wire.requestFrom(MOISTURE_SENSOR_I2C_ADDR, 2);
  while (!Wire.available());
  unsigned char a=Wire.read(); // receive a byte as character
  while (!Wire.available());
  unsigned char b=Wire.read(); // receive a byte as character
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
  DEBUG("LSENSOR IRQ");
  digitalWrite(LED,!digitalRead(LED));
}

void Feed(void)
{
  int qty=0;
  int timeout=0;
  LSensorCount=0;//clear current value
  digitalWrite(PUMP,0);//turn pump ON
  delay(20);//mandatory in order to avoid the "turn pump ONOFF" voltage spike to corrupt serial data
  sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"pump\",\"dataValue\":\"ON\"}\n",millis());
  Serial.print(tmp);
  while(LSensorCount<WATERING_BASE_QUANTITY && timeout<WATERING_TIMEOUT)
  {
    delay(200);
    timeout+=200;
  }
  digitalWrite(PUMP,1);//turn pump OFF 
  DEBUG(LSensorCount);
  delay(20);//mandatory in order to avoid the "turn pump ONOFF" voltage spike to corrupt serial data
  sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"pump\",\"dataValue\":\"OFF\"}\n",millis());
  Serial.print(tmp);
  if(timeout>=WATERING_TIMEOUT)
  {
    delay(20);//mandatory in order to avoid the "turn pump ONOFF" voltage spike to corrupt serial data
    Serial.print("{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"systemError\",\"dataValue\":\"Pump problem\"}\n");
  }
}

void CLI(char * cmd){
  if(cmd[0]=='s' && cmd[1]=='w' && cmd[2]==' ')
  {
    if(strstr(cmd,CLI_CMD1)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"info\",\"dataValue\":{\"Project\":{\"revision\":\"%s\",\"author\":\"%s\"},\"system\":{\"time\":\"%lu\"}}}\n",REVISION,AUTHOR,millis());
      Serial.print(tmp);
    }
    else if(strstr(cmd,CLI_CMD2)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"moisture\",\"dataValue\":\"%i\"}\n",MoistureValue());
      Serial.print(tmp);
    }
    else if(strstr(cmd,CLI_CMD3)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"temperature\",\"dataValue\":\"%i\"}\n",getTemp());
      Serial.print(tmp);
    }
    else if(strstr(cmd,CLI_CMD4)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"moisture\",\"dataValue\":\"%i\"}\n",MoistureValue());
      Serial.print(tmp);
      Feed();
      MoistureDelayAccu=0;
    }
    else if(strstr(cmd,CLI_CMD5)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"luminosity\",\"dataValue\":\"%d\"}\n",getLumi());
      Serial.print(tmp);
    }
    else
      Serial.println("unknown parameter");
  }
  else
    Serial.println("unknown command");
}

void setup() {
  //moisture sensor init
  Wire.begin();
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(PUMP, OUTPUT);
  digitalWrite(PUMP, 1);
  pinMode(LSENSOR,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LSENSOR), LSensorIRQ, FALLING); 
}

void loop() {  
  int CapaValue=isHungry();
  /*Serial.println("Moisture value:");
  Serial.print(LastMoistureMeasured);
  if(CapaValue){
    Serial.println("\n:-(");
    Feed();
  }
  else{
    Serial.println("\n:-)");
  }
  delay(10000); */
  if(Serial.available()>0){
    RXStr[RXPtr++]=Serial.read();
    if(RXPtr==RXBUFLEN)
      RXPtr=0;
    RXStrTerminatorOffset=strstr(RXStr,"\r\n");
    if(RXStrTerminatorOffset!=0)
    {
      RXStr[RXPtr]=0;//add a NULL 
      CLI(RXStr);
      RXStr[RXPtr-1]=0;//Invalidate the termination symbol
      RXPtr=0;//Reset the Buffer pointer
    }
  }
  else{
    delay(MAINLOOP_BASE_DELAY_MS);
    MoistureDelayAccu+=MAINLOOP_BASE_DELAY_MS;
    
    if(MoistureDelayAccu>=MOISTURE_CHECK_DELAY_MS)
    {
      MoistureDelayAccu=0;
      CapaValue=isHungry();
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"moisture\",\"dataValue\":\"%i\"}\n",MoistureValue());
      Serial.print(tmp);
      if(CapaValue){
        Feed();
      }
    }    
  }
}
```
