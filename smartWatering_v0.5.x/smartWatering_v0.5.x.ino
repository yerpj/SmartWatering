/* Smart Watering project
 * JRY 2016*/
#define REVISION "0.5.0"
#define AUTHOR "JRY"
 /* Board: Particle Core
 * Pins:
 *    D0  -> SDA
 *    D1  -> SCL
 *    D3  -> Water counter interrupt (has to be 5V tolerant!)
 *    D2  -> Water pump control
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
 *    0.4.1 In progress: software serial to communicate with the ESP8266
 *    0.4.2 ESP8266 is replaced with a Particle Core and Xively feed is now replaced by Particle Cloud.
 *    0.5.0 Code ported to Particle Core instead of Arduino Nano
 */

//#define DEBUG_MODE
#ifdef DEBUG
#undef DEBUG
#endif

#ifdef DEBUG_MODE
  #define DEBUG(x) Serial.println(x)
#else
  #define DEBUG(x) 
#endif

#define MOISTURE_SENSOR_I2C_ADDR 0x20
#define CAPACITIVE_MEAS_AVERAGING 10
#define CAPACITIVE_THRESHOLD 400
#define WATERING_BASE_QUANTITY 30https://build.particle.io/build/570e89ea6518b53cdc000109#devices
#define WATERING_TIMEOUT 5000 //milliseconds
#define PUMP A0
#define LSENSOR D3
#define MAINLOOP_BASE_DELAY_MS 100
#define MOISTURE_CHECK_DELAY_MS 30000

#define CLI_CMD1 "info"     //get metadata about the system
#define CLI_CMD2 "moisture" //require the moisture value
#define CLI_CMD3 "temp"     //require the temperature value
#define CLI_CMD4 "start"    //start a watering cycle
#define CLI_CMD5 "lumi"     //require the luminosity value

#define RXBUFLEN 200
char RXStr[RXBUFLEN];
char RXPtr=0;


volatile int LSensorCount=0;
volatile bool IsWatering=false;
volatile int LastMoistureMeasured=0;
volatile unsigned long int MoistureDelayAccu=0;
char tmp[200];

//cloud sync functions &variables
int CloudRequest(String req);
int Capa=0;
int Temp=0;
int Moisture=0;
int Lumi=0;

int getCapa(void){
  Wire.beginTransmission(MOISTURE_SENSOR_I2C_ADDR); // give address
  Wire.write(0x00);            // sends instruction byte
  Wire.endTransmission();     // stop transmitting
  Wire.requestFrom(MOISTURE_SENSOR_I2C_ADDR, 2);
  while (!Wire.available());
  char a=Wire.read(); // receive a byte as character
  while (!Wire.available());
  char b=Wire.read(); // receive a byte as character
  Capa=((a<<8)+b);
  return Capa;
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
  Temp=(a*256+b)/10;
  return Temp;
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
  Lumi=((a<<8)+b);
  return Lumi;
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
  Moisture=acc;
  return Moisture;
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
}

void Feed(void)
{
  int timeout=0;
  LSensorCount=0;//clear current value
  digitalWrite(PUMP,0);//turn pump ON
  delay(20);//mandatory in order to avoid the "turn pump ONOFF" voltage spike to corrupt serial data
  sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"pump\",\"dataValue\":\"ON\"}\r\n"/*,millis()*/);
  Serial.print(tmp);
  while(LSensorCount<WATERING_BASE_QUANTITY && timeout<WATERING_TIMEOUT)
  {
    delay(200);
    timeout+=200;
  }
  digitalWrite(PUMP,1);//turn pump OFF 
  DEBUG(LSensorCount);
  delay(20);//mandatory in order to avoid the "turn pump ONOFF" voltage spike to corrupt serial data
  sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"pump\",\"dataValue\":\"OFF\"}\r\n"/*,millis()*/);
  Serial.print(tmp);
  if(timeout>=WATERING_TIMEOUT)
  {
    delay(20);//mandatory in order to avoid the "turn pump ONOFF" voltage spike to corrupt serial data
    Serial.print("{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"systemError\",\"dataValue\":\"Pump problem\"}\r\n");
  }
}

int CloudRequest(String req)
{
    if(req==CLI_CMD4)
    {
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"moisture\",\"dataValue\":\"%i\"}\r\n",MoistureValue());
      Serial.print(tmp);
      Feed();
      MoistureDelayAccu=0;
    }
    else
    {
        Serial.println("unknown Cloud command");
    }
}

void CLI(char * cmd){
  if(cmd[0]=='s' && cmd[1]=='w' && cmd[2]==' ')
  {
    if(strstr(cmd,CLI_CMD1)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"info\",\"dataValue\":{\"Project\":{\"revision\":\"%s\",\"author\":\"%s\"},\"system\":{\"time\":\"%lu\"}}}\r\n",REVISION,AUTHOR,millis());
      Serial.print(tmp);
    }
    else if(strstr(cmd,CLI_CMD2)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"moisture\",\"dataValue\":\"%i\"}\r\n",MoistureValue());
      Serial.print(tmp);
    }
    else if(strstr(cmd,CLI_CMD3)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"temperature\",\"dataValue\":\"%i\"}\r\n",getTemp());
      Serial.print(tmp);
    }
    else if(strstr(cmd,CLI_CMD4)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"moisture\",\"dataValue\":\"%i\"}\r\n",MoistureValue());
      Serial.print(tmp);
      Feed();
      MoistureDelayAccu=0;
    }
    else if(strstr(cmd,CLI_CMD5)){
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"luminosity\",\"dataValue\":\"%d\"}\r\n",getLumi());
      Serial.print(tmp);
    }
    else
      Serial.println("unknown parameter");
  }
  else
    Serial.println("unknown command");
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  pinMode(PUMP, OUTPUT);
  digitalWrite(PUMP, 1);
  pinMode(LSENSOR,INPUT_PULLUP);
  attachInterrupt(LSENSOR, LSensorIRQ, FALLING); 
  
  //register cloud functions & variables
  Particle.function("CloudRequest", CloudRequest);
  Particle.variable("Capa", Capa);
  Particle.variable("Temp", Temp);
  Particle.variable("Lumi", Lumi);
  Particle.variable("Moisture", Moisture);
}

void loop() {  
  int CapaValue=isHungry();
  int Moisture=0;

  if(Serial.available()>0){
    RXStr[RXPtr++]=Serial.read();
    Serial.print(RXStr[RXPtr-1]);//echo each character
    if(RXPtr==RXBUFLEN)
      RXPtr=0;
    if(strstr(RXStr,"\n") || strstr(RXStr,"\r"))
    {
        Serial.print("\r\n");
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
      Moisture=MoistureValue();
      sprintf(tmp,"{\"device\":\"sw\",\"type\":\"data\",\"dataPoint\":\"moisture\",\"dataValue\":\"%i\"}\r\n",Moisture);
      Serial.print(tmp);
      delay(100);
      if(CapaValue){
        Feed();
      }
    }    
  }
}