/* Smart Watering project
 * JRY 2016
 * Revision: 0.2.0
 * 
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
 */
#include <Wire.h>

#define MOISTURE_SENSOR_I2C_ADDR 0x20
#define CAPACITIVE_MEAS_AVERAGING 5
#define CAPACITIVE_THRESHOLD 420
#define LED 13

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

bool isHungry(void){
  getCapa();//dummy Capa read in order to finally get a current value
  int acc=0;
  for(int i=0;i<CAPACITIVE_MEAS_AVERAGING;i++)
  {
    acc+=getCapa();
  }
  acc=acc/CAPACITIVE_MEAS_AVERAGING;
  if(acc<CAPACITIVE_THRESHOLD)
    return 1;
  return 0;
}

void setup() {
  //moisture sensor init
  Wire.begin();
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

}

void loop() {
  if(isHungry()){
    Serial.println("\n:-(");
    digitalWrite(LED, 1);
  }
  else{
    Serial.println("\n:-)");
    digitalWrite(LED, 0);
  }
  delay(5000); 
}
